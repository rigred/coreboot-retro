/* SPDX-License-Identifier: GPL-2.0-only */
#include <console/console.h>
#include <cpu/x86/legacy_aseg.h>
#include <device/pci_ops.h>
#include <southbridge/intel/i82371eb/smi.h>
#include "i440lx.h"
#include "smm.h"

#define SMRAM_OPEN 0x4a
#define SMRAM_CLOSED 0x0a
#define SMRAM_LOCKED 0x1a

void i440lx_smm_prepare(void)
{
	i82371eb_smm_prepare();
	if (pci_s_read_config8(NB, SMRAM) & 0x10)
		die("440LX SMRAM already locked; cold power cycle required\n");
	legacy_aseg_prepare();
	pci_s_write_config8(NB, SMRAM, SMRAM_OPEN);
	if (pci_s_read_config8(NB, SMRAM) != SMRAM_OPEN)
		die("440LX SMRAM open failed\n");
}

void i440lx_smm_complete(void)
{
	legacy_aseg_complete();
	pci_s_write_config8(NB, SMRAM, SMRAM_CLOSED);
	if (pci_s_read_config8(NB, SMRAM) != SMRAM_CLOSED)
		die("440LX SMRAM close failed\n");
	pci_s_write_config8(NB, SMRAM, SMRAM_LOCKED);
	if (pci_s_read_config8(NB, SMRAM) != SMRAM_LOCKED)
		die("440LX SMRAM lock failed\n");
	i82371eb_smm_mark_ready();
}
