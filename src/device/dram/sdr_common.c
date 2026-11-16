/* SPDX-License-Identifier: GPL-2.0-only */

#include <device/dram/sdr_common.h>
#include <lib.h>
#include <string.h>

static bool power_of_two(unsigned int value)
{
	return value && !(value & (value - 1));
}

static unsigned int decode_address_bits(unsigned int value)
{
	/* SDR SPD revision 0/1: row/column encodings 1..3 roll up to 16..18. */
	return value && value <= 3 ? value + 15 : value;
}

enum spd_status spd_decode_sdr_geometry(struct sdr_dimm_geometry *dimm,
				     const uint8_t spd[SPD_SIZE_MAX_SDR])
{
	unsigned int width = spd[SPD_MODULE_DATA_WIDTH_LSB] |
		(spd[SPD_MODULE_DATA_WIDTH_MSB] << 8);
	unsigned int data_width = width;
	unsigned int ranks = spd[SPD_NUM_DIMM_BANKS];
	unsigned int attributes = spd[SPD_MODULE_ATTRIBUTES];
	unsigned int error_type = spd[SPD_DIMM_CONFIG_TYPE];
	unsigned int density_mask = 0;

	memset(dimm, 0, sizeof(*dimm));
	if (spd[SPD_MEMORY_TYPE] != SPD_MEMORY_TYPE_SDRAM ||
	    spd[SPD_NUM_MANUFACTURER_BYTES] < SPD_SIZE_MAX_SDR ||
	    (spd[SPD_SPD_DATA_REVISION_CODE] >> 4) > 1)
		return SPD_STATUS_INVALID;
	if (!ranks || ranks > 8 || !width ||
	    !power_of_two(spd[SPD_NUM_BANKS_PER_SDRAM]) ||
	    (attributes & (SDR_ATTR_REDUNDANT_ADDRESS | 0x80)) ||
	    error_type > SDR_ERROR_ECC)
		return SPD_STATUS_INVALID_FIELD;
	if (error_type) {
		/* Conventional 36/32, 72/64, 144/128 check-bit extensions. */
		if (width % 9)
			return SPD_STATUS_INVALID_FIELD;
		data_width = width / 9 * 8;
	}
	if (data_width < 8 || data_width > 256 || !power_of_two(data_width))
		return SPD_STATUS_INVALID_FIELD;
	if ((!error_type && spd[SPD_ERROR_CHECKING_SDRAM_WIDTH]) ||
	    (error_type && !(spd[SPD_ERROR_CHECKING_SDRAM_WIDTH] & 0x7f)))
		return SPD_STATUS_INVALID_FIELD;
	if (ranks != 2 && ((spd[SPD_NUM_ROWS] & 0xf0) ||
	    (spd[SPD_NUM_COLUMNS] & 0xf0) ||
	    (spd[SPD_PRIMARY_SDRAM_WIDTH] & 0x80) ||
	    (spd[SPD_ERROR_CHECKING_SDRAM_WIDTH] & 0x80)))
		return SPD_STATUS_INVALID_FIELD;

	dimm->mod_width = width;
	dimm->ranks = ranks;
	dimm->banks = spd[SPD_NUM_BANKS_PER_SDRAM];
	dimm->voltage = spd[SPD_MODULE_VOLTAGE];
	dimm->refresh = spd[SPD_REFRESH];
	dimm->attributes = attributes;
	dimm->error_type = error_type;
	for (unsigned int rank = 0; rank < SDR_SPD_ORGANIZATIONS; rank++) {
		struct sdr_rank_geometry *r = &dimm->rank[rank];
		unsigned int rows = spd[SPD_NUM_ROWS] & 15;
		unsigned int cols = spd[SPD_NUM_COLUMNS] & 15;
		unsigned int device_width = spd[SPD_PRIMARY_SDRAM_WIDTH] & 0x7f;
		unsigned int check_width = spd[SPD_ERROR_CHECKING_SDRAM_WIDTH] & 0x7f;
		int cells, size_bits, device_bits;

		if (rank && ranks == 2) {
			if (spd[SPD_NUM_ROWS] >> 4)
				rows = spd[SPD_NUM_ROWS] >> 4;
			if (spd[SPD_NUM_COLUMNS] >> 4)
				cols = spd[SPD_NUM_COLUMNS] >> 4;
			device_width <<= spd[SPD_PRIMARY_SDRAM_WIDTH] >> 7;
			check_width <<= spd[SPD_ERROR_CHECKING_SDRAM_WIDTH] >> 7;
		}
		rows = decode_address_bits(rows);
		cols = decode_address_bits(cols);
		if (!rows || !cols || !power_of_two(device_width) ||
		    device_width > data_width ||
		    (error_type && (!power_of_two(check_width) ||
		     check_width > width - data_width)))
			return SPD_STATUS_INVALID_FIELD;
		cells = rows + cols + log2(dimm->banks);
		size_bits = cells + log2(data_width / 8) - 20;
		device_bits = cells + log2(device_width) - 20;
		/* Byte 31 encodes 4..512 MiB ranks. Validate before shifting. */
		if (size_bits < 2 || size_bits > 9 || device_bits < 0 || device_bits > 15)
			return SPD_STATUS_INVALID_FIELD;
		r->size_mb = 1U << size_bits;
		r->device_mbit = 1U << device_bits;
		r->row_bits = rows;
		r->col_bits = cols;
		r->width = device_width;
		r->check_width = check_width;
		if (rank < ranks)
			density_mask |= r->size_mb / 4;
	}
	if (density_mask != spd[SPD_DENSITY_OF_EACH_ROW_ON_MODULE])
		return SPD_STATUS_INVALID_FIELD;

	return SPD_STATUS_OK;
}
