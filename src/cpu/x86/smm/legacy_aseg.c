/* SPDX-License-Identifier: GPL-2.0-only */
#include <arch/cpuid.h>
#include <commonlib/helpers.h>
#include <console/console.h>
#include <cpu/x86/cache.h>
#include <cpu/x86/legacy_save_state.h>
#include <cpu/x86/legacy_aseg.h>
#include <cpu/x86/mp.h>
#include <cpu/x86/msr.h>
#include <cpu/x86/mtrr.h>
#include <cpu/x86/smm.h>

static volatile u8 relocated[CONFIG_MAX_CPUS];
_Static_assert(sizeof(legacy_smm_state_save_area_t) == 0x200, "legacy save size");
_Static_assert(offsetof(legacy_smm_state_save_area_t, smbase) == 0xf8, "legacy SMBASE");
_Static_assert(CONFIG_MAX_CPUS <= 2, "legacy ASEG currently supports one or two processors");

static void verify_cpu(void)
{
	const struct cpuid_result vendor = cpuid(0);
	const struct cpuid_result cpu = cpuid(1);
	const unsigned int family = (cpu.eax >> 8) & 0xf;
	const unsigned int model = (cpu.eax >> 4) & 0xf;
	/* This save-state/entry path is for Intel P6, not VIA C3 or modern family 6. */
	if (vendor.eax < 1 || vendor.ebx != 0x756e6547 ||
	    vendor.edx != 0x49656e69 || vendor.ecx != 0x6c65746e ||
	    family != 6 || (cpu.eax & 0x0fff0000) ||
	    !(cpu.edx & (1U << 5)) || !(cpu.edx & (1U << 9)) ||
	    !(cpu.edx & (1U << 12)))
		die("Legacy ASEG SMM requires an Intel P6 CPU with MSR/APIC/MTRR\n");
	switch (model) {
	case 1: case 3: case 5: case 6: case 7: case 8: case 10: case 11:
		break;
	default:
		die("Legacy ASEG SMM CPU model not supported\n");
	}
}

void legacy_aseg_check_cpu(void)
{
	verify_cpu();
	msr_t cap = rdmsr(MTRR_CAP_MSR);
	if (!(cap.lo & MTRR_CAP_FIX))
		die("Legacy ASEG SMM requires fixed MTRRs\n");
	msr_t control = rdmsr(MTRR_DEF_TYPE_MSR);
	msr_t fixed = rdmsr(MTRR_FIX_16K_A0000);
	if ((control.lo & (MTRR_DEF_TYPE_EN | MTRR_DEF_TYPE_FIX_EN)) !=
	    (MTRR_DEF_TYPE_EN | MTRR_DEF_TYPE_FIX_EN) || fixed.lo || fixed.hi)
		die("Legacy ASEG SMM requires fixed UC A/B segments on every CPU\n");
}

void legacy_aseg_prepare(void)
{
	/* Reject unsupported CPUs before the first MTRR MSR access. */
	verify_cpu();
	for (int cpu = 0; cpu < CONFIG_MAX_CPUS; cpu++)
		relocated[cpu] = 0;
	x86_setup_mtrrs();
	x86_mtrr_check();
	legacy_aseg_check_cpu();
	wbinvd();
}

void legacy_aseg_info(uintptr_t *base, size_t *size, size_t *save_size)
{
	*base = LEGACY_ASEG_BASE;
	*size = LEGACY_ASEG_SIZE;
	*save_size = sizeof(legacy_smm_state_save_area_t);
}

void legacy_aseg_trigger(void)
{
	legacy_aseg_check_cpu();
	/* The common helper serializes CPUs sharing reset SMBASE 30000h. */
	smm_initiate_relocation();
}

void legacy_aseg_relocate(int cpu, uintptr_t current, uintptr_t target)
{
	legacy_smm_state_save_area_t *state =
		(void *)(current + SMM_DEFAULT_SIZE - sizeof(*state));
	if (cpu < 0 || cpu >= CONFIG_MAX_CPUS || !(state->smm_revision & (1U << 17)) ||
	    !IS_ALIGNED(target, 16) || target < LEGACY_ASEG_BASE - SMM_ENTRY_OFFSET ||
	    target > LEGACY_ASEG_BASE + LEGACY_ASEG_SIZE - SMM_DEFAULT_SIZE)
		die("Legacy ASEG SMM relocation/state format rejected\n");
	state->smbase = target;
	relocated[cpu] = 1;
}

void legacy_aseg_complete(void)
{
	for (int cpu = 0; cpu < CONFIG_MAX_CPUS; cpu++)
		if (!relocated[cpu])
			die("Legacy ASEG SMM did not relocate every configured CPU\n");
	wbinvd();
}
