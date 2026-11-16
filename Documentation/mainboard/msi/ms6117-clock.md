# MS-6117 SC643 clocks and SDR timing

The IMI SC643 is **write-only**. It cannot report its registers, active frequency
or latched jumper straps over SMBus. Pins FTS/S2/S1/S0/MODE are sampled at true
power-on and subsequently become clocks or other functions. No readable GPIO
connection is assumed. Configured periods are measurement-based board contracts,
not automatic clock detection.

## Configuration

PLL programming is off by default. The working inherited RAM initializer is
also retained by default because the experimental LX geometry policy still
rejects the 128 MiB ranks found in the working 256 MiB DIMMs. Use supported
smaller DIMMs for initial SPD/PLL development until that separate limitation
has been resolved and tested.

```config
CONFIG_VENDOR_MSI=y
CONFIG_BOARD_MSI_MS6117=y
CONFIG_USE_OPTION_TABLE=y
CONFIG_I440LX_SPD_RAMINIT=y
CONFIG_MS6117_SC643=y
# Deliberately unset until board qualification:
# CONFIG_MS6117_SC643_QUALIFIED is not set
# CONFIG_MS6117_SC643_OVERCLOCK is not set
# CONFIG_MS6117_SC643_PROGRAM_OUTPUTS is not set
```

With the feature enabled but the qualification flag unset, firmware stops before
its first clock write. Selecting the feature does not certify board wiring or
live switching. Keep the entire feature off for normal builds until qualified.

The board selects the hidden generic CMOS-backend preference; explicit profile
choices still take precedence. Compression is preferred and embedded build
configuration is omitted. Optional SPD, SMM and ECC are not forcibly selected.
No SDR feature variables are added to `src/device/dram/Kconfig`.

## Sequence

1. Existing early PIIX4 code enables SMBus and PM decode and applies LX errata.
2. `mainboard_sdram_clock_ps()` checks qualification, snapshots the CMOS request
   and supplies the minimum period used for the plan. It performs no PLL I/O.
3. The LX adapter reads SPD, decodes it and validates the entire controller plan.
   Invalid geometry, checksum, timing or EEPROM reads stop before clock apply.
4. `mainboard_sdram_clock_apply()` builds and transmits the cached request only
   after validation, before controller register programming and SDRAM commands.
5. A stable PM timer supplies settling time; TSC is recalibrated with the PIT
   before subsequent TSC-based delays. A transfer failure stops initialization;
   it never retries a partly accepted write or assumes stock frequency survived.

Other LX boards use weak constant-clock/no-op defaults and link no SC643 code.
The image builder lives in `drivers/clock/imi_sc643`; it has no SMBus, CMOS or
board dependency. The PIIX4 transfer helper takes I/O ports, not a PCI slot. It
preloads the documented 32-byte FIFO and does not use newer I801 BYTE_DONE
streaming. No existing chipset's SMBus path is changed.

## CMOS and clock contracts

| Request | Control byte 0 | Nominal host/SDRAM | Nominal PCI |
|---|---|---|---|
| Hardware-Straps | F0 | Qualified baseline strap | Qualified baseline strap |
| 68.5/34.25MHz | B8 | 68.5 MHz | 34.25 MHz |
| 75/37.5MHz | D8 | 75 MHz | 37.5 MHz |

Software modes use FTS=1. The distinct 75/30 MHz entry, reserved/test encodings
and voltage/ambient-restricted modes are not exposed. The documented baseline
is 66.8/33.4 MHz, not exactly 66.667/33.333 MHz.

Software overclocking requires `MS6117_SC643_OVERCLOCK=y`, CMOS
`clock_policy_version=0x6401`, `overclocking=Enable` and a valid `bus_clock`.
Otherwise the driver explicitly restores the latched hardware selection. A
warm CPU reset does not reset this PLL. Physical jumpers must match the measured
baseline contract; there is no invented readback mechanism to check them.

`MS6117_SC643_STRAP_CLOCK_PS`, `MS6117_SC643_68_CLOCK_PS` and
`MS6117_SC643_75_CLOCK_PS` are conservative minimum period bounds, including
uncertainty and relevant jitter/load/temperature corners. Defaults are provisional,
not measured values. The inherited tAC ceiling remains separately board-qualified.
Only the approximately 66.8 MHz baseline strap is covered; slower strap modes
need a refresh/timing review. Do not program the reserved PACCFG frequency bit.

Full programming sends bytes 0–5. The MS-6117's compile-time output configuration
enables all CPU/AGP, PCI, SDRAM and reference outputs. Trace loads before changing
it; another board can supply its own `struct sc643_config` to the same driver.
By default only byte zero is sent, preserving the existing output enables.
True power-on restores their defaults; a warm reboot from another BIOS may not.

Byte 1 preserves 24/48 MHz and normal operation; byte 6 is reserved and never
sent. The wire frame is `START D2 00 count byte0 ... STOP`, with ACKs. Command
and count are mandatory framing, not a register address. Spread-spectrum
controls remain reserved because this SC643 revision does not implement them.
The separate `cpu_ratio` menu is retained at its original bit offset440,
width4 and enumeration14, with `Auto` as the default. Its original encodings
are unchanged:0=Auto;1..13=2x,2.5x,3x,3.5x,4x,4.5x,5x,5.5x,6x,6.5x,7x,7.5x,8x.
NVRAMCUI can edit it even with SC643 support disabled.

This is a CPU/board policy control for multiplier-configurable CPU setups,
including suitable Klamath systems; it is not a PLL setting. The SC643 driver
does not consume it, adjust timing from it or write a speculative CPU MSR.
An actual multiplier-setting implementation must first establish the fitted
CPU's capabilities, the board/adapter selection mechanism and reset requirements.
Saving a non-Auto request currently does not change the CPU multiplier. Locked
CPUs must not be treated as programmable merely because this menu exists.
All other offsets/checksum coverage are retained, with the new version field
in previously unused space.

## Qualification and recovery

Measure baseline power-on straps, output clocks, supplies, SMBus framing,
live transitions and cold/warm recovery before setting QUALIFIED. The 10 ms
settling wait is a starting point for qualification, not a datasheet guarantee.
No timeout may span a clock change, and firmware timestamps crossing it are
not accurate elapsed-time measurements. There is no runtime/OS clock switching.

Normal recovery disables CMOS overclocking so the next successful firmware
entry restores straps. If execution cannot reach the clock code, remove power,
restore known-good straps and clear CMOS using the documented board procedure.
No automatic reset-loop recovery is claimed. Qualify every intended DIMM, CPU,
adapter and add-in-card population. PCI/AGP/storage are overclocked too; there is
no independent PCI lock. A successful POST does not establish data integrity.

Primary references: IMI SC643 rev 1.6 (20 June 1997), pages 1–9; Intel PIIX4
290562-001, sections 7.3 and 11.5.4.1. Hardware measurements remain outstanding.
