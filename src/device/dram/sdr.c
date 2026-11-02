/* SPDX-License-Identifier: GPL-2.0-only */
#include <device/dram/sdr.h>
#include <string.h>

/* Intel PC SDRAM SPD 1.2B, bytes 9/10 and 23/24: decimal nibbles.
 * At revision 1.x, 1..3 in the whole-ns nibble encode 16..18 ns.
 */
static unsigned int decimal_ps(uint8_t byte, uint8_t revision)
{
	unsigned int ns = byte >> 4;
	if (!ns || (byte & 15) > 9)
		return 0;
	if (ns <= 3 && (revision >> 4) <= 1)
		ns += 15;
	return ns * 1000 + (byte & 15) * 100;
}

int sdr_spd_decode(const uint8_t spd[SDR_SPD_BYTES], struct sdr_dimm *d)
{
	unsigned int checksum = 0;
	unsigned int density_mask = 0;
	memset(d, 0, sizeof(*d));
	for (unsigned int i = 0; i < 63; i++)
		checksum += spd[i];
	if ((checksum & 255) != spd[63] || spd[0] < 64 || (spd[62] >> 4) > 1 || spd[2] != 4 ||
	    spd[8] != 1 || spd[5] < 1 || spd[5] > 2 || spd[7] ||
	    (spd[6] != 64 && spd[6] != 72) || spd[11] > 2 ||
	    (spd[6] == 64 && spd[11]) || (spd[6] == 72 && !spd[11]) ||
	    (spd[6] == 64 && spd[14]) ||
	    (spd[6] == 72 && (!spd[14] || spd[14] > 8 || (spd[14] & (spd[14] - 1)))) ||
	    (spd[13] != 4 && spd[13] != 8 && spd[13] != 16 && spd[13] != 32) ||
	    (spd[17] != 2 && spd[17] != 4) || !(spd[16] & 4) ||
	    !(spd[19] & 1) || !(spd[20] & 1) || spd[15] > 1 ||
	    (spd[18] & 0xf8) || !(spd[18] & 6))
		return -1;
	d->ranks = spd[5];
	d->banks = spd[17];
	d->device_width = spd[13];
	d->attributes = spd[21];
	d->refresh = spd[12] & 0x7f;
	d->cas_mask = spd[18];
	d->ecc = spd[11] == 2 && spd[6] == 72;
	for (unsigned int rank = 0; rank < d->ranks; rank++) {
		unsigned int rows = rank && (spd[3] >> 4) ? spd[3] >> 4 : spd[3] & 15;
		unsigned int cols = rank && (spd[4] >> 4) ? spd[4] >> 4 : spd[4] & 15;
		if (rows < 10 || rows > 14 || cols < 8 || cols > 12)
			return -1;
		unsigned int mb = (1U << (rows + cols - 17)) * d->banks;
		if (mb < 4 || mb > 512)
			return -1;
		/* Byte 31 is an unordered set, not a physical chip-select order.
		 * Retain rank order from the row/column nibbles and cross-check it. */
		density_mask |= mb / 4;
		d->rank_mb[rank] = mb;
		d->rows[rank] = rows;
		d->cols[rank] = cols;
	}
	if (density_mask != spd[31])
		return -1;
	unsigned int highest = (spd[18] & 4) ? 3 : 2;
	d->tck_ps[highest] = decimal_ps(spd[9], spd[62]);
	d->tac_ps[highest] = decimal_ps(spd[10], spd[62]);
	if (highest == 3 && (spd[18] & 2)) {
		d->tck_ps[2] = decimal_ps(spd[23], spd[62]);
		d->tac_ps[2] = decimal_ps(spd[24], spd[62]);
	}
	d->trp_ps = spd[27] * 1000U;
	d->trcd_ps = spd[29] * 1000U;
	d->tras_ps = spd[30] * 1000U;
	d->trrd_ps = spd[28] * 1000U;
	return d->tck_ps[highest] && d->tac_ps[highest] && d->trp_ps &&
		d->trcd_ps && d->tras_ps && d->trrd_ps ? 0 : -1;
}

bool sdr_supports_cas(const struct sdr_dimm *d, unsigned int cas,
		     unsigned int clock_ps, unsigned int max_tac_ps)
{
	return cas >= 2 && cas <= 3 && (d->cas_mask & (1U << (cas - 1))) &&
		d->tck_ps[cas] && d->tck_ps[cas] <= clock_ps &&
		d->tac_ps[cas] && d->tac_ps[cas] <= max_tac_ps;
}
