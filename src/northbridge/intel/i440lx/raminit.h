/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef RAMINIT_H
#define RAMINIT_H

/* The 440LX supports 4 or 3 DIMM's in differing configurations */
#if CONFIG(SDRAM_4DIMM)
	#define DIMM_SOCKETS	4

	/* DIMM SPD addresses */
	#define DIMM0		0x50
	#define DIMM1		0x51
	#define DIMM2		0x52
	#define DIMM3		0x53
#elif CONFIG(SDRAM_3DIMM)
	#define DIMM_SOCKETS	3

	/* DIMM SPD addresses */
	#define DIMM0		0x50
	#define DIMM1		0x51
	#define DIMM2		0x52
#elif CONFIG(SDRAM_3DIMM_REVERSE_OFFSET)
	#define DIMM_SOCKETS	3

	/* DIMM SPD addresses */
	#define DIMM0		0x52
	#define DIMM1		0x51
	#define DIMM2		0x50
#endif 

void enable_spd(void);
void sdram_initialize(int s3resume);
struct lx_sdram_plan;
void lx_spd_configure(struct lx_sdram_plan *plan, unsigned int ecc_mode, int cmos_valid);
void lx_sdram_finish(const struct lx_sdram_plan *plan, unsigned int ecc_mode);

/* Debug */
#if CONFIG(DEBUG_RAM_SETUP)
void dump_spd_registers(void);
void dump_pci_device(unsigned int dev);
#else
#define dump_spd_registers()
#endif
#endif				/* RAMINIT_H */
