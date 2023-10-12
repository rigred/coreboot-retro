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
	
	/* const pci_devfn_t px73 = PCI_DEV(0, 7, 3); */
	
	
}



struct chip_operations mainboard_ops = {
	.init = mainboard_init
};
