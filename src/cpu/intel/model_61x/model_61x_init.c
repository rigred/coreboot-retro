/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <device/device.h>
#include <cpu/cpu.h>
#include <cpu/x86/mtrr.h>
#include <cpu/intel/microcode.h>
#include <cpu/x86/cache.h>
#include <cpu/intel/l2_cache.h>
#include <cpu/x86/name.h>

static void model_61x_init(struct device *dev)
{
	char processor_name[49];

	/* Update the microcode */
	intel_update_microcode_from_cbfs();
	
	/* Print processor name */
	fill_processor_name(processor_name);
	printk(BIOS_INFO, "CPU: %s.\n", processor_name);

	/* Turn on caching if we haven't already */
	enable_cache();

	/* Setup MTRRs */
	x86_setup_mtrrs();
	x86_mtrr_check();
};

static struct device_operations cpu_dev_ops = {
	.init     = model_61x_init,
};

/*
 * Intel Pentium Pro Processor Specification Update
 * http://download.intel.com/design/archives/processors/pro/docs/24268935.pdf
 */
static const struct cpu_device_id cpu_table[] = {
	{ X86_VENDOR_INTEL, 0x0610 }, /* Pentium Pro ES, A0 Stepping 0, Abandon Ship */
	{ X86_VENDOR_INTEL, 0x0611 }, /* Pentium Pro, B0 Stepping 1 */
	{ X86_VENDOR_INTEL, 0x0612 }, /* Pentium Pro, C0 Stepping 2 */
	{ X86_VENDOR_INTEL, 0x0616 }, /* Pentium Pro, sA0 Stepping 6 */
	{ X86_VENDOR_INTEL, 0x0617 }, /* Pentium Pro, sA1 Stepping 7 */
	{ X86_VENDOR_INTEL, 0x0619 }, /* Pentium Pro, sB1 Stepping 9 */
	{ 0, 0 },
};

static const struct cpu_driver driver __cpu_driver = {
	.ops      = &cpu_dev_ops,
	.id_table = cpu_table,
};
