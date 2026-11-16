/* SPDX-License-Identifier: GPL-2.0-only */

#include <southbridge/intel/i82371eb/i82371eb.h>

#define SUPERIO_PNP_BASE 0x3F0
#define SUPERIO_SHOW_UARTA
#define SUPERIO_SHOW_UARTB
#define SUPERIO_SHOW_FDC
#define SUPERIO_SHOW_LPT

#include <acpi/acpi.h>
#include "routing.h"

DefinitionBlock (
	"dsdt.aml",
	"DSDT",
	ACPI_DSDT_REV_2,
	OEM_ID,
	ACPI_TABLE_CREATOR,
	1
	)
{
	#include <acpi/dsdt_top.asl>
	/* \_SB scope defining the main processor is generated in SSDT. */

	#include <arch/x86/acpi/post.asl>

	/* S1 needs a validated resume path; vendor SMI traps are not implemented. */
	Name (\_S0, Package () { 0x05, 0x05, 0x00, 0x00 })
	Name (\_S5, Package () { 0x00, 0x00, 0x00, 0x00 })

	/* Root of the bus hierarchy */
	Scope (\_SB)
	{
		#include "acpi/intx.asl"

		PCI_INTX_DEV(LNKA, \_SB.PCI0.PX70.PIRA, 1)
		PCI_INTX_DEV(LNKB, \_SB.PCI0.PX70.PIRB, 2)
		PCI_INTX_DEV(LNKC, \_SB.PCI0.PX70.PIRC, 3)
		PCI_INTX_DEV(LNKD, \_SB.PCI0.PX70.PIRD, 4)

		/* Top PCI device */
		Device (PCI0)
		{
			Name (_HID, EisaId ("PNP0A03"))
			Name (_UID, 0x00)
			Name (_BBN, 0x00)

			/* One wiring description for $PIR, firmware setup and ACPI. */
#define ROTATE_0(d) \
	Package () { ((d) << 16) | 0xffff, 0, LNKA, 0 }, \
	Package () { ((d) << 16) | 0xffff, 1, LNKB, 0 }, \
	Package () { ((d) << 16) | 0xffff, 2, LNKC, 0 }, \
	Package () { ((d) << 16) | 0xffff, 3, LNKD, 0 },
#define ROTATE_1(d) \
	Package () { ((d) << 16) | 0xffff, 0, LNKB, 0 }, \
	Package () { ((d) << 16) | 0xffff, 1, LNKC, 0 }, \
	Package () { ((d) << 16) | 0xffff, 2, LNKD, 0 }, \
	Package () { ((d) << 16) | 0xffff, 3, LNKA, 0 },
#define ROTATE_2(d) \
	Package () { ((d) << 16) | 0xffff, 0, LNKC, 0 }, \
	Package () { ((d) << 16) | 0xffff, 1, LNKD, 0 }, \
	Package () { ((d) << 16) | 0xffff, 2, LNKA, 0 }, \
	Package () { ((d) << 16) | 0xffff, 3, LNKB, 0 },
#define ROTATE_3(d) \
	Package () { ((d) << 16) | 0xffff, 0, LNKD, 0 }, \
	Package () { ((d) << 16) | 0xffff, 1, LNKA, 0 }, \
	Package () { ((d) << 16) | 0xffff, 2, LNKB, 0 }, \
	Package () { ((d) << 16) | 0xffff, 3, LNKC, 0 },
#define ROUTE(d, s, r) ROTATE_##r(d)
			Name (_PRT, Package () { MS6117_ROUTES(ROUTE) })
#undef ROUTE
			#include <northbridge/intel/i440lx/acpi/sb_pci0_crs.asl>
			#include <southbridge/intel/i82371eb/acpi/isabridge.asl>

			#include <mainboard/msi/ms6117/i82371ab.asl>
			#include <superio/winbond/w83977tf/acpi/superio.asl>
		}
	}

}
