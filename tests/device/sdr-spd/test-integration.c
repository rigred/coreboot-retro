/* SPDX-License-Identifier: GPL-2.0-only */
/* Reuse the fixture generator; call the real initializer with only I/O mocked. */
#define main sdr_unit_main
#include "test-sdr.c"
#undef main
#include <setjmp.h>
#include "raminit.h"

static jmp_buf failure;
static uint8_t module[64];
static unsigned int selected, applied, writes;
static bool io_failure, empty;

_Noreturn void host_die(void)
{
	longjmp(failure, 1);
}

unsigned int mainboard_sdram_clock_ps(void)
{
	selected++;
	return 15000;
}

void mainboard_sdram_clock_apply(void)
{
	assert(selected == 1 && !writes && !applied);
	applied++;
}

unsigned int get_uint_option(const char *name, unsigned int fallback)
{
	(void)name;
	return fallback;
}

int smbus_read_byte(unsigned int address, unsigned int offset)
{
	if (address != 0x50 || empty)
		return 0;
	if (io_failure && offset != SPD_MEMORY_TYPE)
		return -2;
	assert(offset < sizeof(module));
	return module[offset];
}

uint8_t pci_read_config8(unsigned int device, unsigned int reg)
{
	(void)device; (void)reg;
	return 0;
}

uint16_t pci_read_config16(unsigned int device, unsigned int reg)
{
	(void)device; (void)reg;
	return 0; /* Three-DIMM topology. */
}

void pci_write_config8(unsigned int device, unsigned int reg, uint8_t value)
{
	(void)device; (void)reg; (void)value;
	assert(applied == 1);
	writes++;
}

void pci_write_config16(unsigned int device, unsigned int reg, uint16_t value)
{
	(void)device; (void)reg; (void)value;
	assert(applied == 1);
	writes++;
}

static bool configure(void)
{
	struct lx_sdram_plan plan;

	selected = applied = writes = 0;
	if (setjmp(failure))
		return false;
	lx_spd_configure(&plan, 0, 0);
	assert(plan.total_mb == 128);
	return true;
}

int main(void)
{
	fixture(module);
	assert(configure() && applied == 1 && writes);
	module[4] = 10; module[31] = 32; checksum(module);
	assert(!configure() && !applied && !writes); /* Unsupported LX geometry. */
	fixture(module); module[27] = 60; checksum(module);
	assert(!configure() && !applied && !writes); /* No legal tRP. */
	fixture(module); module[63] ^= 1;
	assert(!configure() && !applied && !writes); /* Bad checksum. */
	fixture(module); io_failure = true;
	assert(!configure() && !applied && !writes);
	io_failure = false; empty = true;
	assert(!configure() && !applied && !writes);
	puts("PASS: actual LX initializer validates SPD/plan before clock apply and controller writes");
	return 0;
}
