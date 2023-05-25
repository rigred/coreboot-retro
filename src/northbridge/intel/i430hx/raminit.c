/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <delay.h>
#include <stdint.h>
#include <device/mmio.h>
#include <device/pci_ops.h>
#include <device/pci_def.h>
#include <console/console.h>
#include <commonlib/console/post_codes.h>
#include <timestamp.h>
#include "i430hx.h"
#include "raminit.h"

/*
 * Macros and definitions
 */

/* Debugging macros. */
#if CONFIG(DEBUG_RAM_SETUP)
#define PRINT_DEBUG(x...)	printk(BIOS_DEBUG, x)
#define DUMPNORTH()		dump_pci_device(NB)
#else
#define PRINT_DEBUG(x...)
#define DUMPNORTH()
#endif
