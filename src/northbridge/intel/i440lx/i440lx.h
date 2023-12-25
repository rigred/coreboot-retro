/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef NORTHBRIDGE_INTEL_I440LX_I440LX_H
#define NORTHBRIDGE_INTEL_I440LX_I440LX_H

/*
 * Host-to-PCI Bridge Registers.
 * The values in parenthesis are the default values as per datasheet.
 * Any addresses between 0x00 and 0xff not listed below are either
 * Reserved or Intel Reserved and should not be touched.
 */

#define APBASE  0x10 /* Aperture Base Address (0x10->0x13) (0x00000008) */
#define PACCFG  0x50 /* 440LX Configuration (0x0000:0s00_s000_0000_0s00b). */
#define DBC		0x53 /* Data Buffer control register */
#define DRT		0x55 /* DRAM Row Type on 440LX, is part of [4:3] DRAMC on 440BX*/
#define DRAMC   0x57 /* DRAM Control (0x01). */
#define DRAMT   0x58 /* DRAM Timing (0x00). */
#define PAM     0x59 /* Programmable Attribute Map, 7 registers (0x00). */
#define PAM0    0x59
#define PAM1    0x5a
#define PAM2    0x5b
#define PAM3    0x5c
#define PAM4    0x5d
#define PAM5    0x5e
#define PAM6    0x5f
#define DRB     0x60 /* DRAM Row Boundary, 8 registers (0x01). */
#define DRB0    0x60
#define DRB1    0x61
#define DRB2    0x62
#define DRB3    0x63
#define DRB4    0x64
#define DRB5    0x65
#define DRB6    0x66
#define DRB7    0x67
#define FDHC    0x68 /* Fixed SDRAM Hole Control (0x00). */
#define DRAMXC	0x6A /* DRAM Extended Mode Select [6A-6B] (0x0000) */
#define MBSC    0x6C /* Memory Buffer Strength Control (0x55555555). */
#define MTT		0x70 /* Multi Transaction Timer Register */
#define SMRAM   0x72 /* System Management RAM Control (0x02). */

#define ERRCMD  0x90 /* Error Command Register (0x00). */
#define ERRSTS  0x91 /* Error Status (0x0000). */
// TODO: AGP stuff.
#define ACAPID  0xa0 /* AGP Capability Identifier (0x00100002 or 0x00000000) */
#define AGPSTAT 0xa4 /* AGP Status Register (0x1f000203, read only) */
#define AGPCMD  0xa8 /* AGP Command Register (0x00000000) */
#define AGPCTRL 0xb0 /* AGP Control Register (0x00000000) */
#define APSIZE  0xb4 /* Aperture Size Control Register (0x00) */
#define ATTBASE 0xb8 /* Aperture Translation Table (0x00000000) */
#define AMTT	0xbc /* AGP MTT Control Register */
#define LPTT	0xbd /* AGP Low Priority transaction timer register*/

#define NB PCI_DEV(0, 0, 0)
#define NBP PCI_DEV(0, 1, 0)

enum agp_aperture {
	AGP_APERTURE_4MB = 0,
	AGP_APERTURE_8MB = 1,
	AGP_APERTURE_16MB = 2,
	AGP_APERTURE_32MB = 3,
	AGP_APERTURE_64MB = 4,
	AGP_APERTURE_128MB = 5,
	AGP_APERTURE_256MB = 6
};

#define AGP_APERTURE_DEFAULT AGP_APERTURE_64MB

#endif /* NORTHBRIDGE_INTEL_I440LX_I440LX_H */
