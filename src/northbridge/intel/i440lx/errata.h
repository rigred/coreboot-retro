/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef NORTHBRIDGE_INTEL_I440LX_ERRATA_H
#define NORTHBRIDGE_INTEL_I440LX_ERRATA_H

#include <device/pci_type.h>
#include <stdint.h>

/* Intel 297655-005, erratum 6. Only for controlled pre-payload intervals. */
struct lx_smram_guard {
	pci_devfn_t isa;
	uint8_t masters[32];
	uint8_t tom;
	uint8_t active;
};

void lx_smram_close(void);
void lx_smram_open(struct lx_smram_guard *guard, pci_devfn_t isa);
void lx_smram_end(struct lx_smram_guard *guard);
void lx_check_dimm_errata(void);

#endif
