/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef MAINBOARD_MS6117_ROUTING_H
#define MAINBOARD_MS6117_ROUTING_H

/* AMI and Award $PIR + DSDT agree: device, physical slot, INTA rotation. */
#define MS6117_ROUTES(X) \
	X(0x0e, 1, 0) \
	X(0x0f, 2, 1) \
	X(0x10, 3, 2) \
	X(0x11, 4, 3) \
	X(0x12, 5, 0) \
	X(0x07, 0, 0) \
	X(0x01, 0, 0)

#define MS6117_IRQ_MASK CONFIG_MS6117_PCI_IRQ_MASK
#if !MS6117_IRQ_MASK || (MS6117_IRQ_MASK & ~0x0c20)
#error "MS-6117 PCI IRQ pool must be a nonempty subset of IRQs 5, 10 and 11"
#endif

#endif
