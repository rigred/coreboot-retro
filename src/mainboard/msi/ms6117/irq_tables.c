/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/pirq_routing.h>
#include <device/pci_def.h>
#include <string.h>
#include "routing.h"

_Static_assert(CONFIG_IRQ_SLOT_COUNT == 7, "MS-6117 has seven PIRQ entries");

/* PIIX link values are PCI configuration offsets, not generic link indices. */
#define LINK(r, p) { 0x60 + (((r) + (p)) & 3), MS6117_IRQ_MASK }
#define ROUTE(d, s, r) { 0, PCI_DEVFN(d, 0), \
	{ LINK(r, 0), LINK(r, 1), LINK(r, 2), LINK(r, 3) }, s, 0 },
static const struct irq_routing_table routing_table = {
	.signature = PIRQ_SIGNATURE,
	.version = PIRQ_VERSION,
	.size = sizeof(struct irq_routing_table),
	.rtr_bus = 0,
	.rtr_devfn = PCI_DEVFN(7, 0),
	.rtr_vendor = 0x8086,
	.rtr_device = 0x7110,
	.slots = { MS6117_ROUTES(ROUTE) },
};
#undef ROUTE
#undef LINK

unsigned long write_pirq_routing_table(unsigned long addr)
{
	struct irq_routing_table *table;
	u8 sum = 0;

	/* Do not call generic pirq_route_irqs(): its link indexing differs. */
	addr = ALIGN_UP(addr, 16);
	table = (void *)addr;
	memcpy(table, &routing_table, sizeof(*table));
	for (size_t i = 0; i < sizeof(*table); i++)
		sum += ((const u8 *)table)[i];
	table->checksum = -sum;
	return addr + sizeof(*table);
}
