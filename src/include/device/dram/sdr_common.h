/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef DEVICE_DRAM_SDR_COMMON_H
#define DEVICE_DRAM_SDR_COMMON_H

#include <device/dram/common.h>
#include <spd.h>
#include <stdbool.h>

/* Exactly represents the decimal 0.1 ns and quarter-ns SDR SPD fields. */
#define SDR_TIMEBASE_PS 50
#define SPD_SIZE_MAX_SDR 64
#define SDR_SPD_ORGANIZATIONS 2

enum sdr_module_attribute {
	SDR_ATTR_BUFFERED = MODULE_BUFFERED | (1 << 3),
	SDR_ATTR_REGISTERED = MODULE_REGISTERED | (1 << 4),
	SDR_ATTR_PLL = 1 << 2,
	SDR_ATTR_DIFFERENTIAL_CLOCK = 1 << 5,
	SDR_ATTR_REDUNDANT_ADDRESS = 1 << 6,
};

enum sdr_error_type {
	SDR_ERROR_NONE,
	SDR_ERROR_PARITY,
	SDR_ERROR_ECC,
};

/* Two organizations, not a two-rank limit. More than two ranks must be
 * symmetric because SPD has only two geometry fields.
 */
struct sdr_rank_geometry {
	uint16_t size_mb;
	uint16_t device_mbit;
	uint8_t row_bits;
	uint8_t col_bits;
	uint8_t width;
	uint8_t check_width;
};

struct sdr_dimm_geometry {
	struct sdr_rank_geometry rank[SDR_SPD_ORGANIZATIONS];
	uint16_t mod_width; /* Includes check bits; size_mb excludes them. */
	uint8_t ranks;
	uint8_t banks; /* Internal banks per component, not module ranks. */
	uint8_t voltage;
	uint8_t refresh; /* Raw byte, including self-refresh capability. */
	uint8_t attributes;
	uint8_t error_type;
};

/* No transport/controller policy. Caller supplies valid bytes 0..31 and 62.
 * No checksum check here. Output is valid only on SPD_STATUS_OK.
 */
enum spd_status spd_decode_sdr_geometry(struct sdr_dimm_geometry *dimm,
				     const uint8_t spd[SPD_SIZE_MAX_SDR]);

static inline bool spd_dimm_is_registered_sdr(uint8_t attributes)
{
	return attributes & SDR_ATTR_REGISTERED;
}

static inline bool spd_dimm_is_buffered_sdr(uint8_t attributes)
{
	return attributes & SDR_ATTR_BUFFERED;
}

static inline unsigned int spd_sdr_size_mb(const struct sdr_dimm_geometry *dimm)
{
	return dimm->rank[0].size_mb * (dimm->ranks - 1) +
		dimm->rank[dimm->ranks > 1].size_mb;
}

#endif /* DEVICE_DRAM_SDR_COMMON_H */
