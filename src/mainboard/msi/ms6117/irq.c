/* SPDX-License-Identifier: GPL-2.0-only */

#include <bootstate.h>
#include <console/console.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ops.h>
#include <pc80/i8259.h>
#include "routing.h"

static void ms6117_irq_init(void *unused)
{
	static const u8 priority[] = { 10, 11, 5 };
	u8 pool[ARRAY_SIZE(priority)], irq[4];
	unsigned int count = 0;
	struct device *isa = pcidev_path_on_root(PCI_DEVFN(7, 0));

	if (!isa || !isa->enabled || pci_read_config32(isa, PCI_VENDOR_ID) != 0x71108086)
		die("MS-6117: expected PIIX4 at 00:07.0\n");

	for (size_t i = 0; i < ARRAY_SIZE(priority); i++)
		if (MS6117_IRQ_MASK & (1 << priority[i]))
			pool[count++] = priority[i];
	for (size_t i = 0; i < ARRAY_SIZE(irq); i++) {
		irq[i] = pool[i % count];
		pci_write_config8(isa, 0x60 + i, irq[i]);
		if (pci_read_config8(isa, 0x60 + i) != irq[i])
			die("MS-6117: PIRQ programming failed\n");
		i8259_configure_irq_trigger(irq[i], IRQ_LEVEL_TRIGGERED);
	}
	i8259_configure_irq_trigger(9, IRQ_LEVEL_TRIGGERED);

	/* Reuse the PCI core's bridge swizzling, including multifunction cards. */
	for (struct device *dev = all_devices; dev; dev = dev->next) {
		struct device *root = NULL;
		int rotation, pin;

		if (!dev->enabled || dev->path.type != DEVICE_PATH_PCI)
			continue;
		if (!dev->bus->secondary) {
			/* SCI is independent of the four PIRQ links. */
			if (dev->path.pci.devfn == PCI_DEVFN(7, 3)) {
				pci_write_config8(dev, PCI_INTERRUPT_LINE, 9);
				continue;
			}
			/* Integrated IDE stays on compatibility IRQs 14 and 15. */
			if (dev->path.pci.devfn == PCI_DEVFN(7, 1))
				continue;
		}
		pin = get_pci_irq_pins(dev, &root);

		if (pin < 1 || pin > 4 || !root || root->bus->secondary)
			continue;
		switch (PCI_SLOT(root->path.pci.devfn)) {
#define ROUTE(d, s, r) case d: rotation = r; break;
		MS6117_ROUTES(ROUTE)
#undef ROUTE
		default:
			printk(BIOS_WARNING, "MS-6117: no IRQ wiring for %s\n", dev_path(dev));
			continue;
		}
		pci_write_config8(dev, PCI_INTERRUPT_LINE, irq[(rotation + pin - 1) & 3]);
	}
}

/* Run after southbridge/device initialization, before payload table creation. */
BOOT_STATE_INIT_ENTRY(BS_DEV_INIT, BS_ON_EXIT, ms6117_irq_init, NULL);
