/* SPDX-License-Identifier: GPL-2.0-only */
#include <assert.h>
#include <device/dram/sdr.h>
#include <stdio.h>
#include <string.h>
#include "sdram_policy.h"

static void checksum(uint8_t spd[64])
{
	uint8_t value = 0;

	for (unsigned int i = 0; i < 63; i++)
		value += spd[i];
	spd[63] = value;
}

static void fixture(uint8_t spd[64])
{
	memset(spd, 0, 64);
	spd[0] = 128;
	spd[1] = 8;
	spd[2] = SPD_MEMORY_TYPE_SDRAM;
	spd[3] = 12;
	spd[4] = 9;
	spd[5] = 2;
	spd[6] = 64;
	spd[8] = 1;
	spd[9] = 0x75;
	spd[10] = 0x54;
	spd[12] = 0x80;
	spd[13] = 8;
	spd[15] = 1;
	spd[16] = 4;
	spd[17] = 4;
	spd[18] = 6;
	spd[19] = 1;
	spd[20] = 1;
	spd[23] = 0xa0;
	spd[24] = 0x60;
	spd[27] = 20;
	spd[28] = 15;
	spd[29] = 20;
	spd[30] = 45;
	spd[31] = 16;
	spd[62] = 0x12;
	checksum(spd);
}

static enum spd_status decode(uint8_t spd[64], struct dimm_attr_sdr_st *dimm)
{
	checksum(spd);
	return spd_decode_sdr(dimm, spd);
}

