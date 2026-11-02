/* SPDX-License-Identifier: GPL-2.0-only */
#include <arch/io.h>
#include <console/console.h>
#include <device/pci_def.h>
#include <device/pci_ops.h>
#include "i82371eb.h"

#define GENCFG_GPI8 (1U << 23)
#define PCNTRL_THROTTLE (1U << 4)
#define PCNTRL_CLOCK_CONTROL (1U << 9)

/* Reuse the PM location already supplied by early_pm.c; no new PCI search. */
static pci_devfn_t lx_isa = PCI_DEV_INVALID;

pci_devfn_t i82371eb_lx_isa(void)
{
	return lx_isa;
}

void i82371eb_lx_disable_throttling(pci_devfn_t pm)
{
	/* Only linked on LX. Keep other users of the shared PIIX4 driver intact.
	 * The PM function comes from the caller; derive its physical ISA sibling.
	 */
	pci_devfn_t isa = pm & ~PCI_DEV(0, 0, 7);
	if ((pm & PCI_DEV(0, 0, 7)) != PCI_DEV(0, 0, 3) ||
	    pci_s_read_config32(isa, PCI_VENDOR_ID) != 0x71108086)
		die("LX clock workaround requires a PIIX4 PM/ISA pair\n");

	lx_isa = isa;

	/* 297655-005 erratum 4. THRM_EN only masks SCI/SMI; it does NOT stop
	 * automatic thermal throttling (290562-001 section 11.2.1). Select GPI8
	 * instead of THRM# using its documented pin mux (section 4.1.16).
	 * This removes this input's thermal protection: board cooling/shutdown
	 * must not depend on PIIX4 STPCLK# modulation.
	 */
	pci_s_write_config32(isa, GENCFG,
		pci_s_read_config32(isa, GENCFG) | GENCFG_GPI8);
	if (!(pci_s_read_config32(isa, GENCFG) & GENCFG_GPI8))
		die("LX could not disable PIIX4 thermal throttling\n");
	uint32_t control = inl(DEFAULT_PMBASE + PCNTRL);
	control &= ~(PCNTRL_THROTTLE | PCNTRL_CLOCK_CONTROL);
	outl(control, DEFAULT_PMBASE + PCNTRL);
	if (inl(DEFAULT_PMBASE + PCNTRL) & (PCNTRL_THROTTLE | PCNTRL_CLOCK_CONTROL))
		die("LX could not disable PIIX4 software clock control\n");
}
