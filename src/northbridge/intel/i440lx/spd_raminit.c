/* SPDX-License-Identifier: GPL-2.0-only */
#include <console/console.h>
#include <commonlib/console/post_codes.h>
#include <device/pci_ops.h>
#include <device/smbus_host.h>
#include <option.h>
#include "i440lx.h"
#include "raminit.h"
#include "sdram_policy.h"

void lx_spd_configure(struct lx_sdram_plan *plan, unsigned int ecc_mode, int cmos_valid)
{
	struct sdr_dimm dimms[4] = {0};
	uint8_t spd[SDR_SPD_BYTES];
	const unsigned int capacities[] = {0, 64, 128, 192, 256, 384, 512};
	unsigned int capacity = cmos_valid ? get_uint_option("ram_limit", 0) : 0;
	unsigned int policy = cmos_valid ? get_uint_option("ram_capacity", 1) : 1;
	unsigned int manual = cmos_valid ? get_uint_option("ram_config", 0) : 0;
	unsigned int ma = manual ? get_uint_option("ma_wait_state", 0) : 0;
	struct lx_sdram_options options = {
		.clock_ps = CONFIG_I440LX_SDRAM_CLOCK_PS,
		.max_tac_ps = CONFIG_I440LX_SDRAM_MAX_TAC_PS,
		.use_supported = policy == 1,
		.fast_ma = CONFIG(I440LX_SDRAM_FAST_MA),
		.require_ecc = ecc_mode != 0,
	};
	if (!!(pci_read_config16(NB, PACCFG) & PACCFG_MEMCFG1) != CONFIG(SDRAM_4DIMM))
		die_with_post_code(POSTCODE_RAM_FAILURE, "LX DIMM topology disagrees with CKE strap\n");
	if (!cmos_valid)
		printk(BIOS_NOTICE, "RAM CMOS policy not initialized; using SPD defaults, ECC off\n");
	if (manual > 1 || policy > 1 || capacity >= ARRAY_SIZE(capacities) || ecc_mode > 2)
		die_with_post_code(POSTCODE_RAM_FAILURE, "Invalid RAM CMOS selection\n");
	options.limit_mb = capacities[capacity];
	if (manual) {
		unsigned int cas = get_uint_option("cas_latency", 0);
		unsigned int rcd = get_uint_option("ras_to_cas_delay", 0);
		unsigned int rp = get_uint_option("ras_precharge_time", 0);
		if (cas > 1 || rcd > 1 || rp > 1 || ma > 1 ||
		    (ma && !CONFIG(I440LX_SDRAM_FAST_MA)))
			die_with_post_code(POSTCODE_RAM_FAILURE, "RAM manual override not qualified\n");
		options.cas = cas ? 2 : 3;
		options.trcd = rcd ? 2 : 3;
		options.trp = rp ? 2 : 3;
		options.fast_ma = ma;
	}
	for (unsigned int slot = 0; slot < DIMM_SOCKETS; slot++) {
		unsigned int address = CONFIG(SDRAM_3DIMM_REVERSE_OFFSET) ?
			DIMM0 - slot : DIMM0 + slot;
		int type = smbus_read_byte(address, 2);
		if (type < 0 || type == 0xff || type == 0)
			continue; /* SMBus NACK cannot distinguish absent from unreadable SPD. */
		if (type != 4)
			die_with_post_code(POSTCODE_RAM_FAILURE, "Only SDR SDRAM initialization supported\n");
		for (unsigned int byte = 0; byte < sizeof(spd); byte++) {
			int value = smbus_read_byte(address, byte);
			if (value < 0)
				die_with_post_code(POSTCODE_RAM_FAILURE, "SPD read failed\n");
			spd[byte] = value;
		}
		if (sdr_spd_decode(spd, &dimms[slot]))
			die_with_post_code(POSTCODE_RAM_FAILURE, "SPD checksum/geometry/timing invalid\n");
		printk(BIOS_INFO, "DIMM%u SPD %02x: %u ranks %u/%u MiB, x%u, %s\n",
			slot, address, dimms[slot].ranks, dimms[slot].rank_mb[0],
			dimms[slot].rank_mb[1], dimms[slot].device_width,
			dimms[slot].ecc ? "72-bit ECC capable" : "non-ECC/parity");
	}
	if (lx_sdram_plan(dimms, DIMM_SOCKETS, &options, plan))
		die_with_post_code(POSTCODE_RAM_FAILURE, "No valid LX SDRAM plan for SPD/CMOS\n");
	for (unsigned int slot = 0; slot < DIMM_SOCKETS; slot++)
		for (unsigned int rank = 0; rank < dimms[slot].ranks; rank++)
			if (!(plan->selected & (1U << (slot * 2 + rank))))
				printk(BIOS_WARNING, "DIMM%u rank%u disabled by geometry/capacity/ECC policy\n",
					slot, rank);
	/* Set every row, including the two nonexistent rows on 3-DIMM boards. */
	for (unsigned int row = 0; row < 8; row++)
		pci_write_config8(NB, DRB + row, plan->drb[row]);
	pci_write_config16(NB, DRT, plan->drt);
	pci_write_config8(NB, DRAMC, 0);
	pci_write_config8(NB, DRAMT, plan->timing);
	/* ECC remains off throughout SDRAM commands and normal-mode entry. */
	pci_write_config8(NB, ERRCMD, pci_read_config8(NB, ERRCMD) & ~ERRCMD_MEMORY_MASK);
	pci_write_config16(NB, PACCFG, pci_read_config16(NB, PACCFG) &
		~(PACCFG_DDIM_MASK | PACCFG_ECC_TEST));
	printk(BIOS_INFO, "LX SDRAM: %u MiB, CL%u, DRAMT=%02x, selected=%02x, ECC capable=%u\n",
		plan->total_mb, plan->cas, plan->timing, plan->selected, plan->all_ecc);
}
