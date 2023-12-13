/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <spd.h>
#include <delay.h>
#include <stdint.h>
#include <device/mmio.h>
#include <device/pci_ops.h>
#include <device/pci_def.h>
#include <device/smbus_host.h>
#include <console/console.h>
#include <commonlib/console/post_codes.h>
#include <timestamp.h>
#include "i440lx.h"
#include "raminit.h"

/*
 * Macros and definitions
 */

/* Debugging macros. */
#if CONFIG(DEBUG_RAM_SETUP)
#define PRINT_DEBUG(x...)	printk(BIOS_DEBUG, x)
#define PRINT_DEBUG_HEX8(x)	printk(BIOS_DEBUG, "%02x", x)
#define PRINT_DEBUG_HEX16(x)	printk(BIOS_DEBUG, "%04x", x)
#define PRINT_DEBUG_HEX32(x)	printk(BIOS_DEBUG, "%08x", x)
#define DUMPNORTH()		dump_pci_device(NB)
#else
#define PRINT_DEBUG(x...)
#define PRINT_DEBUG_HEX8(x)
#define PRINT_DEBUG_HEX16(x)
#define PRINT_DEBUG_HEX32(x)
#define DUMPNORTH()
#endif

/* DRAMXC[7:5] - SDRAM Mode Select (SMS). */
#define RAM_COMMAND_NORMAL	0x0
#define RAM_COMMAND_NOP		0x1
#define RAM_COMMAND_PRECHARGE	0x2
#define RAM_COMMAND_MRS		0x3
#define RAM_COMMAND_CBR		0x4

