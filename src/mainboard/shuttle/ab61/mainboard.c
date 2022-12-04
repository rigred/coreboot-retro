/* SPDX-License-Identifier: GPL-2.0-only */

#include <device/pci_ops.h>
#include <device/pci.h>
#include <device/pci_def.h>
#include <device/pci_ids.h>
#include <console/console.h>
#include <southbridge/intel/i82371eb/i82371eb.h>

/**
 * Mainboard specific enables.
 *
 * @param chip_info Ignored
 */


static void mainboard_init(void *chip_info)
{
	
	const pci_devfn_t px73 = PCI_DEV(0, 7, 3);
	
	/*
	 * Set up an 8-byte generic I/O decode block at device 9.
	 * This will be for W83781D hardware monitor.
	 * Port 0x290 mask 0x007
	 *
	 * This should enable access to W83781D over the ISA bus.
	 */
	
	/* 
	reg = pci_s_read_config32(px43, DEVRESB);
	reg |= (0x290 | (0xe7 << 16));
	pci_s_write_config32(px43, DEVRESB, reg);
	*/

	u16 reg;

	reg = pci_s_read_config16(px73, XBCS);
	printk(BIOS_DEBUG, "XBCS: %u", reg);
}



struct chip_operations mainboard_ops = {
	.init = mainboard_init
};
