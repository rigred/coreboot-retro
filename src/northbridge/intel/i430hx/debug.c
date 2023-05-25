/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <device/pci_ops.h>
#include "raminit.h"

void dump_pci_device(unsigned int dev)
{
	int i;
	printk(BIOS_DEBUG, "PCI: %02x:%02x.%02x", (dev >> 20) & 0xff, (dev >> 15) & 0x1f,
		(dev >> 12) & 7);

	for (i = 0; i <= 255; i++) {
		if ((i & 0x0f) == 0)
			printk(BIOS_DEBUG, "\n%02x:", i);
		printk(BIOS_DEBUG, " %02x", pci_read_config8(dev, i));
	}
	printk(BIOS_DEBUG, "\n");
}