int main(void)
{
	uint8_t spd[64];
	struct dimm_attr_sdr_st d, dimms[4] = {0};
	struct lx_sdram_plan plan;
	struct lx_sdram_options options = {
		.clock_ps = 15000, .max_tac_ps = 9000, .use_supported = true,
	};

	fixture(spd);
	assert(decode(spd, &d) == SPD_STATUS_OK);
	assert(sizeof(d) == 46 && sizeof(d.geometry) == 24);
	assert(spd_sdr_size_mb(&d.geometry) == 128);
	assert(d.geometry.rank[0].size_mb == 64 && d.geometry.rank[0].device_mbit == 64);
	assert(d.timings.cycle_time[0] == 150 && d.timings.access_time[0] == 108);
	assert(spd_sdr_supports_cas(&d.timings, 2, 15000, 9000));
	assert(!spd_sdr_supports_cas(&d.timings, 2, 9999, 9000));
	assert(!spd_sdr_supports_cas(&d.timings, 2, 15000, 5999));
	assert(!spd_sdr_supports_cas(&d.timings, 0, 15000, 9000));
	assert(!spd_sdr_supports_cas(&d.timings, 8, 15000, 9000));
	dimms[0] = d;
	assert(!lx_sdram_plan(dimms, 3, &options, &plan));
	assert(plan.total_mb == 128 && plan.cas == 2);
	options.cas = 3; options.trp = options.trcd = 3;
	assert(!lx_sdram_plan(dimms, 3, &options, &plan) && plan.cas == 3);
	options.trp = 1;
	assert(lx_sdram_plan(dimms, 3, &options, &plan));
	options.cas = options.trp = options.trcd = 0;

	/* Full 256 MiB geometry parses; unchanged LX whitelist still rejects it. */
	spd[4] = 10; spd[31] = 32;
	assert(decode(spd, &d) == SPD_STATUS_OK && spd_sdr_size_mb(&d.geometry) == 256);
	dimms[0] = d;
	assert(lx_sdram_plan(dimms, 3, &options, &plan));

	/* Registered ECC metadata is portable; LX cannot enable this module. */
	spd[6] = 72; spd[11] = 2; spd[14] = 8; spd[21] = 0x12;
	assert(decode(spd, &d) == SPD_STATUS_OK);
	assert(spd_dimm_is_registered_sdr(d.geometry.attributes));
	assert(d.geometry.error_type == SDR_ERROR_ECC && spd_sdr_size_mb(&d.geometry) == 256);
	dimms[0] = d;
	assert(lx_sdram_plan(dimms, 3, &options, &plan));
	spd[21] = 9;
	assert(decode(spd, &d) == SPD_STATUS_OK && spd_dimm_is_buffered_sdr(d.geometry.attributes));
	spd[21] = 0; spd[11] = 1;
	assert(decode(spd, &d) == SPD_STATUS_OK && d.geometry.error_type == SDR_ERROR_PARITY);
	spd[11] = 2; spd[13] = 4; spd[14] = 4;
	assert(decode(spd, &d) == SPD_STATUS_OK && d.geometry.rank[0].device_mbit == 64);
	spd[5] = 4; spd[21] = 0x12;
	assert(decode(spd, &d) == SPD_STATUS_OK && spd_sdr_size_mb(&d.geometry) == 512);
	spd[5] = 8;
	assert(decode(spd, &d) == SPD_STATUS_OK && spd_sdr_size_mb(&d.geometry) == 1024);
	spd[5] = 9;
	assert(decode(spd, &d) == SPD_STATUS_INVALID_FIELD);

	/* Asymmetric organizations and doubled second-rank component widths. */
	fixture(spd); spd[3] = 0xdc; spd[13] = 0x88; spd[31] = 16 | 32;
	assert(decode(spd, &d) == SPD_STATUS_OK);
	assert(d.geometry.rank[0].size_mb == 64 && d.geometry.rank[1].size_mb == 128);
	assert(d.geometry.rank[1].width == 16 && spd_sdr_size_mb(&d.geometry) == 192);
	dimms[0] = d;
	assert(lx_sdram_plan(dimms, 3, &options, &plan)); /* Unqualified mixed widths. */
	spd[5] = 3;
	assert(decode(spd, &d) == SPD_STATUS_INVALID_FIELD);
	fixture(spd); spd[3] = 1; spd[4] = 4; spd[31] = 8;
	assert(decode(spd, &d) == SPD_STATUS_OK && d.geometry.rank[0].row_bits == 16);

	fixture(spd); spd[18] = 7; spd[25] = 0x41; spd[26] = 0x25;
	assert(decode(spd, &d) == SPD_STATUS_OK);
	assert(d.timings.cycle_time[2] == 325 && d.timings.access_time[2] == 185);
	assert(spd_sdr_supports_cas(&d.timings, 1, 16250, 9250));
	spd[25] = spd[26] = 0;
	assert(decode(spd, &d) == SPD_STATUS_OK && !spd_sdr_supports_cas(&d.timings, 1, 20000, 9000));
	spd[23] = 0x10;
	assert(decode(spd, &d) == SPD_STATUS_OK && d.timings.cycle_time[1] == 320);
	spd[9] = 0x10;
	assert(decode(spd, &d) == SPD_STATUS_OK && d.timings.cycle_time[0] == 20);
	spd[9] = 0x7a;
	assert(decode(spd, &d) == SPD_STATUS_INVALID_FIELD);
	fixture(spd); spd[18] = 5; spd[25] = 0x40; spd[26] = 0x24;
	assert(decode(spd, &d) == SPD_STATUS_OK && d.timings.cycle_time[1] == 0);
	assert(spd_sdr_supports_cas(&d.timings, 1, 16000, 9000));

	fixture(spd); spd[63] ^= 1;
	assert(spd_decode_sdr(&d, spd) == SPD_STATUS_CRC_ERROR);
	fixture(spd); spd[2] = SPD_MEMORY_TYPE_SDRAM_DDR2;
	assert(decode(spd, &d) == SPD_STATUS_INVALID);
	fixture(spd); spd[62] = 0x20;
	assert(decode(spd, &d) == SPD_STATUS_INVALID);
	fixture(spd); spd[31] = 0;
	assert(decode(spd, &d) == SPD_STATUS_INVALID_FIELD);
	fixture(spd); spd[14] = 8;
	assert(decode(spd, &d) == SPD_STATUS_INVALID_FIELD);
	fixture(spd); spd[17] = 3;
	assert(decode(spd, &d) == SPD_STATUS_INVALID_FIELD);
	fixture(spd); spd[21] = 0x40;
	assert(decode(spd, &d) == SPD_STATUS_INVALID_FIELD);

	/* Exhaustive byte mutations catch shift/bounds errors under UBSan. */
	for (unsigned int offset = 0; offset < 64; offset++) {
		for (unsigned int value = 0; value < 256; value++) {
			fixture(spd); spd[offset] = value;
			(void)decode(spd, &d);
		}
	}
	puts("PASS: SDR geometry/timings/checksum, ECC/registered/buffered/asymmetric modules, LX policy, 16384 SPD mutations");
	return 0;
}
