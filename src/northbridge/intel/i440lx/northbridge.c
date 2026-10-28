/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <cpu/x86/mp.h>
#include <device/pci_ops.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <stdint.h>
#include <option.h>
#include "i440lx.h"
#include "smm.h"
#include <cpu/x86/legacy_aseg.h>

/* Program APSIZE before the generic PCI code probes the aperture BAR. */
static void northbridge_read_resources(struct device *dev)
{
	static const uint8_t sizes[] = { 0x3f, 0x3e, 0x3c, 0x38, 0x30, 0x20, 0x00 };
	unsigned int choice = get_uint_option("agp_aperture", AGP_APERTURE_DEFAULT);
	uint8_t reg;

	if (choice >= ARRAY_SIZE(sizes)) {
		printk(BIOS_WARNING, "Invalid AGP aperture option %u; using 64 MiB\n", choice);
		choice = AGP_APERTURE_DEFAULT;
	}

	reg = pci_read_config8(dev, APSIZE);
	pci_write_config8(dev, APSIZE, (reg & 0xc0) | sizes[choice]);
	/* Newly writable APBASE bits have an undefined value after APSIZE changes. */
	pci_write_config32(dev, APBASE, 0);
	printk(BIOS_INFO, "440LX AGP aperture: %u MiB (APSIZE=%02x)\n",
	       4U << choice, pci_read_config8(dev, APSIZE));
	pci_dev_read_resources(dev);
}

static void northbridge_init(struct device *dev)
{
	printk(BIOS_SPEW, "Northbridge Init\n");
}

static struct device_operations northbridge_operations = {
	.read_resources   = northbridge_read_resources,
	.set_resources    = pci_dev_set_resources,
	.enable_resources = pci_dev_enable_resources,
	.init             = northbridge_init,
};

static const struct pci_driver northbridge_driver __pci_driver = {
	.ops = &northbridge_operations,
	.vendor = PCI_VID_INTEL,
	.device = PCI_DID_INTEL_82443LX_0,
};

static void i440lx_domain_read_resources(struct device *dev)
{
	struct device *mc_dev;
	uint32_t pci_tolm;

	pci_domain_read_resources(dev);

	pci_tolm = find_pci_tolm(dev->link_list);
	printk(BIOS_SPEW, "Found PCI tolm at           %.8x\n", pci_tolm);
	printk(BIOS_SPEW, "Found PCI tolm at           %dMB\n", pci_tolm >> 20);
	
	mc_dev = dev->link_list->children;
	if (mc_dev) {
		unsigned long tomk, tolmk;
		int idx;

		/* Figure out which areas are/should be occupied by RAM. The
		 * value of the highest DRB denotes the end of the physical
		 * memory (in units of 8MB).
		 */
		tomk = ((unsigned long)pci_read_config8(mc_dev, DRB7));

		/* Convert to KB. */
		tomk *= (8 * 1024);

		printk(BIOS_DEBUG, "Setting RAM size to %ld MB\n", tomk / 1024);

		/* Compute the top of low memory. */
		tolmk = pci_tolm / 1024;

		printk(BIOS_DEBUG, "Top of Low Memory: %ld bytes \n", tolmk);

		if (tolmk >= tomk) {
			/* The PCI hole does not overlap the memory. */
			tolmk = tomk;
		}

		/* Report the memory regions. */
		idx = 10;
		ram_range(dev, idx++, 0, 0xa0000);
		ram_from_to(dev, idx++, 0xc0000, tolmk * KiB);
		printk(BIOS_DEBUG, "Memory Range Index: %d \n", idx);
	}
}

static struct device_operations pci_domain_ops = {
	.read_resources		= i440lx_domain_read_resources,
	.set_resources		= pci_domain_set_resources,
	.scan_bus		= pci_domain_scan_bus,
};

static int get_cpu_count(void)
{
	return CONFIG_MAX_CPUS;
}

static const struct mp_ops mp_ops = {
#if CONFIG(I440LX_SMM)
	.pre_mp_init = i440lx_smm_prepare,
	.get_smm_info = legacy_aseg_info,
	.per_cpu_smm_trigger = legacy_aseg_trigger,
	.relocation_handler = legacy_aseg_relocate,
	.post_mp_init = i440lx_smm_complete,
#endif
	.get_cpu_count = get_cpu_count,
};

void mp_init_cpus(struct bus *cpu_bus)
{
	/* TODO: Handle mp_init_with_smm failure? */
	if (mp_init_with_smm(cpu_bus, &mp_ops) != CB_SUCCESS)
		die("440LX CPU initialization failed\n");
}

static struct device_operations cpu_bus_ops = {
	.read_resources   = noop_read_resources,
	.set_resources    = noop_set_resources,
	.init             = mp_cpu_bus_init,
};

static void enable_dev(struct device *dev)
{
	/* Set the operations if it is a special bus type */
	if (dev->path.type == DEVICE_PATH_DOMAIN) {
		dev->ops = &pci_domain_ops;
	}
	else if (dev->path.type == DEVICE_PATH_CPU_CLUSTER) {
		dev->ops = &cpu_bus_ops;
	}
}

struct chip_operations northbridge_intel_i440lx_ops = {
	CHIP_NAME("Intel 82443LX (440LX) Northbridge")
	.enable_dev = enable_dev,
};
