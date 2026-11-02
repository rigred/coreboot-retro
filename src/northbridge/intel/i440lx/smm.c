/* SPDX-License-Identifier: GPL-2.0-only */
#include <console/console.h>
#include <cpu/x86/legacy_aseg.h>
#include <device/pci_ops.h>
#include <southbridge/intel/i82371eb/smi.h>
#include "i440lx.h"
#include "smm.h"
#include "errata.h"

#define SMRAM_LOCKED 0x1a

static struct lx_smram_guard smram_guard;

void i440lx_smm_prepare(void)
{
	i82371eb_smm_prepare();
	if (pci_s_read_config8(NB, SMRAM) & 0x10)
		die("440LX SMRAM already locked; cold power cycle required\n");
	legacy_aseg_prepare();
	/* PM location is bound by sconfig, not a runtime southbridge search. */
	pci_devfn_t isa = pcidev_bdf(DEV_PTR(piix4_pm)) & ~PCI_DEV(0, 0, 7);
	lx_smram_open(&smram_guard, isa);
}

void i440lx_smm_complete(void)
{
	legacy_aseg_complete();
	lx_smram_end(&smram_guard);
	pci_s_write_config8(NB, SMRAM, SMRAM_LOCKED);
	if (pci_s_read_config8(NB, SMRAM) != SMRAM_LOCKED)
		die("440LX SMRAM lock failed\n");
	i82371eb_smm_mark_ready();
}
