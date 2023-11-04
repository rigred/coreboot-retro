/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/io.h>
#include <device/device.h>
#include <superio/conf_mode.h>
#include <acpi/acpigen.h>

/* Common enter/exit implementations */

void pnp_enter_conf_mode_55(struct device *dev)
{
	outb(0x55, dev->path.pnp.port);
}

void pnp_enter_conf_mode_6767(struct device *dev)
{
	outb(0x67, dev->path.pnp.port);
	outb(0x67, dev->path.pnp.port);
}

void pnp_enter_conf_mode_7777(struct device *dev)
{
	outb(0x77, dev->path.pnp.port);
	outb(0x77, dev->path.pnp.port);
}

void pnp_enter_conf_mode_8787(struct device *dev)
{
	outb(0x87, dev->path.pnp.port);
	outb(0x87, dev->path.pnp.port);
}

void pnp_enter_conf_mode_a0a0(struct device *dev)
{
	outb(0xa0, dev->path.pnp.port);
	outb(0xa0, dev->path.pnp.port);
}

void pnp_enter_conf_mode_a5a5(struct device *dev)
{
	outb(0xa5, dev->path.pnp.port);
	outb(0xa5, dev->path.pnp.port);
}

void pnp_exit_conf_mode_aa(struct device *dev)
{
	outb(0xaa, dev->path.pnp.port);
}

void pnp_enter_conf_mode_870155aa(struct device *dev)
{
	outb(0x87, dev->path.pnp.port);
	outb(0x01, dev->path.pnp.port);
	outb(0x55, dev->path.pnp.port);

	if (dev->path.pnp.port == 0x4e)
		outb(0xaa, dev->path.pnp.port);
	else
		outb(0x55, dev->path.pnp.port);
}

void pnp_enter_conf_mode_86805555(struct device *dev)
{
    /* PNP Seqeuence      IO Address Port	Data Port    
	 * 86, 80, 55, 55  -> 0x3F0	  			0x3F1
	 * 86, 80, 55, AA  -> 0x3BD				0x3BF
	 * 86, 80, AA, 55  -> 0x370				0x371
	*/

	outb(0x86, dev->path.pnp.port);
	outb(0x80, dev->path.pnp.port);
	outb(0x55, dev->path.pnp.port);
	outb(0x55, dev->path.pnp.port);

	/* Write the other 32 bytes to hardcoded 3f0 I/O port */
    unsigned char data[] = { 	
		0x6A, 0xB5, 0xDA, 0xED, 0xF6, 0xFB, 0x7D, 0xBE, 0xDF, 0x6F,
        0x37, 0x1B, 0x0D, 0x86, 0xC3, 0x61, 0xB0, 0x58, 0x2C, 0x16,
		0x8B, 0x45, 0xA2, 0xD1, 0xE8, 0x74, 0x3A, 0x9D, 0xCE, 0xE7,
		0x73, 0x39
	};

	for (int i = 0; i < 32; i++) {
        outb(data[i], 0x3f0);
    }
}

void pnp_exit_conf_mode_0202(struct device *dev)
{
	pnp_write_config(dev, 0x02, (1 << 1));
}

/* Functions for ACPI */
#if CONFIG(HAVE_ACPI_TABLES)
static void pnp_ssdt_enter_conf_mode_55(struct device *dev, const char *idx, const char *data)
{
	acpigen_write_store();
	acpigen_write_byte(0x55);
	acpigen_emit_namestring(idx);
}

static void pnp_ssdt_enter_conf_mode_6767(struct device *dev, const char *idx, const char *data)
{
	acpigen_write_store();
	acpigen_write_byte(0x67);
	acpigen_emit_namestring(idx);

	acpigen_write_store();
	acpigen_write_byte(0x67);
	acpigen_emit_namestring(idx);
}

static void pnp_ssdt_enter_conf_mode_7777(struct device *dev, const char *idx, const char *data)
{
	acpigen_write_store();
	acpigen_write_byte(0x77);
	acpigen_emit_namestring(idx);

	acpigen_write_store();
	acpigen_write_byte(0x77);
	acpigen_emit_namestring(idx);
}

static void pnp_ssdt_enter_conf_mode_8787(struct device *dev, const char *idx, const char *data)
{
	acpigen_write_store();
	acpigen_write_byte(0x87);
	acpigen_emit_namestring(idx);

	acpigen_write_store();
	acpigen_write_byte(0x87);
	acpigen_emit_namestring(idx);
}

static void pnp_ssdt_enter_conf_mode_a0a0(struct device *dev, const char *idx, const char *data)
{
	acpigen_write_store();
	acpigen_write_byte(0xa0);
	acpigen_emit_namestring(idx);

	acpigen_write_store();
	acpigen_write_byte(0xa0);
	acpigen_emit_namestring(idx);

}

static void pnp_ssdt_enter_conf_mode_a5a5(struct device *dev, const char *idx, const char *data)
{
	acpigen_write_store();
	acpigen_write_byte(0xa5);
	acpigen_emit_namestring(idx);

	acpigen_write_store();
	acpigen_write_byte(0xa5);
	acpigen_emit_namestring(idx);
}

