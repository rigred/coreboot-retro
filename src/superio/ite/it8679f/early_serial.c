/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <arch/io.h>
#include <device/pnp_ops.h>
#include <device/pnp.h>
#include <superio/ite/common/ite.h>
#include <stdint.h>
#include "it8679f.h"

/* The base address is 0x3f0, 0x3bd, or 0x370, depending on config bytes. */
#define SIO_BASE	0x3f0
#define SIO_INDEX	SIO_BASE
#define SIO_DATA	(SIO_BASE + 1)

/* Global configuration registers. */
#define IT8679F_CONFIG_REG_CC      0x02   /* Configure Control (write-only). */
#define IT8679F_CONFIG_REG_LDN     0x07   /* Logical Device Number. */
#define IT8679F_CONFIG_REG_LDE     0x23   /* PnP Logical Device Enable. */
#define IT8679F_CONFIG_REG_SWSUSP  0x24   /* Software Suspend. */

#define IT8679F_CONFIGURATION_PORT 0x0279 /* Write-only. */

static const u8 init_values[] = {
	0x6a, 0xb5, 0xda, 0xed, /**/ 0xf6, 0xfb, 0x7d, 0xbe,
	0xdf, 0x6f, 0x37, 0x1b, /**/ 0x0d, 0x86, 0xc3, 0x61,
	0xb0, 0x58, 0x2c, 0x16, /**/ 0x8b, 0x45, 0xa2, 0xd1,
	0xe8, 0x74, 0x3a, 0x9d, /**/ 0xce, 0xe7, 0x73, 0x39,
};

static void it8679f_sio_write(u8 ldn, u8 index, u8 value)
{
	outb(IT8679F_CONFIG_REG_LDN, SIO_BASE);
	outb(ldn, SIO_DATA);
	outb(index, SIO_BASE);
	outb(value, SIO_DATA);
}

/* Enter the configuration state (MB PnP mode). */
static void it8679f_enter_conf(void)
{
	int i;

	/* Perform MB PnP setup to put the SIO chip at 0x3f0. */
	/* Base address 0x3f0: 0x86 0x80 0x55 0x55. */
	/* Base address 0x3bd: 0x86 0x80 0x55 0xaa. */
	/* Base address 0x370: 0x86 0x80 0xaa 0x55. */
	outb(0x86, IT8679F_CONFIGURATION_PORT);
	outb(0x80, IT8679F_CONFIGURATION_PORT);
	outb(0x55, IT8679F_CONFIGURATION_PORT);
	outb(0x55, IT8679F_CONFIGURATION_PORT);

	/* Sequentially write the 32 special values. */
	for (i = 0; i < 32; i++)
		outb(init_values[i], SIO_BASE);
}

/* Exit the configuration state (MB PnP mode). */
static void it8679f_exit_conf(void)
{
	it8679f_sio_write(0x00, IT8679F_CONFIG_REG_CC, 0x02);
}

/* Select 48MHz CLKIN (24MHz is the default). */
void it8679f_48mhz_clkin(void)
{
	it8679f_enter_conf();
	it8679f_sio_write(0x00, IT8679F_CONFIG_REG_SWSUSP, (1 << 6));
	it8679f_exit_conf();
}


/* Bring up early serial debugging output before the RAM is initialized. */
void it8679f_enable_serial(pnp_devfn_t dev, u16 iobase)
{
	it8679f_enter_conf();

	/*
	 * Allow all devices to be enabled. Bits: FDC (0), Com1 (1), Com2 (2),
     * PP (3), APC (4), KBCK (5), KBCM (6), GPIO (7).
	 */

	it8679f_sio_write(0x00, IT8679F_CONFIG_REG_LDE, 0x6f);

	/* Enable serial port(s). */
	it8679f_sio_write(IT8679F_SP1, 0x30, 0x01); /* Serial port 1 */
	it8679f_sio_write(IT8679F_SP2, 0x30, 0x01); /* Serial port 2 */

	it8679f_exit_conf();
}