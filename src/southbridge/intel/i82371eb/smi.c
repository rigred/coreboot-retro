/* SPDX-License-Identifier: GPL-2.0-only */
#include <arch/io.h>
#include <console/console.h>
#include <cpu/x86/smm.h>
#include <device/device.h>
#include <device/pci_ops.h>
#include <device/pci_def.h>
#include "i82371eb.h"
#include "smi.h"

#define DEVACTB 0x58
#define APMC_ENABLE (1U << 25)

#if !ENV_SMM
static const struct device *pm_device(void)
{
	const struct device *pm = DEV_PTR(piix4_pm);

	/* sconfig binds this alias at build time; no device-list search. */
	if (!pm || !pm->enabled || !pm->bus ||
	    pm->path.type != DEVICE_PATH_PCI || PCI_FUNC(pm->path.pci.devfn) != 3)
		die("PIIX4 SMM requires an enabled piix4_pm PCI alias\n");
	if (pci_s_read_config32(pcidev_bdf(pm), PCI_VENDOR_ID) != 0x71138086)
		die("PIIX4 SMM PM alias does not identify PIIX4 hardware\n");
	return pm;
}

void i82371eb_smm_prepare(void)
{
	const struct device *dev = pm_device();
	const struct device *usb_dev = DEV_PTR(piix4_usb);
	const pci_devfn_t pm = pcidev_bdf(dev);

	/* Both aliases must describe functions of the same physical PIIX4. */
	if (!usb_dev || usb_dev->path.type != DEVICE_PATH_PCI ||
	    usb_dev->bus != dev->bus || PCI_FUNC(usb_dev->path.pci.devfn) != 2 ||
	    PCI_SLOT(usb_dev->path.pci.devfn) != PCI_SLOT(dev->path.pci.devfn))
		die("PIIX4 SMM USB alias is not the PM function's sibling\n");
	const pci_devfn_t usb = pcidev_bdf(usb_dev);
	const u32 usb_id = pci_s_read_config32(usb, PCI_VENDOR_ID);

	if (usb_id != 0x71128086 && usb_id != 0xffffffff)
		die("PIIX4 SMM found an unexpected USB sibling\n");
	pci_s_write_config32(pm, PMBA, DEFAULT_PMBASE | 1);
	pci_s_write_config8(pm, PMREGMISC,
		pci_s_read_config8(pm, PMREGMISC) | PMIOSE);
	pci_s_write_config16(pm, PCI_COMMAND,
		pci_s_read_config16(pm, PCI_COMMAND) | PCI_COMMAND_IO);
	outl(inl(DEFAULT_PMBASE + GLBCTL) & ~(SMI_EN | EOS), DEFAULT_PMBASE + GLBCTL);
	pci_s_write_config32(pm, DEVACTB,
		pci_s_read_config32(pm, DEVACTB) & ~APMC_ENABLE);
	outw(0, DEFAULT_PMBASE + GLBEN);
	outw(0, DEFAULT_PMBASE + PMEN);
	outw(0, DEFAULT_PMBASE + GPEN);
	outl(0, DEFAULT_PMBASE + DEVCTL);
	/* No USB/KBC SMI emulation in this implementation. Keep normal PIRQD. */
	if (usb_id == 0x71128086)
		pci_s_write_config16(usb, 0xc0,
			pci_s_read_config16(usb, 0xc0) & ~(0x009f | 0x8f00));
	outw(PIIX4_APM_STS, DEFAULT_PMBASE + GLBSTS); /* Only this W1C cause. */
}

void i82371eb_smm_enable_apmc(void)
{
	/* Called after ordinary PM device initialization, not during CPU init. */
	i82371eb_smm_prepare();
	const pci_devfn_t pm = pcidev_bdf(DEV_PTR(piix4_pm));
	pci_s_write_config32(pm, DEVACTB,
		pci_s_read_config32(pm, DEVACTB) | APMC_ENABLE);
	outl(inl(DEFAULT_PMBASE + GLBCTL) | SMI_EN | EOS, DEFAULT_PMBASE + GLBCTL);
}
#else
static bool probing = true;

void cpu_smi_handler(void)
{
	/* Boot-only self-SMI acknowledgement before APMC is enabled. */
	if (probing)
		outb(0xa5, 0xb3);
}

void southbridge_smi_handler(void)
{
	const u16 status = inw(DEFAULT_PMBASE + GLBSTS);
	if (status & PIIX4_APM_STS) {
		u16 control = inw(DEFAULT_PMBASE + PMCNTRL);
		switch (inb(APM_CNT)) {
		case APM_CNT_NOOP_SMI:
			probing = false;
			outb(0x5a, 0xb3); /* APMS port B3, not GLBSTS bit mask. */
			break;
		case APM_CNT_ACPI_ENABLE:
			outw(control | SCI_EN, DEFAULT_PMBASE + PMCNTRL);
			break;
		case APM_CNT_ACPI_DISABLE:
			outw(control & ~SCI_EN, DEFAULT_PMBASE + PMCNTRL);
			break;
		default:
			break; /* Unsupported commands have no privileged side effects. */
		}
		outw(PIIX4_APM_STS, DEFAULT_PMBASE + GLBSTS);
	}
}

void southbridge_smi_set_eos(void)
{
	outl(inl(DEFAULT_PMBASE + GLBCTL) | EOS, DEFAULT_PMBASE + GLBCTL);
}
#endif