static void pnp_ssdt_enter_conf_mode_870155aa(struct device *dev,
					      const char *idx, const char *data)
{
	acpigen_write_store();
	acpigen_write_byte(0x87);
	acpigen_emit_namestring(idx);

	acpigen_write_store();
	acpigen_write_byte(0x01);
	acpigen_emit_namestring(idx);

	acpigen_write_store();
	acpigen_write_byte(0x55);
	acpigen_emit_namestring(idx);

	acpigen_write_store();
	if (dev->path.pnp.port == 0x4e)
		acpigen_write_byte(0xaa);
	else
		acpigen_write_byte(0x55);
	acpigen_emit_namestring(idx);
}

static void pnp_ssdt_enter_conf_mode_86805555(struct device *dev,
					      const char *idx, const char *data)
{
	acpigen_write_store();
	acpigen_write_byte(0x86);
	acpigen_emit_namestring(idx);

	acpigen_write_store();
	acpigen_write_byte(0x80);
	acpigen_emit_namestring(idx);

	acpigen_write_store();
	acpigen_write_byte(0x55);
	acpigen_emit_namestring(idx);

	acpigen_write_store();
	acpigen_write_byte(0x55);
	acpigen_emit_namestring(idx);
}

static void pnp_ssdt_exit_conf_mode_aa(struct device *dev, const char *idx, const char *data)
{
	acpigen_write_store();
	acpigen_write_byte(0xaa);
	acpigen_emit_namestring(idx);
}

static void pnp_ssdt_exit_conf_mode_0202(struct device *dev, const char *idx, const char *data)
{

	acpigen_write_store();
	acpigen_write_byte(0x02);
	acpigen_emit_namestring(idx);

	acpigen_write_store();
	acpigen_write_byte(0x02);
	acpigen_emit_namestring(data);
}
#endif

const struct pnp_mode_ops pnp_conf_mode_55_aa = {
	.enter_conf_mode = pnp_enter_conf_mode_55,
	.exit_conf_mode  = pnp_exit_conf_mode_aa,
#if CONFIG(HAVE_ACPI_TABLES)
	.ssdt_enter_conf_mode = pnp_ssdt_enter_conf_mode_55,
	.ssdt_exit_conf_mode = pnp_ssdt_exit_conf_mode_aa,
#endif
};

const struct pnp_mode_ops pnp_conf_mode_6767_aa = {
	.enter_conf_mode = pnp_enter_conf_mode_6767,
	.exit_conf_mode  = pnp_exit_conf_mode_aa,
#if CONFIG(HAVE_ACPI_TABLES)
	.ssdt_enter_conf_mode = pnp_ssdt_enter_conf_mode_6767,
	.ssdt_exit_conf_mode = pnp_ssdt_exit_conf_mode_aa,
#endif
};

const struct pnp_mode_ops pnp_conf_mode_7777_aa = {
	.enter_conf_mode = pnp_enter_conf_mode_7777,
	.exit_conf_mode  = pnp_exit_conf_mode_aa,
#if CONFIG(HAVE_ACPI_TABLES)
	.ssdt_enter_conf_mode = pnp_ssdt_enter_conf_mode_7777,
	.ssdt_exit_conf_mode = pnp_ssdt_exit_conf_mode_aa,
#endif
};

const struct pnp_mode_ops pnp_conf_mode_8787_aa = {
	.enter_conf_mode = pnp_enter_conf_mode_8787,
	.exit_conf_mode  = pnp_exit_conf_mode_aa,
#if CONFIG(HAVE_ACPI_TABLES)
	.ssdt_enter_conf_mode = pnp_ssdt_enter_conf_mode_8787,
	.ssdt_exit_conf_mode = pnp_ssdt_exit_conf_mode_aa,
#endif
};

const struct pnp_mode_ops pnp_conf_mode_a0a0_aa = {
	.enter_conf_mode = pnp_enter_conf_mode_a0a0,
	.exit_conf_mode  = pnp_exit_conf_mode_aa,
#if CONFIG(HAVE_ACPI_TABLES)
	.ssdt_enter_conf_mode = pnp_ssdt_enter_conf_mode_a0a0,
	.ssdt_exit_conf_mode = pnp_ssdt_exit_conf_mode_aa,
#endif
};

const struct pnp_mode_ops pnp_conf_mode_a5a5_aa = {
	.enter_conf_mode = pnp_enter_conf_mode_a5a5,
	.exit_conf_mode  = pnp_exit_conf_mode_aa,
#if CONFIG(HAVE_ACPI_TABLES)
	.ssdt_enter_conf_mode = pnp_ssdt_enter_conf_mode_a5a5,
	.ssdt_exit_conf_mode = pnp_ssdt_exit_conf_mode_aa,
#endif
};

const struct pnp_mode_ops pnp_conf_mode_870155_aa = {
	.enter_conf_mode = pnp_enter_conf_mode_870155aa,
	.exit_conf_mode  = pnp_exit_conf_mode_0202,
#if CONFIG(HAVE_ACPI_TABLES)
	.ssdt_enter_conf_mode = pnp_ssdt_enter_conf_mode_870155aa,
	.ssdt_exit_conf_mode = pnp_ssdt_exit_conf_mode_0202,
#endif
};

const struct pnp_mode_ops pnp_conf_mode_86805555 = {
	.enter_conf_mode = pnp_enter_conf_mode_86805555,
	.exit_conf_mode  = pnp_exit_conf_mode_0202,
#if CONFIG(HAVE_ACPI_TABLES)
	.ssdt_enter_conf_mode = pnp_ssdt_enter_conf_mode_86805555,
	.ssdt_exit_conf_mode = pnp_ssdt_exit_conf_mode_0202,
#endif
};
