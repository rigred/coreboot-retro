/* SPDX-License-Identifier: GPL-2.0-only */
#include <string.h>
#include "i440lx.h"
#include "sdram_policy.h"

/* Intel 297655-005 replacement Table 14 and datasheet Table 17.
 * No invented page-size or density clamp: missing geometries are disabled.
 */
static bool rank_supported(const struct sdr_dimm *d, unsigned int rank)
{
	static const uint8_t geometry[][4] = {
		/* row bits, column bits, internal banks, SDRAM component width */
		{11, 8, 2, 16}, {11, 9, 2, 8},
		{11, 10, 2, 16}, {13, 8, 2, 16}, {13, 9, 2, 8},
		{12, 8, 4, 16}, {12, 9, 4, 8},
	};
	if (d->attributes || (d->refresh != 0 && d->refresh != 3 &&
	    d->refresh != 4 && d->refresh != 5))
		return false;
	for (unsigned int i = 0; i < sizeof(geometry) / sizeof(geometry[0]); i++)
		if (d->rows[rank] == geometry[i][0] && d->cols[rank] == geometry[i][1] &&
		    d->banks == geometry[i][2] && d->device_width == geometry[i][3])
			return d->rank_mb[rank] >= 8 && d->rank_mb[rank] <= 64;
	return false;
}

int lx_sdram_plan(const struct sdr_dimm dimms[4], unsigned int sockets,
		  const struct lx_sdram_options *o, struct lx_sdram_plan *p)
{
	unsigned int available = 0, best = 0, best_mb = 0;
	unsigned int available_mb = 0;
	unsigned int limit = sockets == 4 ? 512 : 384;
	memset(p, 0, sizeof(*p));
	p->drt = 0xffff;
	p->all_ecc = true;
	if (sockets < 1 || sockets > 4 || o->clock_ps < 15000 || o->clock_ps > 16667 ||
	    !o->max_tac_ps || (o->limit_mb && (o->limit_mb < 8 || o->limit_mb % 8)))
		return -1;
	if (o->limit_mb && o->limit_mb < limit)
		limit = o->limit_mb;
	for (unsigned int i = 0; i < sockets; i++) {
		if (dimms[i].ranks > 2)
			return -1;
		/* Unmapped DIMMs still load the memory bus. Do not work around
		 * explicitly excluded electrical/module organizations by disabling CS. */
		if (dimms[i].ranks && (dimms[i].attributes ||
		    (dimms[i].device_width != 8 && dimms[i].device_width != 16)))
			return -1;
		for (unsigned int r = 0; r < dimms[i].ranks; r++) {
			if (!rank_supported(&dimms[i], r) || (o->require_ecc && !dimms[i].ecc)) {
				if (!o->use_supported)
					return -1;
				continue;
			}
			available |= 1U << (i * 2 + r);
			available_mb += dimms[i].rank_mb[r];
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
				mb += dimms[r / 2].rank_mb[r % 2];
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
		const struct sdr_dimm *d = &dimms[i];
		if (best & (3U << (i * 2))) {
			p->all_ecc &= d->ecc;
			for (unsigned int cas = 2; cas <= 3; cas++)
				if (!sdr_supports_cas(d, cas, o->clock_ps, o->max_tac_ps))
					cas_mask &= ~(1U << (cas - 1));
			/* Non-programmable timing envelope: only PC66-compatible SDRAM.
			 * Never relax these constraints in manual mode. */
			if (d->tras_ps > 4 * o->clock_ps || d->trrd_ps > 2 * o->clock_ps ||
			    d->trp_ps > 3 * o->clock_ps || d->trcd_ps > 3 * o->clock_ps)
				return -1;
			if (d->trp_ps > 2 * o->clock_ps)
				trp = 3;
			if (d->trcd_ps > 2 * o->clock_ps)
				trcd = 3;
		}
		for (unsigned int r = 0; r < 2; r++) {
			unsigned int index = i * 2 + r;
			if (best & (1U << index)) {
				total += d->rank_mb[r] / 8;
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
