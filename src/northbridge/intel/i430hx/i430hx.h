/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef NORTHBRIDGE_INTEL_I430HX_I430HX_H
#define NORTHBRIDGE_INTEL_I430HX_I430HX_H

/*
 * Host-to-PCI Bridge Registers.
 * The values in parenthesis are the default values as per datasheet.
 * Any addresses between 0x00 and 0xff not listed below are either
 * Reserved or Intel Reserved and should not be touched.
 */

#define NBXCFG  0x50 /* 430HX Configuration (0x00). */
#define CCFG    0x52 /* Cache Control (0x00:SSSS0010b) */
#define DRAMEC  0x56 /* DRAM Extended Control (0x00) */
#define DRAMC   0x57 /* DRAM Control (0x01) */
#define DRAMT   0x58 /* DRAM Timing (0x00) */
#define PAM0    0x59 /* Programmable Attribute Map 0 */
#define PAM1    0x5A /* Programmable Attribute Map 1 */
#define PAM2    0x5B /* Programmable Attribute Map 2 */
#define PAM3    0x5C /* Programmable Attribute Map 3 */
#define PAM4    0x5D /* Programmable Attribute Map 4 */
#define PAM5    0x5E /* Programmable Attribute Map 5 */
#define PAM6    0x5F /* Programmable Attribute Map 6 */
#define DRB0    0x60 /* DRAM Row Boundary 0 (0x02) */
#define DRB1    0x61 /* DRAM Row Boundary 1 (0x02) */
#define DRB2    0x62 /* DRAM Row Boundary 2 (0x02) */
#define DRB3    0x63 /* DRAM Row Boundary 3 (0x02) */
#define DRB4    0x64 /* DRAM Row Boundary 4 (0x02) */
#define DRB5    0x65 /* DRAM Row Boundary 5 (0x02) */
#define DRB6    0x66 /* DRAM Row Boundary 6 (0x02) */
#define DRB7    0x67 /* DRAM Row Boundary 7 (0x02) */
#define DRT     0x68 /* DRAM Row Type (0x00) */
#define DRAT    0x69 /* DRAM Row Attribute (Intel forgot to define this in their document) */
#define SMRAM   0x72 /* System Management RAM Control (0x02:_0000010b) */
#define ERRCMD  0x90 /* Error Command (0x00:0____000b) */
#define ERRSTS  0x91 /* Error Status (0x00) */
#define ERRSYN  0x92 /* Error Syndrome (0x00:00000000b) RO */

#define NB PCI_DEV(0, 0, 0)

#endif /* NORTHBRIDGE_INTEL_I430HX_I430HX_H */