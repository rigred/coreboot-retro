/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef I440LX_SDRAM_POLICY_H
#define I440LX_SDRAM_POLICY_H
#include <device/dram/sdr.h>

struct lx_sdram_options {
	unsigned int clock_ps, max_tac_ps, limit_mb;
	unsigned int cas, trcd, trp; /* zero = SPD auto, otherwise 2 or 3 */
	bool use_supported, fast_ma, require_ecc;
};

struct lx_sdram_plan {
	uint8_t drb[8], selected, cas, timing;
	uint16_t drt, total_mb;
	bool all_ecc;
};

/* Pure policy: no SMBus, CMOS, PCI or CPU-specific operations. */
int lx_sdram_plan(const struct sdr_dimm dimms[4], unsigned int sockets,
		  const struct lx_sdram_options *options, struct lx_sdram_plan *plan);
#endif
