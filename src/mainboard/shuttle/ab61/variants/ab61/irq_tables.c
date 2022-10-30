/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <arch/pirq_routing.h>
#include <console/console.h>
#include <device/pci_def.h>
#include <string.h>

static void write_pirq_info(struct irq_info *pirq_info, u8 bus, u8 devfn,
			    u8 link0, u16 bitmap0, u8 link1, u16 bitmap1,
			    u8 link2, u16 bitmap2, u8 link3, u16 bitmap3, u8 slot, u8 rfu)
{
	pirq_info->bus = bus;
	pirq_info->devfn = devfn;
	pirq_info->irq[0].link = link0;
	pirq_info->irq[0].bitmap = bitmap0;
	pirq_info->irq[1].link = link1;
	pirq_info->irq[1].bitmap = bitmap1;
	pirq_info->irq[2].link = link2;
	pirq_info->irq[2].bitmap = bitmap2;
	pirq_info->irq[3].link = link3;
	pirq_info->irq[3].bitmap = bitmap3;
	pirq_info->slot = slot;
	pirq_info->rfu = rfu;
}

unsigned long write_pirq_routing_table(unsigned long addr)
{
	struct irq_routing_table *pirq;
	struct irq_info *pirq_info;
	u32 slot_num;

	u8 sum = 0;
	int i;

	/* Align the table to be 16 byte aligned. */
	addr = ALIGN_UP(addr, 16);

	/* This table must be between 0xf0000 & 0x100000 */
	printk(BIOS_INFO, "Writing IRQ routing tables to 0x%lx...", addr);

	pirq = (void *)(addr);

	pirq->signature = PIRQ_SIGNATURE;
	pirq->version = PIRQ_VERSION;

	pirq->rtr_bus = 0;
	pirq->rtr_devfn = PCI_DEVFN(0x07, 0);

	pirq->exclusive_irqs = 0x0c80;

	pirq->rtr_vendor = 0x8086;
	pirq->rtr_device = 0x7000;

	pirq->miniport_data = 0;

	memset(pirq->rfu, 0, sizeof(pirq->rfu));

	pirq_info = (void *)(&pirq->slots);
	slot_num = 0;

	/* pci slot 1 */
	write_pirq_info(pirq_info, 0, PCI_DEVFN(0x09, 0),
			0x60, 0xdeb8, 0x61, 0xdeb8, 0x62, 0xdeb8, 0x63, 0xdeb8, 1, 0);
	pirq_info++;
	slot_num++;

	/* pci slot 2 */
	write_pirq_info(pirq_info, 0, PCI_DEVFN(0x0a, 0),
			0x61, 0xdeb8, 0x62, 0xdeb8, 0x63, 0xdeb8, 0x60, 0xdeb8, 2, 0);
	pirq_info++;
	slot_num++;

	/* pci slot 3 */
	write_pirq_info(pirq_info, 0, PCI_DEVFN(0x0b, 0),
			0x62, 0xdeb8, 0x63, 0xdeb8, 0x60, 0xdeb8, 0x61, 0xdeb8, 3, 0);
	pirq_info++;
	slot_num++;

	/* pci slot 4 */
	write_pirq_info(pirq_info, 0, PCI_DEVFN(0x0c, 0),
			0x63, 0xdeb8, 0x60, 0xdeb8, 0x61, 0xdeb8, 0x62, 0xdeb8, 4, 0);
	pirq_info++;
	slot_num++;

	/* pci slot 5 */
	write_pirq_info(pirq_info, 0, PCI_DEVFN(0x0c, 0),
			0x62, 0xdeb8, 0x63, 0xdeb8, 0x60, 0xdeb8, 0x61, 0xdeb8, 5, 0);
	pirq_info++;
	slot_num++;

	/* 82371AB/EB/MB PIIX4/E/M EIDE Controller */
	write_pirq_info(pirq_info, 0, PCI_DEVFN(0x07, 1),
			0x60, 0xdeb8, 0x61, 0xdeb8, 0x62, 0xdeb8, 0x63, 0xdeb8, 0, 0);
	pirq_info++;
	slot_num++;

	/* 82443BX/ZX 440BX/ZX PCI to AGP Bridge  */
	write_pirq_info(pirq_info, 0, PCI_DEVFN(0x01, 0),
			0x60, 0xdeb8, 0x61, 0xdeb8, 0x62, 0xdeb8, 0x63, 0xdeb8, 0, 0);
	pirq_info++;
	slot_num++;

	pirq->size = 32 + 16 * slot_num;

	{
		const u8 *const v = (u8 *)(pirq);
		for (i = 0; i < pirq->size; i++)
			sum += v[i];
	}

	sum = pirq->checksum - sum;

	if (sum != pirq->checksum)
		pirq->checksum = sum;

	printk(BIOS_INFO, "%s done.\n", __func__);

	return (unsigned long)pirq_info;
}


// static const struct irq_routing_table intel_irq_routing_table = {
// 	PIRQ_SIGNATURE,
// 	PIRQ_VERSION,
// 	32 + 16 * CONFIG_IRQ_SLOT_COUNT,/* Max. number of devices on the bus */
// 	0x00,			/* Interrupt router bus */
// 	(0x04 << 3) | 0x0,	/* Interrupt router device */
// 	0,			/* IRQs devoted exclusively to PCI usage */
// 	0x8086,			/* Vendor */
// 	0x7000,			/* Device */
// 	0,			/* Miniport data */
// 	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* u8 rfu[11] */
// 	0x54,			/* Checksum */
// 	/* clang-format off */
// 	{
// 		/* bus,       dev|fn,   {link, bitmap}, {link, bitmap}, {link, bitmap}, {link, bitmap},  slot, rfu */
// 		{0x00,(0x09 << 3)|0x0, {{0x60, 0x1eb8}, {0x61, 0x1eb8}, {0x62, 0x1eb8}, {0x63, 0x01eb8}}, 0x1, 0x0},
// 		{0x00,(0x0a << 3)|0x0, {{0x61, 0x1eb8}, {0x62, 0x1eb8}, {0x63, 0x1eb8}, {0x60, 0x01eb8}}, 0x2, 0x0},
// 		{0x00,(0x0b << 3)|0x0, {{0x62, 0x1eb8}, {0x63, 0x1eb8}, {0x60, 0x1eb8}, {0x61, 0x01eb8}}, 0x3, 0x0},
// 		{0x00,(0x0c << 3)|0x0, {{0x63, 0x1eb8}, {0x60, 0x1eb8}, {0x61, 0x1eb8}, {0x62, 0x01eb8}}, 0x4, 0x0},
// 		{0x00,(0x0d << 3)|0x0, {{0x62, 0x1eb8}, {0x63, 0x1eb8}, {0x60, 0x1eb8}, {0x61, 0x01eb8}}, 0x5, 0x0},
// 		{0x00,(0x07 << 3)|0x0, {{0x60, 0x1eb8}, {0x61, 0x1eb8}, {0x62, 0x1eb8}, {0x63, 0x01eb8}}, 0x0, 0x0},
// 		{0x00,(0x01 << 3)|0x0, {{0x60, 0x1eb8}, {0x61, 0x1eb8}, {0x62, 0x1eb8}, {0x63, 0x01eb8}}, 0x0, 0x0},
// 	}
// 	/* clang-format on */
// };

// unsigned long write_pirq_routing_table(unsigned long addr)
// {
// 	return copy_pirq_routing_table(addr, &intel_irq_routing_table);
// }
