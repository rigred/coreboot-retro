/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <device/device.h>
#include <cpu/cpu.h>
#include <cpu/x86/mtrr.h>
#include <cpu/intel/microcode.h>
#include <cpu/x86/cache.h>
#include <cpu/intel/l2_cache.h>
#include <cpu/x86/name.h>

static void model_65x_init(struct device *dev)
{
	char processor_name[49];

	/* Update the microcode */
	intel_update_microcode_from_cbfs();
	
	/* Initialize L2 cache */
	p6_configure_l2_cache();

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
	.init     = model_65x_init,
};

/*
 * Intel Pentium II Processor Specification Update
 * http://download.intel.com/design/PentiumII/specupdt/24333749.pdf
 *
 * Mobile Intel Pentium II Processor Specification Update
 * http://download.intel.com/design/intarch/specupdt/24388757.pdf
 *
 * Intel Pentium II Xeon Processor Specification Update
 * http://download.intel.com/support/processors/pentiumii/xeon/24377632.pdf
 */
static const struct cpu_device_id cpu_table[] = {
	{ X86_VENDOR_INTEL, 0x1632, CPUID_EXACT_MATCH_MASK }, /* PII Overdrive, tdB0 */
	{ X86_VENDOR_INTEL, 0x0650, CPUID_EXACT_MATCH_MASK }, /* PII/Celeron, dA0/mdA0/A0 */
	{ X86_VENDOR_INTEL, 0x0651, CPUID_EXACT_MATCH_MASK }, /* PII/Celeron, dA1/A1 */
	{ X86_VENDOR_INTEL, 0x0652, CPUID_EXACT_MATCH_MASK }, /* PII/Celeron/Xeon, dB0/mdB0/B0 */
	{ X86_VENDOR_INTEL, 0x0653, CPUID_EXACT_MATCH_MASK }, /* PII/Xeon, dB1/B1 */
	CPU_TABLE_END
};

static const struct cpu_driver driver __cpu_driver = {
	.ops      = &cpu_dev_ops,
	.id_table = cpu_table,
};
