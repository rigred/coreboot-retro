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
	/* 440LX SMRAM uses the A0000h or C0000h legacy window, not TSEG. */
	uintptr_t tom = pci_read_config8(NB, DRB7) * 8 * MiB;
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
