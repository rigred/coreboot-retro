/* SPDX-License-Identifier: GPL-2.0-only */

#include <device/dram/sdr.h>
#include <lib.h>
#include <string.h>

/* SDR SPD byte 63 stores the additive checksum of bytes 0 through 62. */
static uint8_t sdr_spd_checksum(const uint8_t spd[SPD_SIZE_MAX_SDR])
{
	uint8_t checksum = 0;

	for (unsigned int i = 0; i < SPD_CHECKSUM_FOR_BYTES_0_TO_62; i++)
		checksum += spd[i];

	return checksum;
}

static uint16_t decode_decimal_time(uint8_t value, bool rollover)
{
	unsigned int ns = value >> 4;

	if (!ns || (value & 15) > 9)
		return 0;
	/* Only SDR bytes 23/24 roll 1..3 ns up to 16..18 ns. DDR2's fractional
	 * nibble encodings differ and must not be reused here.
	 */
	if (rollover && ns <= 3)
		ns += 15;

	return ns * 20 + (value & 15) * 2;
}

enum spd_status spd_decode_sdr_timings(struct sdr_dimm_timings *t,
				    const uint8_t spd[SPD_SIZE_MAX_SDR])
{
	unsigned int cas_mask = spd[SPD_ACCEPTABLE_CAS_LATENCIES];

	memset(t, 0, sizeof(*t));
	if (spd[SPD_MEMORY_TYPE] != SPD_MEMORY_TYPE_SDRAM ||
	    (spd[SPD_SPD_DATA_REVISION_CODE] >> 4) > 1)
		return SPD_STATUS_INVALID;
	if (!cas_mask || (cas_mask & 0x80) || !spd[SPD_CS_LATENCY] ||
	    !spd[SPD_WE_LATENCY] || !spd[SPD_SUPPORTED_BURST_LENGTHS])
		return SPD_STATUS_INVALID_FIELD;
	t->cas_supported = cas_mask;
	t->cas_max = log2(cas_mask) + 1;
	t->cs_latency = spd[SPD_CS_LATENCY];
	t->we_latency = spd[SPD_WE_LATENCY];
	t->burst_lengths = spd[SPD_SUPPORTED_BURST_LENGTHS];
	t->tCCD = spd[SPD_MIN_CLOCK_DELAY_B2B_RAND_COLUMN];
	t->tRP = spd[SPD_MIN_ROW_PRECHARGE_TIME];
	t->tRCD = spd[SPD_MIN_RAS_TO_CAS_DELAY];
	t->tRAS = spd[SPD_MIN_ACTIVE_TO_PRECHARGE_DELAY];
	t->tRRD = spd[SPD_MIN_ROWACTIVE_TO_ROWACTIVE];
	if (!t->tRP || !t->tRCD || !t->tRAS || !t->tRRD)
		return SPD_STATUS_INVALID_FIELD;
	for (unsigned int i = 0; i < ARRAY_SIZE(t->cycle_time); i++) {
		unsigned int cycle_byte = i ? SPD_SDRAM_CYCLE_TIME_2ND + (i - 1) * 2 :
			SPD_MIN_CYCLE_TIME_AT_CAS_MAX;

		if (t->cas_max <= i || !(cas_mask & (1U << (t->cas_max - i - 1))))
			continue;
		/* Lower-CAS timing pairs may be omitted. Retain the advertised CAS
		 * mask, but never claim that an unspecified timing is usable.
		 */
		if (i && !spd[cycle_byte] && !spd[cycle_byte + 1])
			continue;
		if (i == 2) {
			/* Bytes 25/26: six whole-ns bits and two quarter-ns bits. */
			if (!(spd[cycle_byte] >> 2) || !(spd[cycle_byte + 1] >> 2))
				return SPD_STATUS_INVALID_FIELD;
			t->cycle_time[i] = spd[cycle_byte] * 5;
			t->access_time[i] = spd[cycle_byte + 1] * 5;
		} else {
			t->cycle_time[i] = decode_decimal_time(spd[cycle_byte], i != 0);
			t->access_time[i] = decode_decimal_time(spd[cycle_byte + 1], i != 0);
		}
		if (!t->cycle_time[i] || !t->access_time[i])
			return SPD_STATUS_INVALID_FIELD;
	}

	return SPD_STATUS_OK;
}

enum spd_status spd_decode_sdr(struct dimm_attr_sdr_st *dimm,
			     const uint8_t spd[SPD_SIZE_MAX_SDR])
{
	enum spd_status ret;

	memset(dimm, 0, sizeof(*dimm));
	if (sdr_spd_checksum(spd) !=
	    spd[SPD_CHECKSUM_FOR_BYTES_0_TO_62])
		return SPD_STATUS_CRC_ERROR;
	ret = spd_decode_sdr_geometry(&dimm->geometry, spd);
	if (ret != SPD_STATUS_OK)
		return ret;

	return spd_decode_sdr_timings(&dimm->timings, spd);
}

bool spd_sdr_supports_cas(const struct sdr_dimm_timings *t, unsigned int cas,
			unsigned int clock_ps, unsigned int max_tac_ps)
{
	unsigned int index;

	if (!cas || cas > t->cas_max || !(t->cas_supported & (1U << (cas - 1))))
		return false;
	index = t->cas_max - cas;
	if (index >= ARRAY_SIZE(t->cycle_time))
		return false;

	return t->cycle_time[index] && t->cycle_time[index] * SDR_TIMEBASE_PS <= clock_ps &&
		t->access_time[index] && t->access_time[index] * SDR_TIMEBASE_PS <= max_tac_ps;
}
