/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/romstage.h>
#include <device/pci_ops.h>
#include <cbmem.h>
#include <console/console.h>
#include <commonlib/helpers.h>
#include <cpu/x86/mtrr.h>
#include <program_loading.h>
#include "i430hx.h"

uintptr_t cbmem_top_chipset(void)
{
	/* Base of TSEG is top of usable DRAM */
	/*
	 * SMRAM - System Management RAM Control Register
	 * 0x72
	 * [7:4] Not relevant to this function.
	 * [3:3] Global SMRAM Enable (SMRAME)
	 * [2:0] Hardwired to 010.
	 *
	 * Source: 430HX documentation.
	 */
	uintptr_t tom = pci_read_config8(NB, DRB7) * 4 * MiB;

	int smrame = pci_read_config8(NB, SMRAM) & 0x8;
	uintptr_t smram_base = 0xA0000;
	uintptr_t smram_end = 0xBFFFF;
	/* DOPEN, DCLS, DCLK, SMRAME, DBASESEG bits */
	uint8_t smram = pci_read_config8(NB, SMRAM);

	// Calculate SMRAM size based on TSEG Size (DBASESEG)
	uintptr_t smram_size = 0;
	switch (smram & 0x7) {
		case 0x2:
			smram_size = 128 * KiB;
			break;
		default:
			// Invalid or unrecognized DBASESEG value
			// Handle the error or default case accordingly
			printk(BIOS_DEBUG, "Invalid DBASESEG Register: %02x:", smram & 0x7);
			break;
	}

	// Adjust TOM based on SMRAM settings
	if ((smrame >> 3) & 0x1) {
		// High SMRAM space enabled
		tom -= smram_size;
	} else if ((smram >> 1) & 0x1) {
		// Compatible SMRAM space enabled
		tom -= smram_end - smram_base + 1;
	}

	return tom;
}

void fill_postcar_frame(struct postcar_frame *pcf)
{
	uintptr_t top_of_ram;

	/* Cache CBMEM region as WB. */
	top_of_ram = (uintptr_t)cbmem_top();
	postcar_frame_add_mtrr(pcf, top_of_ram - 4*MiB, 4*MiB,
		MTRR_TYPE_WRBACK);

}
