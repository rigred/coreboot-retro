/* SPDX-License-Identifier: GPL-2.0-only */
#include <arch/io.h>
#include <arch/bootblock.h>
#include <device/pci_ops.h>
#include <halt.h>
#include "i440lx.h"

/* Called by the Slot-1 CAR bootblock, before console/romstage/PCI enumeration. */
void bootblock_early_northbridge_init(void)
{
	/* Intel 297655-005 erratum 5; datasheet 290564-002 sections 3.3.27/4.8.7.
	 * BIST changes BOTH CPU and PAC IOQ depth to one. There is no documented
	 * LX equivalent of the BX IOQD bit: perform the required PIIX4 hard reset.
	 * Do not use system_reset(), which flushes the live CAR stack first.
	 */
	uint8_t reset = pci_s_read_config8(NB, LX_RSTCTRL);
	if (reset & LX_RSTCTRL_BIST) {
		pci_s_write_config8(NB, LX_RSTCTRL,
			reset & ~(LX_RSTCTRL_BIST | LX_RSTCTRL_RCPU | LX_RSTCTRL_CSRE));
		if (pci_s_read_config8(NB, LX_RSTCTRL) & LX_RSTCTRL_BIST)
			halt();
		outb(0x02, 0xcf9);
		outb(0x06, 0xcf9);
		halt();
	}

	/* Erratum 6: never carry DOPEN into normal firmware execution. Keep
	 * the segment, enable and lock state; do not lock before SMM installation.
	 */
	uint8_t smram = pci_s_read_config8(NB, SMRAM);
	pci_s_write_config8(NB, SMRAM, smram & ~LX_SMRAM_DOPEN);
	if (pci_s_read_config8(NB, SMRAM) & LX_SMRAM_DOPEN)
		halt();
}
