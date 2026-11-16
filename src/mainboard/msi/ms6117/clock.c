/* SPDX-License-Identifier: GPL-2.0-only */
#include <console/console.h>
#include <cpu/x86/tsc.h>
#include <device/smbus_host.h>
#include <drivers/clock/imi_sc643/sc643.h>
#include <northbridge/intel/i440lx/raminit.h>
#include <option.h>
#include <southbridge/intel/i82371eb/block_write.h>
#include <southbridge/intel/i82371eb/i82371eb.h>

static enum sc643_mode selected_mode;
static bool clock_selected;

unsigned int mainboard_sdram_clock_ps(void)
{
	unsigned int mode = SC643_HARDWARE_STRAPS;
	unsigned int clock_ps = CONFIG_MS6117_SC643_STRAP_CLOCK_PS;

	if (!CONFIG(MS6117_SC643_QUALIFIED))
		die("SC643 board clock contract has not been qualified\n");
	if (get_uint_option("clock_policy_version", 0) == 0x6401 &&
	    get_uint_option("overclocking", 0)) {
		mode = get_uint_option("bus_clock", 0);
		if (mode > 2 || (mode && !CONFIG(MS6117_SC643_OVERCLOCK)))
			die("SC643 CMOS clock request is not qualified\n");
	}
	if (mode == 1)
		clock_ps = CONFIG_MS6117_SC643_68_CLOCK_PS;
	if (mode == 2)
		clock_ps = CONFIG_MS6117_SC643_75_CLOCK_PS;
	selected_mode = mode;
	clock_selected = true;
	return clock_ps;
}

void mainboard_sdram_clock_apply(void)
{
	/* Keep every clock enabled until its PCB loads have been traced.
	 * Other boards can supply different masks to the same pure driver.
	 */
	static const struct sc643_config outputs = {
		.cpu = SC643_CPU_MASK,
		.pci = SC643_PCI_MASK,
		.sdram = SC643_SDRAM_LOW_MASK | (SC643_SDRAM_HIGH_MASK << 8),
		.ref = SC643_REF_MASK,
	};
	uint8_t image[SC643_BYTES];
	size_t count = CONFIG(MS6117_SC643_PROGRAM_OUTPUTS) ? sizeof(image) : 1;

	if (!clock_selected)
		die("SC643 clock applied without a validated clock selection\n");
	if (sc643_build_image(selected_mode, &outputs, image) != CB_SUCCESS)
		die("SC643 invalid output image\n");
	/* Spread spectrum is reserved. cpu_ratio is a separate CPU/board
	 * request; the SC643 has no multiplier register and must not consume it.
	 */
	printk(BIOS_NOTICE, "SC643 applying mode %u (not readback)\n", selected_mode);
	/* Command=0 and count are mandatory framing, NOT a register address.
	 * The PLL's first data byte is always byte zero. Do not retry failures.
	 */
	if (i82371eb_block_write(smbus_base(), DEFAULT_PMBASE + PMTMR,
		SC643_ADDRESS, 0, image, count))
		die("SC643 write failed; clock state uncertain, stopping before DRAM\n");
	/* No TSC-based delays until settling and PIT recalibration are complete.
	 * 10 ms is a qualification starting point, not a datasheet lock guarantee.
	 */
	if (i82371eb_pm_wait(DEFAULT_PMBASE + PMTMR, 35796))
		die("SC643 stable timer failed\n");
	if (!tsc_recalibrate())
		die("SC643 TSC recalibration failed\n");
	clock_selected = false;
}
