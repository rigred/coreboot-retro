/* SPDX-License-Identifier: GPL-2.0-only */
#include <console/console.h>
#include <device/smbus_host.h>
#include <stdbool.h>
#include "errata.h"
#include "raminit.h"

void lx_check_dimm_errata(void)
{
	/* Intel 297655-005 erratum 7 is a physical loading restriction, not a
	 * rank-allocation problem. Do not try to cure it by reducing DRBs.
	 * Preserve known-working x4/256-MiB DIMMs; the specification's withdrawal
	 * of validation is not itself evidence of a silicon addressing limit.
	 */
	if (CONFIG(SDRAM_4DIMM))
		return;
	bool mab_x32 = false, other_x8 = false, mab_mapped = false;
	for (unsigned int slot = 0; slot < DIMM_SOCKETS; slot++) {
		unsigned int address = CONFIG(SDRAM_3DIMM_REVERSE_OFFSET) ?
			DIMM0 - slot : DIMM0 + slot;
		mab_mapped |= address == CONFIG_I440LX_MAB_SPD_ADDRESS;
		int type = smbus_read_byte(address, 2);
		if (type < 0 || type == 0 || type == 0xff)
			continue;
		if (type != 4)
			continue; /* This erratum does not apply to EDO. */
		int width = smbus_read_byte(address, 13);
		if (width < 0 || (width != 4 && width != 8 && width != 16 && width != 32))
			die("LX cannot validate SDRAM component width for erratum 7\n");
		if (address == CONFIG_I440LX_MAB_SPD_ADDRESS)
			mab_x32 = width == 32;
		else
			other_x8 |= width == 8;
	}
	if (!mab_mapped)
		die("LX MAB SPD address is outside the board socket map\n");
	if (mab_x32 && other_x8)
		die("LX erratum 7: remove x32 MAB DIMM or the other x8 DIMMs\n");
}
