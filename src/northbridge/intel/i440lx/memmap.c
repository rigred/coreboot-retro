/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/romstage.h>
#include <device/pci_ops.h>
#include <cbmem.h>
#include <commonlib/helpers.h>
#include <cpu/x86/mtrr.h>
#include <program_loading.h>
#include "i440lx.h"

uintptr_t cbmem_top_chipset(void)
{
	/* Base of TSEG is top of usable DRAM */
	/*
	 * SMRAM - System Management RAM Control Register
	 * 0x72
	 * [7:4] Not relevant to this function.
	 * [3:3] Global SMRAM Enable (G_SMRAME)
	 * [2:0] Hardwired to 010.
	 *
	 */
	uintptr_t tom = pci_read_config8(NB, DRB7) * 8 * MiB;

	int gsmrame = pci_read_config8(NB, SMRAM) & 0x8;
	if (gsmrame) {
		/* Adjust tom based on TSEG size for the 440LX chipset (no ESMRAMC) */
		int tseg_size = 128 * KiB;
		printk(BIOS_DEBUG, "Setting TSEG size to %ld\n", tseg_size);
		tom -= tseg_size;
	}
	printk(BIOS_DEBUG, "Setting TOM size to %ld\n", tom);
	return tom;
}

void fill_postcar_frame(struct postcar_frame *pcf)
{
	uintptr_t top_of_ram;

	/* Cache CBMEM region as WB. */
	top_of_ram = (uintptr_t)cbmem_top();
	postcar_frame_add_mtrr(pcf, top_of_ram - 8*MiB, 8*MiB,
		MTRR_TYPE_WRBACK);

}
