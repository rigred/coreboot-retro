/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef DEVICE_DRAM_SDR_H
#define DEVICE_DRAM_SDR_H

#include <device/dram/sdr_common.h>

struct sdr_dimm_timings {
	/* Index 0 = maximum CAS, 1 = maximum-1, 2 = maximum-2.
	 * Not an index into set bits. Units: SDR_TIMEBASE_PS; zero = absent.
	 */
	uint16_t cycle_time[3];
	uint16_t access_time[3];
	uint8_t cas_supported; /* Bit 0 = CL1, unlike DDR's CAS masks. */
	uint8_t cas_max;
	uint8_t cs_latency; /* Bit 0 = zero-clock CS latency. */
	uint8_t we_latency;
	uint8_t burst_lengths;
	uint8_t tCCD; /* Clocks. */
	uint8_t tRP, tRCD, tRAS, tRRD; /* Whole nanoseconds. */
};

struct dimm_attr_sdr_st {
	struct sdr_dimm_geometry geometry;
	struct sdr_dimm_timings timings;
};

/* Full checksum-validated decoder, following the DDR2 API convention.
 * Caller reads a complete snapshot using its existing SMBus/I2C backend.
 * Output is valid only on SPD_STATUS_OK. No controller policy is applied.
 */
enum spd_status spd_decode_sdr(struct dimm_attr_sdr_st *dimm,
			     const uint8_t spd[SPD_SIZE_MAX_SDR]);

/* Geometry-only users can omit this entry point and all timing code. */
enum spd_status spd_decode_sdr_timings(struct sdr_dimm_timings *timings,
				    const uint8_t spd[SPD_SIZE_MAX_SDR]);
bool spd_sdr_supports_cas(const struct sdr_dimm_timings *timings, unsigned int cas,
			unsigned int clock_ps, unsigned int max_tac_ps);

#endif /* DEVICE_DRAM_SDR_H */
