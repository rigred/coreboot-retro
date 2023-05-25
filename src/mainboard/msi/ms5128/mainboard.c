/* SPDX-License-Identifier: GPL-2.0-only */

#include <device/pci_ops.h>
#include <southbridge/intel/i82371eb/i82371eb.h>

/**
 * Mainboard specific enables.
 *
 * @param chip_info Ignored
 */
static void mainboard_init(void *chip_info)
{

}

struct chip_operations mainboard_ops = {
	.init = mainboard_init
};
