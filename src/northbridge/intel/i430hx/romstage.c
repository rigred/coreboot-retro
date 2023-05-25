/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <arch/romstage.h>
#include <cbmem.h>
#include <southbridge/intel/i82371eb/i82371eb.h>
#include <northbridge/intel/i430hx/raminit.h>

void mainboard_romstage_entry(void)
{
	edo_initialize(0);
	cbmem_initialize_empty();
}