/* Table format: register, value. */
static const u8 register_values[] = {
	/* PACCFG - PAC Configuration Register
	 * 0x50 - 0x51
	 *
	 * [15:15] WSC# Handshake Disable
	 *         1 = Uni-processor mode
	 *         0 = Dual-processor mode with external IOAPIC (default)
	 * [14:14] Host Frequency (RO)
	 *         MECC0 pin strap, used to select refresh tick rate and display system bus frequency
	 *         1 = 60 MHz
	 *         0 = 66 MHz
	 * [13:12] Reserved
	 * [11:11] DRAM Configuration Mode (strapped on CKE)
	 *         1 = Memory Configuration #1
	 *         0 = Memory Configuration #2 (default)
	 * [10:10] PCI Agent to Aperture Access Disable
	 *         1 = Disable
	 *         0 = Enable (default)
	 * [09:09] Aperture Access Global Enable
	 *         1 = Enable
	 *         0 = Disable
	 * [08:07] DRAM Data Integrity Mode (DDIM)
	 *         00 = Non-ECC
	 *         01 = EC-only with no correction
	 *         10 = reserved
	 *         11 = ECC Mode with correction
	 * [06:06] ECC Diagnostic Mode Enable (EDME)
	 *         1 = Enable
	 *         0 = Normal operation mode (default)
	 * [05:05] MDA Present (MDAP)
	 *         Works in conjunction with the VGA_EN bit.
	 *         VGA_EN MDAP
	 *           0     x   All VGA cycles are sent to PCI
	 *           1     0   All VGA cycles are sent to AGP
	 *           1     1   All VGA cycles are sent to AGP, except for
	 *                     cycles in the MDA range.
	 * [04:00] Reserved
	 */

	PACCFG, 0x04,
#if CONFIG(SMP)
	PACCFG + 1, 0x08,
#else
	PACCFG + 1, 0x82,
#endif

	/* DBC - Data Buffer Control Register 
	 * 0x53
	 *
	 * [7:7] Reserved
	 * [6:6] CPU-to-PCI IDE Posting Enable
	 *       1 = Enable
	 *       0 = Disable
	 * [5:5] WC Write Post During I/O Bridge Access Enable
	 *       1 = Enable
	 *       0 = Disable
	 * [4:0] Reserved
	 */
	DBC, 0xE3, /* Enable all useful features*/

	/* DRT DRAM Row Type (must be set)
	 * 0x55 - 0x56
	 * 
	 * 00 = EDO
	 * 01 = Reserved
	 * 10 = SDRAM
	 * 11 = Empty Row
	 * 
	 * RPS bits Corresponding DRB register
	 * [01:00]  DRB[0], row 0
	 * [03:02]  DRB[1], row 1
	 * [05:04]  DRB[2], row 2
	 * [07:06]  DRB[3], row 3
	 * [09:08]  DRB[4], row 4
	 * [11:10]  DRB[5], row 5
	 * [13:12]  DRB[6], row 6
	 * [15:14]  DRB[7], row 7
	 */
	DRT,   0xFF,
	DRT+1, 0xFF,
	

	/* DRAMC - DRAM Control Register
	 * 0x57
	 *
	 * [7:6] Reserved
	 * [5:5] DRAM EDO Auto Detect Mode Enable (DEDM)
	 *       1 = Enable a special timing mode for BIOS to detect EDO DRAM type on a row-by-row basis
	 *       0 = Disable (Default)
	 * [4:4] SDRAM Power Management Support Enable (SPME)
	 *       1 = Enable
	 *       0 = Disable (Default)
	 * [2:0] DRAM Refresh Rate (DRR)
	 *       000 = Refresh disabled
	 *       001 = Normal (15.6us)
	 *       010-111 = Reserved 
	 */
	/* Disable refresh for now. */
	DRAMC, 0x00,

	/* DRAMT - DRAM Timing Register
	 * 0x58
	 *
	 * [7:7] SDRAM RAS to CAS Delay (SRCD)
	 *       1 = 2 CL
	 *       0 = 3 CL (Default)
	 * [6:6] SDRAM CAS Latency
	 *       1 = 2 CL
	 *       0 = 3 CL (Default)
	 * [5:5] SDRAM RAS Precharge Time (SRPT)
	 *       1 = 2 CL
	 *       0 = 3 CL
	 * [4:4] EDO DRAM Read Burst Timing (DRBT)
	 *       1 = x222
	 *       0 = x333 (Default)
	 * [3:3] EDO DRAM Write Burst Timing (DWBT)
	 *       1 = x222
	 *       0 = x333 (Default)
	 * [2:2] EDO RAS Precharge Time (RPT)
	 *       1 = 3 CL
	 *       0 = 4 CL (Default)
	 * [1:1] EDO RAS to CAS Delay (RCD)
	 *       1 = 2 CL
	 *       0 = 3 CL
	 * [0:0] MA Wait State (MAWS)
	 * 	     1 = FAST
	 *       0 = SLOW (Default)
	 */
	/* Set timings later */
	DRAMT, 0x00,

	/*
	 * PAM[6:0] - Programmable Attribute Map Registers
	 * 0x59 - 0x5f
	 *
	 * 0x59 [3:0] Reserved
	 * 0x59 [5:4] 0xF0000 - 0xFFFFF BIOS area
	 * 0x5a [1:0] 0xC0000 - 0xC3FFF ISA add-on BIOS
	 * 0x5a [5:4] 0xC4000 - 0xC7FFF ISA add-on BIOS
	 * 0x5b [1:0] 0xC8000 - 0xCBFFF ISA add-on BIOS
	 * 0x5b [5:4] 0xCC000 - 0xCFFFF ISA add-on BIOS
	 * 0x5c [1:0] 0xD0000 - 0xD3FFF ISA add-on BIOS
	 * 0x5c [5:4] 0xD4000 - 0xD7FFF ISA add-on BIOS
	 * 0x5d [1:0] 0xD8000 - 0xDBFFF ISA add-on BIOS
	 * 0x5d [5:4] 0xDC000 - 0xDFFFF ISA add-on BIOS
	 * 0x5e [1:0] 0xE0000 - 0xE3FFF BIOS extension
	 * 0x5e [5:4] 0xE4000 - 0xE7FFF BIOS extension
	 * 0x5f [1:0] 0xE8000 - 0xEBFFF BIOS extension
	 * 0x5f [5:4] 0xEC000 - 0xEFFFF BIOS extension
	 *
	 * Bit assignment:
	 * 00 = DRAM Disabled (all access goes to memory mapped I/O space)
	 * 01 = Read Only (Reads to DRAM, writes to memory mapped I/O space)
	 * 10 = Write Only (Writes to DRAM, reads to memory mapped I/O space)
	 * 11 = Read/Write (all access goes to DRAM)
	 */

	/*
	 * Map all legacy regions to RAM (read/write). This is required if
	 * you want to use the RAM area from 768 KB - 1 MB. If the PAM
	 * registers are not set here appropriately, the RAM in that region
	 * will not be accessible, thus a RAM check of it will also fail.
	 */
	PAM0, 0x30,
	PAM1, 0x33,
	PAM2, 0x33,
	PAM3, 0x33,
	PAM4, 0x33,
	PAM5, 0x33,
	PAM6, 0x33,

	/* DRB[0:7] - DRAM Row Boundary Registers
	 * 0x60 - 0x67
	 *
	 * An array of 8 byte registers, which hold the ending memory address
	 * assigned to each pair of DIMMs, in 8MB granularity.
	 *
	 * 0x60 DRB0 = Total memory in row0 (in 8 MB)
	 * 0x61 DRB1 = Total memory in row0+1 (in 8 MB)
	 * 0x62 DRB2 = Total memory in row0+1+2 (in 8 MB)
	 * 0x63 DRB3 = Total memory in row0+1+2+3 (in 8 MB)
	 * 0x64 DRB4 = Total memory in row0+1+2+3+4 (in 8 MB)
	 * 0x65 DRB5 = Total memory in row0+1+2+3+4+5 (in 8 MB)
	 * 0x66 DRB6 = Total memory in row0+1+2+3+4+5+6 (in 8 MB)
	 * 0x67 DRB7 = Total memory in row0+1+2+3+4+5+6+7 (in 8 MB)
	 */
	/* DRBs will be set later. */

	/* FDHC - Fixed DRAM Hole Control Register
	 * 0x68
	 *
	 * Controls two fixed DRAM holes: 512 KB - 640 KB and 15 MB - 16 MB.
	 *
	 * [7:6] Hole Enable (HEN)
	 *       00 = None
	 *       01 = 512 KB - 640 KB (128 KB)
	 *       10 = 15 MB - 16 MB (1 MB)
	 *       11 = Reserved
	 * [5:0] Reserved
	 */
	/* No memory holes. */
	FDHC, 0x00,

	/* DRAMXC - SDRAM Control Register
	 * 0x6a - 0x6b
	 *
	 * [15:08] Reserved
	 * [07:05] SDRAM Mode Select (SMS)
	 *         000 = Normal SDRAM Operation (default)
	 *         001 = NOP Command Enable
	 *         010 = All Banks Precharge Enable
	 *         011 = Mode Register Set Enable
	 *         100 = CBR Enable
	 *         101 = Reserved
	 *         110 = Reserved
	 *         111 = Reserved
	 * [04:04] Reserved
	 * [03:02] Page Timeout Select (PTOS)
	 *         00 = 16 Clocks (Default)
	 *         01 = Reserved
	 *         10 = Reserved
	 *         11 = Reserved
	 * [01:00] Close Both Banks Control (CBBC)
	 *         00 = Close Both Banks on Arb Switch PageMiss (Default)
	 *         01 = Reserved
	 *         10 = Reserved
	 *         11 = Reserved
	 */

	/* MBSC - Memory Buffer Strength Control
	 * 0x6c - 0x6f
	 * [31:30] MAA[1:0] Buffer Strength
	 *         00 = 48 mA
	 *         01 = 42 mA
	 *         10 = 22 mA
	 *         11 = Reserved
	 */

	/* APSIZE - Aperture Size
	 * 0xB4
	 * [7:6] Reserved
	 * [5:0] Graphics Aperture Size
	 *       11 1111 = 4MB
	 *       11 1110 = 8MB 
	 *       11 1100 = 16MB 
	 *       11 1000 = 32MB 
	 *       11 0000 = 64MB 
	 *       10 0000 = 128MB 
	 *       00 0000 = 256MB 
	 */
	/* Write default 64MB Aperture size*/

	/* APBASE - Aperture Base configuration Register
	 * 0x10-0x13
	 */

};

