/* SPDX-License-Identifier: GPL-2.0-only */
#include <console/console.h>
#include <delay.h>
#include <device/pci_def.h>
#include <device/pci_ops.h>
#include <halt.h>
#include <string.h>
#include "errata.h"
#include "i440lx.h"

/* PIIX4 function 0 has a hardwired BME bit. Its TOM register, rather than
 * PCI_COMMAND, gates ISA/DMA writes to the legacy VGA range (290562-001 4.1.12).
 */
#define PIIX4_TOM 0x69
#define PIIX4_TOM_VGA (1 << 2)

void lx_smram_close(void)
{
	uint8_t value = pci_s_read_config8(NB, SMRAM) & ~LX_SMRAM_DOPEN;
	pci_s_write_config8(NB, SMRAM, value);
	/* If closing fails, avoid a possibly graphical error console. */
	if (pci_s_read_config8(NB, SMRAM) & LX_SMRAM_DOPEN)
		halt();
}

void lx_smram_open(struct lx_smram_guard *g, pci_devfn_t isa)
{
	lx_smram_close();
	if (g->active || (pci_s_read_config8(NB, SMRAM) & LX_SMRAM_DLCK))
		die("LX SMRAM guard is nested or SMRAM is locked\n");
	if ((isa & ~PCI_DEV(0, 31, 0)) ||
	    pci_s_read_config32(isa, PCI_VENDOR_ID) != 0x71108086)
		die("LX SMRAM guard requires primary-bus PIIX4 ISA function\n");
	memset(g->masters, 0, sizeof(g->masters));
	g->isa = isa;
	g->tom = pci_s_read_config8(isa, PIIX4_TOM);
	pci_s_write_config8(isa, PIIX4_TOM, g->tom & ~PIIX4_TOM_VGA);
	if (pci_s_read_config8(isa, PIIX4_TOM) & PIIX4_TOM_VGA)
		die("LX could not block ISA/DMA VGA forwarding\n");

	/* Add-in masters cannot be described at compile time. This is a bounded
	 * primary-bus safety sweep, not discovery of the southbridge address.
	 * Also gates upstream traffic from PCI bridges; no secondary-bus scan.
	 * PAC's own two functions have hardwired command bits and are not external
	 * primary-PCI initiators. Do not attempt to change those command bits.
	 */
	for (unsigned int fn = 0; fn < 256; fn++) {
		pci_devfn_t dev = fn << 12;
		if (dev == NB || dev == NBP || dev == isa ||
		    pci_s_read_config16(dev, PCI_VENDOR_ID) == 0xffff)
			continue;
		uint16_t command = pci_s_read_config16(dev, PCI_COMMAND);
		if (!(command & PCI_COMMAND_MASTER))
			continue;
		pci_s_write_config16(dev, PCI_COMMAND, command & ~PCI_COMMAND_MASTER);
		if (pci_s_read_config16(dev, PCI_COMMAND) & PCI_COMMAND_MASTER)
			die("LX could not quiesce a primary PCI master\n");
		g->masters[fn / 8] |= 1 << (fn % 8);
	}
	/* Boot-only callers have not submitted DMA. Allow a final in-flight PCI
	 * transaction to finish after command readbacks, before exposing DRAM.
	 */
	udelay(10);
	g->active = 1;
	pci_s_write_config8(NB, SMRAM, LX_SMRAM_ASEG_OPEN);
	if (pci_s_read_config8(NB, SMRAM) != LX_SMRAM_ASEG_OPEN) {
		lx_smram_close();
		die("LX SMRAM open failed\n");
	}
}

void lx_smram_end(struct lx_smram_guard *g)
{
	/* Close and read back BEFORE restoring any initiator's permissions. */
	lx_smram_close();
	if (!g->active)
		die("LX SMRAM guard was not active\n");
	pci_s_write_config8(g->isa, PIIX4_TOM, g->tom);
	if (pci_s_read_config8(g->isa, PIIX4_TOM) != g->tom)
		die("LX ISA/DMA forwarding restore failed\n");
	for (unsigned int fn = 0; fn < 256; fn++) {
		if (!(g->masters[fn / 8] & (1 << (fn % 8))))
			continue;
		pci_devfn_t dev = fn << 12;
		pci_s_write_config16(dev, PCI_COMMAND,
			pci_s_read_config16(dev, PCI_COMMAND) | PCI_COMMAND_MASTER);
		if (!(pci_s_read_config16(dev, PCI_COMMAND) & PCI_COMMAND_MASTER))
			die("LX PCI master restore failed\n");
	}
	g->active = 0;
}
