/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef DEVICE_DRAM_SDR_H
#define DEVICE_DRAM_SDR_H

#include <stdbool.h>
#include <stdint.h>

#define SDR_SPD_BYTES 64
#define SDR_MAX_RANKS 2

/* SDR-specific encodings; do not use spd.h's DDR half-clock CAS masks. */
struct sdr_dimm {
	uint16_t rank_mb[SDR_MAX_RANKS];
	uint8_t rows[SDR_MAX_RANKS], cols[SDR_MAX_RANKS];
	uint8_t ranks, banks, device_width, attributes, refresh;
	uint8_t cas_mask;
	uint32_t tck_ps[4], tac_ps[4]; /* entries indexed by CAS latency */
	uint32_t trp_ps, trcd_ps, tras_ps, trrd_ps;
	bool ecc;
};

/* Pure decoder. Caller supplies a complete, successfully read SPD snapshot. */
int sdr_spd_decode(const uint8_t spd[SDR_SPD_BYTES], struct sdr_dimm *dimm);
bool sdr_supports_cas(const struct sdr_dimm *dimm, unsigned int cas,
		     unsigned int clock_ps, unsigned int max_tac_ps);

#endif