/*-----------------------------------------------------------------------------
SDRAM configuration functions.
-----------------------------------------------------------------------------*/

/**
 * Send the specified RAM command to all DIMMs.
 *
 * @param command The RAM command to send to the DIMM(s).
 */
static void do_ram_command(u32 command)
{
	int i, caslatency;
	u8 dimm_start, dimm_end;
	u16 reg16;
	void *addr;
	u32 addr_offset;

	/* Configure the RAM command. */
	reg16 = pci_read_config16(NB, DRAMXC);
	reg16 &= 0xff1f;		/* Clear bits 7-5. */
	reg16 |= (u16)(command << 5);	/* Write command into bits 7-5. */
	pci_write_config16(NB, DRAMXC, reg16);

	/*
	 * RAM_COMMAND_NORMAL affects only the memory controller and
	 * doesn't need to be "sent" to the DIMMs.
	 */
	if (command == RAM_COMMAND_NORMAL)
		return;

	/* Send the RAM command to each row of memory. */
	dimm_start = 0;
	for (i = 0; i < (DIMM_SOCKETS * 2); i++) {
		addr_offset = 0;
		caslatency = 3; /* TODO: Dynamically get CAS latency later. */
		if (command == RAM_COMMAND_MRS) {
			if (caslatency == 3)
				addr_offset = 0x1d0;
			if (caslatency == 2)
				addr_offset = 0x150;
		}

		dimm_end = pci_read_config8(NB, DRB + i);

		addr = (void *)((dimm_start * 8 * 1024 * 1024) + addr_offset);
		if (dimm_end > dimm_start) {
			read32(addr);
		}

		/* Set the start of the next DIMM. */
		dimm_start = dimm_end;
	}
}

