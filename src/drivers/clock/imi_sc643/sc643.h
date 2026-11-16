/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef DRIVERS_CLOCK_IMI_SC643_H
#define DRIVERS_CLOCK_IMI_SC643_H
#include <types.h>

/* IMI SC643 rev 1.6, pp. 1, 5-7. Write-only address, NOT a readable SPD. */
#define SC643_ADDRESS 0x69
#define SC643_BYTES 6
#define SC643_FTS (1U << 7)
#define SC643_S2 (1U << 6)
#define SC643_S1 (1U << 5)
#define SC643_S0 (1U << 4)
#define SC643_SOFTWARE (1U << 3)
#define SC643_NORMAL (1U << 5) /* byte 1: zero enters test mode */
#define SC643_USB (1U << 7)    /* byte 1: fixed 48 MHz */
#define SC643_SIO (1U << 6)    /* byte 1: fixed 24 MHz */
#define SC643_CPU_MASK 0x0f   /* byte 1 */
#define SC643_PCI_MASK 0x7f   /* byte 2: bit 6 = PCI_F */
#define SC643_SDRAM_LOW_MASK 0xff /* byte 3 */
#define SC643_SDRAM_HIGH_MASK 0x0f /* byte 4 */
#define SC643_REF_MASK 0x13   /* byte 5: IOAPIC, REF1, REF0 */

enum sc643_mode {
	SC643_HARDWARE_STRAPS,
	SC643_68_5_MHZ,
	SC643_75_MHZ,
};

struct sc643_config {
	uint16_t sdram;
	uint8_t cpu;
	uint8_t pci;
	uint8_t ref;
};

/* Pure image builder: no bus/CMOS/board policy and no register readback.
 * Never chooses test, tri-state or reserved frequency encodings.
 */
enum cb_err sc643_build_image(enum sc643_mode mode, const struct sc643_config *config,
			     uint8_t image[SC643_BYTES]);

#endif /* DRIVERS_CLOCK_IMI_SC643_H */
