/* SPDX-License-Identifier: GPL-2.0-only */
#include <arch/io.h>
#include <bootstate.h>
#include <console/console.h>
#include <cpu/x86/lapic.h>
#include <cpu/x86/legacy_aseg.h>
#include <cpu/x86/mp.h>
#include <cpu/x86/smm.h>
#include <delay.h>
#include <smp/spinlock.h>
#include "smi.h"

static bool ready;

void i82371eb_smm_mark_ready(void)
{
	/* Called only after chipset SMRAM close/lock readbacks succeeded. */
	ready = true;
}

DECLARE_SPIN_LOCK(probe_lock);

static void smm_cpu_probe(void *unused)
{
	spin_lock(&probe_lock);
	legacy_aseg_check_cpu();
	outb(0xff, 0xb3);
	smm_initiate_relocation(); /* Now enters the permanent per-CPU stub. */
	for (int i = 0; i < 10000; i++) {
		if (inb(0xb3) == 0xa5) {
			printk(BIOS_INFO, "PIIX4 legacy SMM permanent entry passed on APIC %u\n", lapicid());
			spin_unlock(&probe_lock);
			return;
		}
		udelay(10);
	}
	die("PIIX4 legacy permanent SMM CPU probe failed\n");
}

static void smm_final_enable(void *unused)
{
	if (!ready)
		die("PIIX4 legacy SMM initialization incomplete\n");
	if (mp_run_on_all_cpus_synchronously(smm_cpu_probe, NULL) != CB_SUCCESS)
		die("PIIX4 legacy SMM AP probe did not complete\n");
	i82371eb_smm_enable_apmc();
	outb(0xff, 0xb3);
	outb(APM_CNT_NOOP_SMI, APM_CNT);
	for (int i = 0; i < 10000; i++) {
		if (inb(0xb3) == 0x5a) {
			printk(BIOS_INFO, "PIIX4 legacy SMM software-SMI test passed\n");
			return;
		}
		udelay(10);
	}
	die("PIIX4 legacy SMM software-SMI test failed\n");
}
BOOT_STATE_INIT_ENTRY(BS_POST_DEVICE, BS_ON_ENTRY, smm_final_enable, NULL);