static void set_dram_buffer_strength(void)
{
	// set drive strength
	pci_write_config32(NB, MBSC, 0x00000000);
}

static void set_chipset_regs(void) {
	/* TODO? */
	pci_write_config8(NB, PCI_LATENCY_TIMER, 0x20);
	pci_write_config8(NB, MTT, 0x20);
	pci_write_config8(NB, AMTT, 0x20);
}

/*-----------------------------------------------------------------------------
DIMM-independent configuration functions.
-----------------------------------------------------------------------------*/

static void spd_enable_refresh(void)
{
	uint8_t reg;

	reg = pci_read_config8(NB, DRAMC);
	reg = (reg & 0xf8) | 0x01;

	PRINT_DEBUG("    Enabled refresh (DRAMC = 0x%02x) for DIMMs\n", reg);

	pci_write_config8(NB, DRAMC, reg);
}

/*-----------------------------------------------------------------------------
Public interface.
-----------------------------------------------------------------------------*/


static void northbridge_init(void)
{
	uint32_t reg32;

	/* Set default 64MB Aperture size first */
	pci_write_config8(NB, APSIZE, 0x30);
	
	/* Set Aperture base, this should ideally be at top of memory */
	reg32 = pci_read_config32(NB, APBASE);
	reg32 |= 0xe8000000U;
	pci_write_config32(NB, APBASE, reg32);

#if CONFIG_DEBUG_RAM_SETUP
	/*
	 * apbase likely problematic
	 */
	reg32 = pci_read_config32(NB, APBASE);
	PRINT_DEBUG("APBASE ");
	PRINT_DEBUG_HEX32(reg32);
	PRINT_DEBUG("\n");
#endif

	uint16_t reg16;
	reg16 = pci_read_config16(NB, PACCFG);
	printk(BIOS_DEBUG, "CPU Host Freq: 6%d MHz\n", (reg16 & 0x4000) ? 0 : 6);

}


