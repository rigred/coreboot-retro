/* SPDX-License-Identifier: GPL-2.0-only */
#include <arch/cpuid.h>
#include <console/console.h>
#include <commonlib/console/post_codes.h>
#include <cpu/x86/msr.h>
#include <cpu/x86/mtrr.h>
#include <device/mmio.h>
#include <device/pci_ops.h>
#include "i440lx.h"
#include "raminit.h"
#include "sdram_policy.h"
#include "errata.h"
#include <southbridge/intel/i82371eb/i82371eb.h>

static void require_uncached_ram(uintptr_t top)
{
	/* Never flush/invalidate CAR to change cache policy during RAM init. */
	msr_t cap = rdmsr(MTRR_CAP_MSR);
	msr_t def = rdmsr(MTRR_DEF_TYPE_MSR);
	if (!(def.lo & MTRR_DEF_TYPE_EN) || (def.lo & MTRR_DEF_TYPE_MASK))
		die_with_post_code(POSTCODE_RAM_FAILURE, "RAM init requires default UC MTRRs\n");
	if (def.lo & MTRR_DEF_TYPE_FIX_EN) {
		if (!(cap.lo & MTRR_CAP_FIX))
			die_with_post_code(POSTCODE_RAM_FAILURE, "Fixed MTRRs unavailable\n");
		static const unsigned int fixed[] = {
			0x250, 0x258, 0x259, 0x268, 0x269, 0x26a, 0x26b,
			0x26c, 0x26d, 0x26e, 0x26f,
		};
		for (unsigned int i = 0; i < ARRAY_SIZE(fixed); i++) {
			msr_t value = rdmsr(fixed[i]);
			if (value.lo || value.hi)
				die_with_post_code(POSTCODE_RAM_FAILURE, "RAM init fixed range is cached\n");
		}
	}
	unsigned int count = cap.lo & MTRR_CAP_VCNT;
	for (unsigned int i = 0; i < count; i++) {
		msr_t base = rdmsr(MTRR_PHYS_BASE(i));
		msr_t mask = rdmsr(MTRR_PHYS_MASK(i));
		if ((mask.lo & MTRR_PHYS_MASK_VALID) && !(base.hi & mask.hi) &&
		    (base.lo & mask.lo & ~0xfffU) < top && (base.lo & 0xff))
			die_with_post_code(POSTCODE_RAM_FAILURE, "RAM init variable range is cached\n");
	}
}

static void clear_qwords(uintptr_t start, uintptr_t end)
{
	/* UC full 64-bit stores avoid ECC read-merge-write of uninitialized data.
	 * No compiler-generated memset, split dword stores or cache-line fills.
	 * The optional ECC path explicitly requires MMX before executing this.
	 */
	__asm__ __volatile__(
		"pxor %%mm0, %%mm0\n"
		"1: movq %%mm0, (%0)\n"
		"add $8, %0\n"
		"cmp %1, %0\n"
		"jb 1b\n"
		"emms\n"
		: "+r" (start) : "r" (end) : "memory", "cc");
}

