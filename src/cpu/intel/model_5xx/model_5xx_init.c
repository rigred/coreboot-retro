/* SPDX-License-Identifier: GPL-2.0-only */

#include <device/device.h>
#include <cpu/cpu.h>
#include <cpu/x86/mtrr.h>
#include <cpu/intel/microcode.h>
#include <cpu/x86/cache.h>

static void model_5xx_init(struct device *dev)
{
	/* Turn on caching if we haven't already */
	enable_cache();
	x86_setup_mtrrs();
	x86_mtrr_check();
};

static struct device_operations cpu_dev_ops = {
	.init     = model_5xx_init,
};

/*
 */
static const struct cpu_device_id cpu_table[] = {

	{ X86_VENDOR_INTEL, 0x0513, CPUID_EXACT_MATCH_MASK }, /* Pentium, Socket 4, B1 */
	{ X86_VENDOR_INTEL, 0x0515, CPUID_EXACT_MATCH_MASK }, /* Pentium, Socket 4, C1 */
	{ X86_VENDOR_INTEL, 0x0517, CPUID_EXACT_MATCH_MASK }, /* Pentium, Socket 4, D1 */

	{ X86_VENDOR_INTEL, 0x0544, CPUID_EXACT_MATCH_MASK }, /* Pentium, Socket 5, A3 */
	{ X86_VENDOR_INTEL, 0x2544, CPUID_EXACT_MATCH_MASK }, /* Pentium, Socket 5, A3 */
	{ X86_VENDOR_INTEL, 0x0521, CPUID_EXACT_MATCH_MASK }, /* Pentium, Socket 5, B1 */
	{ X86_VENDOR_INTEL, 0x252C, CPUID_EXACT_MATCH_MASK }, /* Pentium, Socket 5, B1 */
	{ X86_VENDOR_INTEL, 0x0522, CPUID_EXACT_MATCH_MASK }, /* Pentium, Socket 5, B3 */
	{ X86_VENDOR_INTEL, 0x2522, CPUID_EXACT_MATCH_MASK }, /* Pentium, Socket 5, B3 */
	{ X86_VENDOR_INTEL, 0x0524, CPUID_EXACT_MATCH_MASK }, /* Pentium, Socket 5, B5 */
	{ X86_VENDOR_INTEL, 0x052C, CPUID_EXACT_MATCH_MASK }, /* Pentium, Socket 5, C0 */


	CPU_TABLE_END
};

static const struct cpu_driver driver __cpu_driver = {
	.ops      = &cpu_dev_ops,
	.id_table = cpu_table,
};
