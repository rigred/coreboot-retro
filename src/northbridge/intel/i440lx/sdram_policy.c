/* SPDX-License-Identifier: GPL-2.0-only */
#include <string.h>
#include "i440lx.h"
#include "sdram_policy.h"

/* Intel 297655-005 replacement Table 14 and datasheet Table 17.
 * No invented page-size or density clamp: missing geometries are disabled.
 */
static bool rank_supported(const struct sdr_dimm_geometry *d, unsigned int rank)
{
	static const uint8_t geometry[][4] = {
		/* row bits, column bits, internal banks, SDRAM component width */
		{11, 8, 2, 16}, {11, 9, 2, 8},
		{11, 10, 2, 16}, {13, 8, 2, 16}, {13, 9, 2, 8},
		{12, 8, 4, 16}, {12, 9, 4, 8},
	};
	if (d->attributes || ((d->refresh & 0x7f) != 0 && (d->refresh & 0x7f) != 3 &&
	    (d->refresh & 0x7f) != 4 && (d->refresh & 0x7f) != 5))
		return false;
	for (unsigned int i = 0; i < sizeof(geometry) / sizeof(geometry[0]); i++)
		if (d->rank[rank].row_bits == geometry[i][0] && d->rank[rank].col_bits == geometry[i][1] &&
		    d->banks == geometry[i][2] && d->rank[rank].width == geometry[i][3])
			return d->rank[rank].size_mb >= 8 && d->rank[rank].size_mb <= 64;
	return false;
}

int lx_sdram_plan(const struct dimm_attr_sdr_st dimms[4], unsigned int sockets,
		  const struct lx_sdram_options *o, struct lx_sdram_plan *p)
{
	unsigned int available = 0, best = 0, best_mb = 0;
	unsigned int available_mb = 0;
	unsigned int limit = sockets == 4 ? 512 : 384;
	memset(p, 0, sizeof(*p));
	p->drt = 0xffff;
	p->all_ecc = true;
	if (sockets < 1 || sockets > 4 ||
	    o->clock_ps < CONFIG_I440LX_SDRAM_MIN_CLOCK_PS || o->clock_ps > 16667 ||
	    !o->max_tac_ps || (o->limit_mb && (o->limit_mb < 8 || o->limit_mb % 8)))
		return -1;
	if (o->limit_mb && o->limit_mb < limit)
		limit = o->limit_mb;
	for (unsigned int i = 0; i < sockets; i++) {
		const struct sdr_dimm_geometry *g = &dimms[i].geometry;
		const struct sdr_dimm_timings *t = &dimms[i].timings;

		if (g->ranks > 2)
			return -1;
		/* The inherited LX path did not accept per-rank component widths.
		 * Keep that electrical limitation here, not in the common decoder.
		 */
		if (g->ranks == 2 && (g->rank[0].width != g->rank[1].width ||
		    g->rank[0].check_width != g->rank[1].check_width))
			return -1;
		/* Unmapped DIMMs still load the memory bus. Do not work around
		 * explicitly excluded electrical/module organizations by disabling CS. */
		if (g->ranks && (g->attributes ||
		    g->voltage != 1 ||
		    (g->mod_width != 64 && g->mod_width != 72) ||
		    !(t->burst_lengths & SPD_BURST_LENGTH_4) ||
		    !(t->cs_latency & 1) || !(t->we_latency & 1) ||
		    t->tCCD > 1 || (t->cas_supported & ~7U) ||
		    !(t->cas_supported & 6) ||
		    (g->rank[0].width != 8 && g->rank[0].width != 16)))
			return -1;
		for (unsigned int r = 0; r < g->ranks; r++) {
			if (g->rank[r].width != 8 &&
			    g->rank[r].width != 16)
				return -1;
			if (!rank_supported(g, r) ||
			    (o->require_ecc && (g->error_type != SDR_ERROR_ECC || g->mod_width != 72))) {
				if (!o->use_supported)
					return -1;
				continue;
			}
			available |= 1U << (i * 2 + r);
			available_mb += g->rank[r].size_mb;
		}
	}
	/* Eight physical ranks: bounded exhaustive subset selection maximizes usable
	 * memory without changing a rank's address translation. Equal totals keep
	 * the earliest mask. No allocation, recursion or DRB overflow.
	 */
	if (available_mb <= limit) {
		best = available;
		best_mb = available_mb;
	}
	for (unsigned int mask = 1; available_mb > limit &&
	     mask < (1U << (sockets * 2)); mask++) {
		unsigned int mb = 0;
		if (mask & ~available)
			continue;
		for (unsigned int r = 0; r < sockets * 2; r++)
			if (mask & (1U << r))
				mb += dimms[r / 2].geometry.rank[r % 2].size_mb;
		if (mb <= limit && mb > best_mb) {
			best = mask;
			best_mb = mb;
		}
	}
	if (!best || (!o->use_supported && best != available))
		return -1;
	p->selected = best;
	p->total_mb = best_mb;
	unsigned int total = 0, cas_mask = 6, trp = 2, trcd = 2;
	for (unsigned int i = 0; i < sockets; i++) {
		const struct dimm_attr_sdr_st *d = &dimms[i];
		if (best & (3U << (i * 2))) {
			p->all_ecc &= d->geometry.error_type == SDR_ERROR_ECC &&
				d->geometry.mod_width == 72;
			for (unsigned int cas = 2; cas <= 3; cas++)
				if (!spd_sdr_supports_cas(&d->timings, cas, o->clock_ps, o->max_tac_ps))
					cas_mask &= ~(1U << (cas - 1));
			/* Non-programmable timing envelope: only PC66-compatible SDRAM.
			 * Never relax these constraints in manual mode. */
			if (d->timings.tRAS * 1000U > 4 * o->clock_ps ||
			    d->timings.tRRD * 1000U > 2 * o->clock_ps ||
			    d->timings.tRP * 1000U > 3 * o->clock_ps || d->timings.tRCD * 1000U > 3 * o->clock_ps)
				return -1;
			if (d->timings.tRP * 1000U > 2 * o->clock_ps)
				trp = 3;
			if (d->timings.tRCD * 1000U > 2 * o->clock_ps)
				trcd = 3;
		}
		for (unsigned int r = 0; r < 2; r++) {
			unsigned int index = i * 2 + r;
			if (best & (1U << index)) {
				total += d->geometry.rank[r].size_mb / 8;
				p->drt &= ~(3U << (index * 2));
				p->drt |= 2U << (index * 2);
			}
			p->drb[index] = total;
		}
	}
	for (unsigned int r = sockets * 2; r < 8; r++)
		p->drb[r] = total;
	p->cas = o->cas ? o->cas : (cas_mask & 2) ? 2 : 3;
	if (p->cas < 2 || p->cas > 3 || !(cas_mask & (1U << (p->cas - 1))))
		return -1;
	if (o->trp) {
		if (o->trp < trp || o->trp > 3)
			return -1;
		trp = o->trp;
	}
	if (o->trcd) {
		if (o->trcd < trcd || o->trcd > 3)
			return -1;
		trcd = o->trcd;
	}
	p->timing = (p->cas == 2 ? DRAMT_CAS_2 : 0) | (trcd == 2 ? DRAMT_SRCD_2 : 0) |
		(trp == 2 ? DRAMT_SRPT_2 : 0) | (o->fast_ma ? DRAMT_FAST_MA : 0);
	return 0;
}