static void sdram_set_registers(void)
{
	int i, max;

	PRINT_DEBUG("Northbridge %s SDRAM init:\n", "prior to");
	DUMPNORTH();

	max = ARRAY_SIZE(register_values);

	/* Set registers as specified in the register_values[] array. */
	for (i = 0; i < max; i += 2) {
		uint8_t tmp;

		pci_write_config8(NB, register_values[i], register_values[i + 1]);


#if CONFIG_DEBUG_RAM_SETUP
		PRINT_DEBUG("    Set register 0x");
		PRINT_DEBUG_HEX8(register_values[i]);
		PRINT_DEBUG(" to 0x");
		PRINT_DEBUG_HEX8(register_values[i + 1]);
		tmp = pci_read_config8(NB, register_values[i]);
		PRINT_DEBUG(" readed 0x");
		PRINT_DEBUG_HEX8(tmp);
		if (tmp == register_values[i + 1]) {
			PRINT_DEBUG(" OK ");
		} else {
			PRINT_DEBUG(" FAIL ");
		}
		PRINT_DEBUG("\n");
#endif

	}
}

struct dimm_size {
	u32 side1;
	u32 side2;
};

static struct dimm_size spd_get_dimm_size(unsigned int device)
{
	struct dimm_size sz;
	int i, module_density, dimm_banks;
	sz.side1 = 0;
	module_density = smbus_read_byte(device, SPD_DENSITY_OF_EACH_ROW_ON_MODULE);
	dimm_banks = smbus_read_byte(device, SPD_NUM_DIMM_BANKS);

	/* Find the size of side1. */
	/* Find the larger value. The larger value is always side1. */
	for (i = 512; i >= 0; i >>= 1) {
		if ((module_density & i) == i) {
			sz.side1 = i;
			break;
		}
	}

	/* Set to 0 in case it's single sided. */
	sz.side2 = 0;

	/* Test if it's a dual-sided DIMM. */
	if (dimm_banks > 1) {
		/* Test if there's a second value. If so it's asymmetrical. */
		if (module_density != i) {
			/*
			 * Find second value, picking up where we left off.
			 * i >>= 1 done initially to make sure we don't get
			 * the same value again.
			 */
			for (i >>= 1; i >= 0; i >>= 1) {
				if (module_density == (sz.side1 | i)) {
					sz.side2 = i;
					break;
				}
			}
			/* If not, it's symmetrical. */
		} else {
			sz.side2 = sz.side1;
		}
	}

	/*
	 * SPD byte 31 is the memory size divided by 4 so we
	 * need to multiply by 4 to get the total size.
	 */
	sz.side1 *= 4;
	sz.side2 *= 4;

	/*
	 * It is possible to partially use larger than supported
	 * modules by setting them to a supported size.
	 */
	if (sz.side1 > 128) {
		PRINT_DEBUG("Side%d was %dMB but only 128MB will be used.\n",
			1, sz.side1);
		sz.side1 = 128;

		if (sz.side2 > 128) {
			PRINT_DEBUG("Side%d was %dMB but only 128MB will be used.\n",
				2, sz.side2);
			sz.side2 = 128;
		}
	}

	return sz;
}
/*
 * Sets DRAM attributes one DIMM at a time, based on SPD data.
 * Northbridge settings that are set: PACCFG[31:24], DRB0-DRB7, DRT, RPS, DRAMC.
 */
