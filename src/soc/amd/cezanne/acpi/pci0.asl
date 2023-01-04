/* SPDX-License-Identifier: GPL-2.0-only */

Device(PCI0) {
	Name(_HID, EISAID("PNP0A08"))	/* PCI Express Root Bridge */
	Name(_CID, EISAID("PNP0A03"))	/* PCI Root Bridge */
	External(TOM1, IntObj)		/* Generated by root_complex.c */

	Method(_BBN, 0, NotSerialized) {
		Return(0) /* Bus number = 0 */
	}

	Method(_STA, 0, NotSerialized) {
		Return(0x0B)	/* Status is visible */
	}

	/* Operating System Capabilities Method */
	Method(_OSC, 4) {
		CreateDWordField(Arg3, 0, CDW1) /* Capabilities dword 1 */

		/* Check for proper PCI/PCIe UUID */
		If (Arg0 == ToUUID("33DB4D5B-1FF7-401C-9657-7441C03DD766")) {
			/* Let OS control everything */
			Return (Arg3)
		} Else {
			CDW1 |= 4	/* Unrecognized UUID */
			Return (Arg3)
		}
	}
	Name(CRES, ResourceTemplate() {
		WordBusNumber (ResourceProducer, MinFixed, MaxFixed, PosDecode,
			0x0000,		/* address granularity */
			0x0000,		/* range minimum */
			0x00ff,		/* range maximum */
			0x0000,		/* translation */
			0x0100,		/* length */
			,, PSB0)	/* ResourceSourceIndex, ResourceSource, DescriptorName */

		IO(Decode16, 0x0cf8, 0x0cf8, 1,	8)

		WORDIO(ResourceProducer, MinFixed, MaxFixed, PosDecode, EntireRange,
			0x0000,		/* address granularity */
			0x0000,		/* range minimum */
			0x0cf7,		/* range maximum */
			0x0000,		/* translation */
			0x0cf8		/* length */
		)

		WORDIO(ResourceProducer, MinFixed, MaxFixed, PosDecode, EntireRange,
			0x0000,		/* address granularity */
			0x0d00,		/* range minimum */
			0xffff,		/* range maximum */
			0x0000,		/* translation */
			0xf300		/* length */
		)

		Memory32Fixed(READONLY, 0x000a0000, 0x00020000, VGAM)	/* VGA memory space */
		Memory32Fixed(READONLY, 0x000c0000, 0x00020000, EMM1)	/* Assume C0000-E0000 empty */

		/* memory space for PCI BARs below 4GB */
		Memory32Fixed(ReadOnly, 0x00000000, 0x00000000, MMIO)
	})

	Method(_CRS, 0) {
		CreateDWordField(CRES, ^MMIO._BAS, MM1B)
		CreateDWordField(CRES, ^MMIO._LEN, MM1L)

		/* Declare memory between TOM1 and MMCONF as available for PCI MMIO. */
		MM1B = TOM1
		Local0 = CONFIG_ECAM_MMCONF_BASE_ADDRESS
		Local0 -= TOM1
		MM1L = Local0

		CreateWordField(CRES, ^PSB0._MAX, BMAX)
		CreateWordField(CRES, ^PSB0._LEN, BLEN)
		BMAX = CONFIG_ECAM_MMCONF_BUS_NUMBER - 1
		BLEN = CONFIG_ECAM_MMCONF_BUS_NUMBER

		Return(CRES) /* note to change the Name buffer */
	} /* end of Method(_SB.PCI0._CRS) */

	/* 0:14.3 - LPC */
	#include <soc/amd/common/acpi/lpc.asl>

} /* End PCI0 scope */
