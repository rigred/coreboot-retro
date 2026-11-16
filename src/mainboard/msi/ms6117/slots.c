/* SPDX-License-Identifier: GPL-2.0-only */

#include <device/device.h>
#include <device/pci.h>
#include <smbios.h>
#include "routing.h"
#include "mainboard.h"

static enum misc_slot_usage slot_usage(unsigned int slot)
{
	struct device *dev = pcidev_path_on_root(PCI_DEVFN(slot, 0));

	return dev && dev->enabled ? SlotUsageInUse : SlotUsageAvailable;
}

static int write_isa_slot(unsigned long *current, int *handle, const char *name, u16 id)
{
	struct smbios_type9 *slot = (void *)*current;
	int len = smbios_write_type9(current, handle, name, SlotTypeIsa,
		SlotDataBusWidth16Bit, SlotUsageUnknown, SlotLengthUnknown, id,
		1, 0, 0xff, 0xff);

	/* The generic writer defaults to PCI segment 0. ISA has no PCI address. */
	slot->segment_group_number = 0xffff;
	return len;
}

int ms6117_smbios_slots(unsigned long *current, int *handle)
{
	int len = 0;
	struct device *agp = pcidev_path_on_root(PCI_DEVFN(1, 0));
	enum misc_slot_usage usage = SlotUsageUnknown;
	u8 agp_bus = 0xff, agp_devfn = 0xff;

#define ROUTE(d, s, r) \
	if (s) \
		len += smbios_write_type9(current, handle, "PCI Slot " #s, SlotTypePci, \
			SlotDataBusWidth32Bit, slot_usage(d), SlotLengthUnknown, s, \
			1, 0, 0, PCI_DEVFN(d, 0));
	MS6117_ROUTES(ROUTE)
#undef ROUTE
	/* Presence of the bridge itself does not mean an AGP card is installed. */
	if (agp && agp->enabled) {
		usage = SlotUsageAvailable;
		for (struct bus *bus = agp->link_list; bus; bus = bus->next)
			for (struct device *dev = bus->children; dev; dev = dev->sibling)
				if (dev->enabled && dev->path.type == DEVICE_PATH_PCI) {
					usage = SlotUsageInUse;
					agp_bus = bus->secondary;
					agp_devfn = dev->path.pci.devfn;
				}
	}
	len += smbios_write_type9(current, handle, "AGP", SlotTypeAgp,
		SlotDataBusWidth32Bit, usage, SlotLengthUnknown, 0,
		1, 0, agp_bus, agp_devfn);
	/* ISA card occupancy and bus/device/function are not discoverable. */
	len += write_isa_slot(current, handle, "ISA Slot 1", 1);
	len += write_isa_slot(current, handle, "ISA Slot 2", 2);
	return len;
}
