# 82443LX BIOS errata handling

Scope: Intel **297655-005**, January 2001, production A-3 PAC (8086:7180,
revision 03). Apply the conservative workarounds on the LX build; do not borrow
a BX queue-depth bit or assume an undocumented later stepping fixes anything.

## Coverage

| Erratum | Required action | Firmware treatment / limit |
| --- | --- | --- |
| 1: EDO/ECC reset sampling | Board must tri-state EDO outputs during reset using the prescribed OE wiring. | Hardware only. Firmware cannot repair entry into manufacturing test mode before it executes. |
| 2: SDRAM reset sampling | Board CKE buffering and pull-ups must satisfy the reset workaround. | Hardware only; no substitute register write. |
| 3: AGP addresses above 4 GiB | Do not rely on SERR to detect these accesses. | Intel gives no workaround. PAC can discard the upper address bits; normal firmware resources remain 32-bit. Drivers must not submit these addresses. |
| 4: STPCLK throttling hang | Disable software and thermal PIIX4 clock throttling. | Clear PCNTRL THT_EN and CC_EN early and during PM enable; omit LX _PTC and legacy FADT duty-cycle fields. Select the documented GPI8 pin function to remove automatic THRM# throttling. |
| 5: IOQ depth one | Use depth four; hard-reset after PAC-initiated BIST. | Early bootblock checks RSTCTRL.BISTE, clears reset-mode controls, and issues a PIIX4 CF9 hard reset. It does not invent an LX IOQD configuration bit. |
| 6: PCI-master VGA write with DOPEN | Prevent PCI masters from writing to AGP VGA while SMRAM is open. | Close early and before resource probing; bracket SMM installation (and optional ECC initialization) with the SMRAM guard below. |
| 7: three-socket x32/x8 loading | Prohibit x32 on the MAB-driven closest socket together with x8 on another socket. | Check SDR component width using the board's SPD map before RAM programming; reject that physical combination. EDO and four-socket designs are exempt. |

The PDF's specification changes and clarifications are also relevant:

- Withdrawal of x4/x32 validation is **not proof of an addressing limit**. The
  errata update does not change the existing 128-MiB/rank sizing code or impose
  a new 384/512-MiB capacity cap. Board-proven 256-MiB modules remain available
  through the existing RAM path. Exact component organization still needs SPD
  and chip-marking evidence.
- ATE identification mode is a manufacturing procedure, not a BIOS sequence.
- The corrected PACCFG definition reserves bits 14:12, reinstates CKE topology
  bit 11, and ties WSC# handshake policy to an actual I/O APIC. Do not display
  bit 14 as a clock-frequency measurement.
- Timing-summary tables do not supply a new generic timing register or justify
  guessing DIMM timing; use the appropriate RAM initialization policy.
- Multi-bit ECC detection is not fail-stop protection: bad data can reach a
  consumer before error handling.
- The buffered EDO note is a population/validation restriction, not new EDO
  initialization support in this port.

## SMRAM guard

`lx_smram_open()` is a **boot-only** operation. The caller must not have started
DMA or launched a payload/option ROM which owns asynchronous devices. It is not
an OS runtime/S3 DMA suspension API. No competing CPU may reprogram devices
inside the guarded interval. SMM installation remains serialized by the existing
MP framework; the guard stays active until all CPU relocations finish.

The PIIX4 address is supplied by the caller. Ramstage uses the existing sconfig
`piix4_pm` alias and derives its ISA sibling; no slot-7 constant or southbridge
search is introduced. The optional ECC path reuses the PM location already
obtained during the existing early PM initialization.

The guard:

1. Clears DOPEN, verifies closure, rejects locked or nested use, and verifies
   that the supplied ISA function is primary-bus PIIX4 8086:7110.
2. Clears PIIX4 TOM[2] (ISA/DMA forwarding of A0000-BFFFF). Function 0's PCI
   bus-master enable is hardwired on and cannot protect this path.
3. Checks all 256 primary-bus function addresses and temporarily clears BME
   wherever it was set, excluding PAC's two architectural functions and the
   separately gated PIIX4 ISA function. Bridges are included to stop upstream
   initiation from secondary buses. A master whose BME cannot be cleared stops
   boot before DOPEN is set.
