/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef CPU_X86_LEGACY_ASEG_H
#define CPU_X86_LEGACY_ASEG_H
#include <types.h>

/* Full legacy A/B window; smaller C-segment windows are not supported. */
#define LEGACY_ASEG_BASE 0xa0000
#define LEGACY_ASEG_SIZE 0x20000

/* Platform masks SMI sources and checks SMRAM is unlocked before prepare.
 * Open SMRAM after prepare. Use info/trigger/relocate as existing mp_ops hooks.
 * Complete verifies all CPUs; platform then closes and locks SMRAM, verifies
 * readback, probes permanent entry and enables only supported SMI sources.
 * No PCI, PM register or southbridge assumptions live in these helpers.
 */
void legacy_aseg_prepare(void);
void legacy_aseg_info(uintptr_t *base, size_t *size, size_t *save_size);
void legacy_aseg_check_cpu(void);
void legacy_aseg_trigger(void);
void legacy_aseg_relocate(int cpu, uintptr_t current, uintptr_t target);
void legacy_aseg_complete(void);
#endif