static void set_dram_row_attributes(void)
{
	int i, dra, drb, col, width, value, rps;
	u16 drt = 0;
	u8 bpr; /* Top 8 bits of PGPOL (Useless for 440LX) */
	u8 nbxecc = 0; /* PACCFG[Not Used] */
	u8 edo, sd, regsd; /* EDO, SDRAM, registered SDRAM */

	edo = 0;
	sd = 0;
	regsd = 0;
	rps = 0;
	drb = 0;
	bpr = 0;

    for (i = 0; i < DIMM_SOCKETS; i++) {
		unsigned int device;
		/* Logic to handle 3DIMM boards where DIMM0 starts at 0x52 */
#if CONFIG(SDRAM_3DIMM_REVERSE_OFFSET)
        device = DIMM0 - i;
#else
		/* All other (normal) boards 3DIMM or 4DIMM */
        device = DIMM0 + i;
#endif
		bpr >>= 2;
		nbxecc >>= 2;

		/* First check if a DIMM is actually present. */
		value = smbus_read_byte(device, SPD_MEMORY_TYPE);
		/* This is 440LX! We do EDO too! */
		if (value == SPD_MEMORY_TYPE_EDO
			|| value == SPD_MEMORY_TYPE_SDRAM) {

			if (value == SPD_MEMORY_TYPE_EDO) {
				edo = 1;
			} else if (value == SPD_MEMORY_TYPE_SDRAM) {
				sd = 1;
			}
			PRINT_DEBUG("Found DIMM in slot %d\n", i);

			if (edo && sd) {
				die_with_post_code(POSTCODE_RAM_FAILURE,
					"Mixing EDO/SDRAM unsupported!\n");
			}

			/* "DRA" is our RPS for the two rows on this DIMM. */
			dra = 0;

			/* Columns */
			col = smbus_read_byte(device, SPD_NUM_COLUMNS);

			/*
			 * Is this an ECC DIMM? Actually will be a 2 if so.
			 * Other registers PACCFG,DRAMC,DRAMXC also needs this
			 * ECC information.
			 */
			value = smbus_read_byte(device, SPD_DIMM_CONFIG_TYPE);

			/* Data width */
			width = smbus_read_byte(device, SPD_MODULE_DATA_WIDTH_LSB);

			/* Exclude error checking data width from page size calculations */
			if (value) {
				value = smbus_read_byte(device,
					SPD_ERROR_CHECKING_SDRAM_WIDTH);
				width -= value;
				/* ### ECC */
				/* Clear top 2 bits to help set up PACCFG. */
				nbxecc &= 0x3f;
			} else {
				/* Without ECC, top 2 bits should be 11. */
				nbxecc |= 0xc0;
			}

			/* If any installed DIMM is *not* registered, this system cannot be
			 * configured for registered SDRAM.
			 * By registered, only the address and control lines need to be, which
			 * we can tell by reading SPD byte 21, bit 1.
			 */
			value = smbus_read_byte(device, SPD_MODULE_ATTRIBUTES);

			PRINT_DEBUG("DIMM is ");
			if ((value & MODULE_REGISTERED) == 0) {
				regsd = 0;
				PRINT_DEBUG("not ");
			} else {
				regsd = 1;
			}
			PRINT_DEBUG("registered\n");

			if (regsd) {
				die_with_post_code(POSTCODE_RAM_FAILURE,
				"Registered SDRAM unsupported!\n");
			}

			/* Calculate page size in bits. */
			value = ((1 << col) * width);

			/* Convert to KB. */
			dra = (value >> 13);

			/* Number of banks of DIMM (single or double sided). */
			value = smbus_read_byte(device, SPD_NUM_DIMM_BANKS);

			/* Once we have dra, col is done and can be reused.
			 * So it's reused for number of banks.
			 */
			col = smbus_read_byte(device, SPD_NUM_BANKS_PER_SDRAM);

			if (value == 1) {
				/*
				 * Second bank of 1-bank DIMMs "doesn't have
				 * ECC" - or anything.
				 */
				if (dra == 2) {
					dra = 0x0; /* 2KB */
				} else if (dra == 4) {
					dra = 0x1; /* 4KB */
				} else if (dra == 8) {
					dra = 0x2; /* 8KB */
				} else if (dra >= 16) {
					/* Page sizes larger than supported are
					 * set to 8KB to use module partially.
					 */
					PRINT_DEBUG("Page size forced to 8KB.\n");
					dra = 0x2; /* 8KB */
				} else {
					dra = -1;
				}
				/*
				 * Sets a flag in PGPOL[BPR] if this DIMM has
				 * 4 banks per row.
				 */
				if (col == 4)
					bpr |= 0x40;
			} else if (value == 2) {
				if (dra == 2) {
					dra = 0x0; /* 2KB */
				} else if (dra == 4) {
					dra = 0x05; /* 4KB */
				} else if (dra == 8) {
					dra = 0x0a; /* 8KB */
				} else if (dra >= 16) {
					/* Ditto */
					PRINT_DEBUG("Page size forced to 8KB.\n");
					dra = 0x0a; /* 8KB */
				} else {
					dra = -1;
				}
				/* Ditto */
				if (col == 4)
					bpr |= 0xc0;
			} else {
				die_with_post_code(POSTCODE_RAM_FAILURE,
					"# of banks of DIMM unsupported!\n");
			}
			if (dra == -1) {
				die_with_post_code(POSTCODE_RAM_FAILURE,
					"Page size not supported!\n");
			}

			/*
			 * 440LX supports asymmetrical dual-sided DIMMs,
			 * but can't handle DIMMs smaller than 8MB per
			 * side.
			 */
			struct dimm_size sz = spd_get_dimm_size(device);
			if ((sz.side1 < 8)) {
				die_with_post_code(POSTCODE_RAM_FAILURE,
					"DIMMs smaller than 8MB per side "
					"are not supported!\n");
			}

			// Set DRT bits for side1
			if (sz.side1 > 0) {
				drt |= (0x2 << (i * 4));  // SDRAM: Set the corresponding DRT bits to 10
			} else {
				drt |= (0x3 << (i * 4));  // Empty row: Set the corresponding DRT bits to 11
			}

			// Set DRT bits for side2
			if (sz.side2 > 0) {
				drt |= (0x2 << ((i * 4) + 2));  // SDRAM: Set the corresponding DRT bits to 10
			} else {
				drt |= (0x3 << ((i * 4) + 2));  // Empty row: Set the corresponding DRT bits to 11
			}
			
			/* Divide size by 8 to set up the DRB registers. */
			drb += (sz.side1 / 8);

			/*
			 * Build the DRB for the next row in MSB so it gets
			 * placed in DRB[n+1] where it belongs when written
			 * as a 16-bit word.
			 */
			drb &= 0xff;
			drb |= (drb + (sz.side2 / 8)) << 8;
		} else {
			/* If there's no DIMM in the slot, set dra to 0x00. and drt to empty row */
			dra = 0x00;
			
			// Set DRT bits for both rows to 11 for an empty DIMM
    		drt |= (0x3 << (i * 4));
    		drt |= (0x3 << ((i * 4) + 2));
    			
			/* Still have to propagate DRB over. */
			drb &= 0xff;
			drb |= (drb << 8);
		}

		pci_write_config16(NB, DRB + (2 * i), drb);

		/* Brings the upper DRB back down to be base for
		 * DRB calculations for the next two rows.
		 */
		drb >>= 8;

		rps |= (dra & 0x0f) << (i * 4);
	}

#if CONFIG(SDRAM_3DIMM_REVERSE_OFFSET)
	/* Set last DRT (Unused Row 6,7) */
	// Set DRT bits for both rows to 11 for a non existent DIMM slot
	drt |= (0x3 << (DIMM_SOCKETS * 4));
	drt |= (0x3 << ((DIMM_SOCKETS * 4) + 2));

	drb >>= 8;

	pci_write_config16(NB, DRB + (2 * DIMM_SOCKETS), drb);
#elif CONFIG(SDRAM_3DIMM)
	/* Set last DRT (Unused Row 6,7) */
	// Set DRT bits for both rows to 11 for a non existent DIMM slot
	drt |= (0x3 << (DIMM_SOCKETS * 4));
	drt |= (0x3 << ((DIMM_SOCKETS * 4) + 2));

	drb >>= 8;

	pci_write_config16(NB, DRB + (2 * DIMM_SOCKETS), drb);
#endif

	pci_write_config8(NB, DRT, drt);
	pci_write_config8(NB, DRT + 1, drt >> 8);

	drt = pci_read_config8(NB, DRT+1);
	drt <<=8;
	drt |= pci_read_config8(NB, DRT);

	PRINT_DEBUG("%s has been set to 0x%02x\n", "DRT", drt);

	/* Set DRAMC. Don't enable refresh for now. */
	pci_write_config8(NB, DRAMC, 0x00);

	/* Cas latency 3, and other shouldbe properly from spd too */
	pci_write_config8(NB, DRAMT, 0xAC);
}