4. Performs readbacks and a 10-us settling delay, then exposes ASEG with 72h=4Ah.
5. On exit, clears DOPEN and verifies it **before** restoring TOM or any BME.
   Only the saved BME permissions are restored; command/status dword writes are
   not used, so adjacent W1C status is not accidentally acknowledged.

The primary-bus sweep is required for unknown add-in masters, not for locating
an onboard device. It runs only during an actual SMRAM-open operation. It does
not scan child buses, allocate resources, or run during normal runtime SMIs.
The delay and guard model are not physical proof that an already-running DMA
engine or arbitrary bridge FIFO has drained; those uses are excluded by the
boot-only contract. Restore failures stop boot with SMRAM closed; an inability
to close halts without invoking a possibly graphical error console.

SMM completion closes the window, restores the guards, and then locks 72h=1Ah
with readback. DCLS is not set: it changes SMM data routing and is not the normal
replacement for clearing DOPEN. SMRAM closure is independent of I440LX_SMM.

## Throttling and thermal policy

Intel's erratum specifically calls for an inactive THRM# input. PIIX4's documented
GENCFG[23] mux offers a software way to select GPI8 instead of that input. This
is a derived implementation using 290562-001 section 4.1.16, not a new workaround
quoted from the LX PDF. Clearing GPEN.THRM_EN alone would be wrong: it masks
SCI/SMI reporting and does not stop automatic hardware throttling (section
11.2.1). A zero thermal-duty encoding is reserved and is not used as a disable.

This removes the THRM# input's throttling protection. Cooling and any independent
thermal shutdown must be qualified on the board; do not claim this patch adds a
replacement thermal sensor/shutdown service. An OS driver with direct access to
PIIX4 can still reprogram its controls; ACPI omission is not a hardware lock.
Other northbridges using the shared PIIX4 driver retain their old ACPI/PM behavior.

## BIST and DIMM contracts

The BIST recovery check is at the first northbridge C hook, after the existing
Slot-1 CAR setup but before console, DRAM initialization, PCI enumeration, or
AGP activation. The CF9 path avoids generic reset code's WBINVD of a live CAR
stack. Clear BISTE before reset to avoid deliberately re-entering BIST.
Do not add code that clears BISTE and then continues without the hard reset.
If an external agent enters depth-one mode while hiding the BIST indication,
this check cannot certify the queue state; such boot chains need separate review.

`I440LX_MAB_SPD_ADDRESS` is board wiring data. Its defaults follow the current
port's normal (52h) and MSI reversed (50h) three-socket maps; confirm the closest
MAB-driven socket against the board wiring. A configured address outside the
socket map fails. Installed DIMMs with unreadable SPD cannot be distinguished
reliably from empty sockets without another presence signal. A malformed width
on an SPD which responds as SDR is rejected. Disabling a rank cannot cure an
unsupported physical loading combination.

## Other defensive corrections

Only observed W1C flags are acknowledged in ERRSTS0/1. In particular, writing
MEF=1 when it was zero can spuriously assert SERR if enabled (290564-002 3.3.25).
This update does not enable ECC SERR routing or install an NMI error handler.

## References

- Intel 297655-005, January 2001: errata pp. 13-15; specification changes pp.
  9-11; clarifications pp. 17-18; documentation changes pp. 19-20.
  https://downloads.bl4ckb0x.de/download.intel.com/design/chipsets/specupdt/29765505.pdf
- Intel 290564-002: SMRAM 3.3.23, reset/BIST 3.3.27 and 4.8.7, ERRSTS 3.3.25.
  https://downloads.bl4ckb0x.de/download.intel.com/design/chipsets/datashts/29056402.pdf
- Intel 290562-001: PIIX4 PCICMD 4.1.3, TOM 4.1.12, GENCFG 4.1.16,
  PCNTRL 7.2.7, clock-control behavior 11.2.1.
  https://theretroweb.com/chip/documentation/29056201-64f6fb6fe8db4631654578.pdf

Build and model tests cannot exercise PAC timing, electrical reset/CKE behavior,
or real PCI/AGP DMA. Test cold/warm resets, empty AGP enumeration, PCI and AGP VGA,
both P2200 CPUs, and the selected DIMM populations on physical boards. Do not
mark the previously reported AGP hang resolved without reproducing and retesting it.