static void initialize_ecc(const struct lx_sdram_plan *p, unsigned int mode)
{
	if (!CONFIG(I440LX_ECC_WIRED) || !CONFIG(CPU_INTEL_SLOT_1) || !p->all_ecc ||
	    !(cpuid_edx(1) & (1U << 23)))
		die_with_post_code(POSTCODE_RAM_FAILURE, "ECC requires qualified wiring, ECC ranks and MMX\n");
	uintptr_t top = (uintptr_t)p->total_mb << 20;
	require_uncached_ram(top);
	struct lx_smram_guard guard = {0};
	printk(BIOS_INFO, "Initializing %u MiB ECC check bits with full QWord writes\n", p->total_mb);
	/* Erratum 6: expose the A/B backing DRAM only after blocking PCI and
	 * ISA/DMA VGA writes. The guard always returns with DOPEN clear.
	 */
	lx_smram_open(&guard, i82371eb_lx_isa());
	uint8_t errcmd = pci_read_config8(NB, ERRCMD);
	pci_write_config8(NB, ERRCMD, errcmd & ~ERRCMD_MEMORY_MASK);
	uint8_t old_status = pci_read_config8(NB, ERRSTS) & ERRSTS_ECC_FLAGS;
	if (old_status)
		pci_write_config8(NB, ERRSTS, old_status);
	uint16_t cfg = pci_read_config16(NB, PACCFG) & ~(PACCFG_DDIM_MASK | PACCFG_ECC_TEST);
	uint16_t mode_bits = mode == 1 ? PACCFG_DDIM_EC : PACCFG_DDIM_ECC;
	/* PACCFG 01 = EC, 11 = ECC; 10 is reserved and is never written. */
	pci_write_config16(NB, PACCFG, cfg | mode_bits);
	if ((pci_read_config16(NB, PACCFG) & (PACCFG_DDIM_MASK | PACCFG_ECC_TEST)) != mode_bits) {
		lx_smram_end(&guard);
		die_with_post_code(POSTCODE_RAM_FAILURE, "ECC mode readback failed\n");
	}
	clear_qwords(0, top);
	/* PCI reads serialize completion of UC stores; do not WBINVD the CAR stack. */
	pci_read_config16(NB, PACCFG);
	lx_smram_end(&guard);
	uint8_t status = pci_read_config8(NB, ERRSTS) & ERRSTS_ECC_FLAGS;
	if (status)
		die_with_post_code(POSTCODE_RAM_FAILURE, "ECC error during check-bit initialization\n");
	/* Leave ECC SERR routing masked: NMI/SMI wiring/policy needs board review.
	 * ERRSTS still records errors; ECC mode still corrects single-bit errors.
	 */
	printk(BIOS_INFO, "LX %s active; ECC error status available at PCI 00:00.0 reg 91h\n",
		mode == 1 ? "error detection (EC)" : "ECC correction");
}

static void address_test(const struct lx_sdram_plan *p)
{
	uintptr_t addresses[224];
	unsigned int count = 0;
	uintptr_t start = 0;
	for (unsigned int row = 0; row < 8; row++) {
		uintptr_t end = (uintptr_t)p->drb[row] << 23;
		if (end > start) {
			addresses[count++] = start;
			for (uintptr_t offset = 8; offset < end - start; offset <<= 1)
				addresses[count++] = start + offset;
			addresses[count++] = end - 8;
		}
		start = end;
	}
	/* Before CBMEM/ramstage uses DRAM. Write all samples before reading any,
	 * so aliasing between ranks as well as within a rank is observable.
	 */
	for (unsigned int pass = 0; pass < 2; pass++) {
		for (unsigned int i = 0; i < count; i++)
			write32((void *)addresses[i], (0x6d2b79f5U * (i + 1)) ^ (pass ? ~0U : 0));
		for (unsigned int i = 0; i < count; i++)
			if (read32((void *)addresses[i]) !=
			    ((0x6d2b79f5U * (i + 1)) ^ (pass ? ~0U : 0)))
				die_with_post_code(POSTCODE_RAM_FAILURE, "SDRAM address/data sample failed\n");
	}
	printk(BIOS_INFO, "LX SDRAM rank address samples passed (%u locations)\n", count);
}

void lx_sdram_finish(const struct lx_sdram_plan *plan, unsigned int ecc_mode)
{
	if (ecc_mode)
		initialize_ecc(plan, ecc_mode);
	if (CONFIG(I440LX_RAM_ADDRESS_TEST)) {
		require_uncached_ram((uintptr_t)plan->total_mb << 20);
		address_test(plan);
	}
	if (ecc_mode && (pci_read_config8(NB, ERRSTS) & ERRSTS_ECC_FLAGS))
		die_with_post_code(POSTCODE_RAM_FAILURE, "ECC errors during SDRAM verification\n");
}