static void sdram_enable(void)
{
	int i;
	PRINT_DEBUG("Northbridge %s SDRAM enable:\n", "prior to");
	DUMPNORTH();

	/* 0. Wait until power/voltages and clocks are stable (200us). */
	udelay(200);

	/* 1. Apply NOP. Wait 200 clock cycles (200us should do). */
	PRINT_DEBUG("RAM Enable %d: %s\n", 1, "Apply NOP");
	do_ram_command(RAM_COMMAND_NOP);
	udelay(200);

	/* 2. Precharge all. Wait tRP. */
	PRINT_DEBUG("RAM Enable %d: %s\n", 2, "Precharge all");
	do_ram_command(RAM_COMMAND_PRECHARGE);
	udelay(1);

	/* 3. Perform 8 refresh cycles. Wait tRC each time. */
	PRINT_DEBUG("RAM Enable %d: %s\n", 3, "CBR");
	for (i = 0; i < 8; i++) {
		do_ram_command(RAM_COMMAND_CBR);
		udelay(1);
	}

	/* 4. Mode register set. Wait two memory cycles. */
	PRINT_DEBUG("RAM Enable %d: %s\n", 4, "Mode register set");
	do_ram_command(RAM_COMMAND_MRS);
	udelay(2);

	/* 5. Normal operation. */
	PRINT_DEBUG("RAM Enable %d: %s\n", 5, "Normal operation");
	do_ram_command(RAM_COMMAND_NORMAL);
	udelay(1);

	/* 6. Finally enable refresh. */
	PRINT_DEBUG("RAM Enable %d: %s\n", 6, "Enable refresh");
	spd_enable_refresh();
	udelay(1);

	PRINT_DEBUG("Northbridge %s SDRAM enable:\n", "following");
	DUMPNORTH();
}

/* Implemented under mainboard. */
void __weak enable_spd(void) { }

void sdram_initialize(int s3resume)
{
	timestamp_add_now(TS_INITRAM_START);
	enable_spd();

	northbridge_init();

	dump_spd_registers();
	sdram_set_registers();
	/* Set up DRAM row boundary registers and other attributes. */
	set_dram_row_attributes();
	/* Set up DRAM buffer strength. */
	set_dram_buffer_strength();
	set_chipset_regs();
	sdram_enable();

	/* Clear any errors reported during raminit. */
	pci_write_config8(NB, ERRSTS, 0x11);
	pci_write_config8(NB, ERRSTS + 1, 0x7);

	timestamp_add_now(TS_INITRAM_END);
}
