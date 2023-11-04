/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <bootblock_common.h>

 
#include <superio/ite/common/ite.h>
#include <superio/ite/it8679f/it8679f.h>

#define SERIAL_DEV PNP_DEV(0x3f0, IT8679F_SP1)



void bootblock_mainboard_early_init(void)
{
	 //ite_enable_serial(SERIAL_DEV, CONFIG_TTYS0_BASE);
}
