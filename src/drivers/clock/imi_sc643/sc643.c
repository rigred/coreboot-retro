/* SPDX-License-Identifier: GPL-2.0-only */
#include "sc643.h"

enum cb_err sc643_build_image(enum sc643_mode mode, const struct sc643_config *config,
			     uint8_t image[SC643_BYTES])
{
	unsigned int cpu, pci, sdram, ref;

	if (!config || !image)
		return CB_ERR_ARG;
	cpu = config->cpu;
	pci = config->pci;
	sdram = config->sdram;
	ref = config->ref;

	if ((unsigned int)mode > SC643_75_MHZ || !cpu || (cpu & ~SC643_CPU_MASK) ||
	    (pci & ~SC643_PCI_MASK) || !(pci & (1U << 6)) ||
	    !sdram || (sdram & ~0xfffU) || (ref & ~SC643_REF_MASK))
		return CB_ERR_ARG;
	/* Strap mode leaves software selection disabled. A warm CPU reset does
	 * not reset this chip: explicitly leave any previous software mode.
	 */
	image[0] = SC643_FTS | SC643_S2 | SC643_S1 | SC643_S0;
	if (mode == SC643_68_5_MHZ)
		image[0] = SC643_FTS | SC643_S1 | SC643_S0 | SC643_SOFTWARE;
	if (mode == SC643_75_MHZ)
		image[0] = SC643_FTS | SC643_S2 | SC643_S0 | SC643_SOFTWARE;
	image[1] = SC643_USB | SC643_SIO | SC643_NORMAL | cpu;
	image[2] = pci;
	image[3] = sdram;
	image[4] = sdram >> 8;
	image[5] = ref;
	return CB_SUCCESS;
}
