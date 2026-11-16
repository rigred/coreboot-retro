# SDR SDRAM SPD decoding

The SDR utilities follow the `device/dram/ddr2.h` / `ddr2.c` convention: a caller
reads raw SPD bytes and passes them to a pure decoder. The compact result describes
the module; it does not program a controller or certify compatibility.

## Interfaces and size

```c
#include <device/dram/sdr.h>

uint8_t spd[SPD_SIZE_MAX_SDR];
struct dimm_attr_sdr_st dimm;

/* Read all 64 bytes with the platform's existing SMBus/I2C byte helper.
 * Check every read before using its result. Do not truncate a negative error.
 */
if (spd_decode_sdr(&dimm, spd) != SPD_STATUS_OK)
	/* Report an invalid/unusable DIMM; never consume partial output. */;
```

The full decoder validates the bytes 0–62 additive checksum against byte 63.
It returns existing `enum spd_status` values. `SPD_STATUS_CRC_ERROR` is the
existing shared status name; SDR uses an additive checksum, not a CRC.

`struct dimm_attr_sdr_st` is 46 bytes: a 24-byte geometry record and a 22-byte
timing record. For smaller consumers:

* `spd_decode_sdr_geometry()` needs bytes 0–31 and 62. It validates type,
  revision and geometry but does not check the checksum or decode timings.
* `spd_decode_sdr_timings()` needs the timing fields in bytes 0–31 and revision
  byte 62. It does not check geometry or the checksum.
* `spd_sdr_supports_cas()` checks a decoded latency at a specified clock period
  and board-qualified maximum tAC. It does not choose controller registers.

These are separate linkable functions. Geometry-only callers need not link
timings, the full decoder, the checksum or CAS selection. No runtime feature
descriptor or new SDR feature Kconfig switches are needed. The existing
`USE_SDR_SDRAM` selects the source files; normal function-section garbage
collection removes unreferenced entry points. Small ECC/registered attribute
decoding remains common rather than introducing per-field global switches.

No allocation, floating point, transport callbacks, SPD-address discovery or
controller-sized slot arrays are present in common code. A board/controller
keeps its existing compile-time slot count, EEPROM addresses and byte transport.
An SMBus failure must be handled by that transport caller. A first-read NACK
cannot universally distinguish an empty slot from an unreadable EEPROM.

## Fields and encodings

| SPD bytes | Decoded information |
|---|---|
| 0, 2, 62 | Available data, SDR type, supported revision |
| 3–5 | Row/column bits and module ranks |
| 6–8 | Module electrical width and interface voltage |
| 9–10 | Highest-CAS tCK and tAC |
| 11–14 | Parity/ECC, refresh/self-refresh, data/check component widths |
| 15–20 | tCCD, burst lengths, device banks, CAS/CS/WE latency masks |
| 21 | Buffered, registered, PLL and differential-clock attributes |
| 23–26 | Lower-CAS tCK/tAC pairs |
| 27–31 | tRP, tRRD, tRCD, tRAS and rank-density cross-check |
| 63 | Additive checksum, full decoder only |

Two geometry entries represent the two organizations encoded by SPD, not a
two-rank hardware limit. The supported common subset includes one to eight
ranks, 4–512 MiB per rank and 8–256 data bits, plus conventional 36/32, 72/64
and 144/128 parity/ECC extensions. More than two ranks must be symmetric.
Unsupported encodings return an error; sizes are never clamped or guessed.

The decoder computes capacity from row bits, column bits, component banks and
**data** bus width. Check bits do not contribute to usable capacity. Device
density is separately recorded in Mbit. Byte 31 is an unordered set of rank
densities; it does not decide physical rank order. Row/column nibbles retain
that order. The doubled second-rank data/check component widths are supported.
Redundant-address organizations are rejected because this subset cannot
represent their mixed address translation.

Timings use exact 50 ps units for tCK/tAC and whole ns for tRP/tRCD/tRAS/tRRD.
The highest-CAS bytes 9/10 use decimal nibbles. Only bytes 23/24 have the
revision-0/1 16–18 ns rollover. Bytes 25/26 use quarter-ns fractions. DDR2's
special fractional nibble encoding must not be used for SDR.

CAS timing indices are highest, highest-minus-one and highest-minus-two, not
successive set bits. Missing lower-CAS timing pairs remain unavailable, even
if that CAS bit is advertised. No timing is inferred from a PC66/PC100 label.

## Existing helpers and code boundaries

The code uses the existing `spd.h` field numbers and memory-type enum,
`device/dram/common.h` status enum, `lib.h` integer `log2()` and build/link
conventions without modifying those interfaces. The fixed-length additive
checksum is private to `sdr.c`: only the complete SDR decoder uses it. There
is no new shared checksum API, no dependency on `ddr_common.c`, and no DDR2
source change. Geometry-only and timing-only users do not retain the complete
decoder or its checksum code. Test fixtures compute their checksum independently.
No DDR2 timing decoder is reused because its encodings differ.

The linked upstream `spd.c` contains manufacturer and SMBIOS module mappings,
not the raw byte transport/parser. Those interfaces remain unchanged. This
series does not add identifying strings, manufacturer lookup tables or SMBIOS
generation to romstage, and does not reinterpret an SDR attribute byte as a
DDR2/DDR3 module-type enum.

## Controller integration

The LX adapter consumes this common result directly. Its policy checks
module width, 3.3 V interface, burst/command latencies, supported organizations,
refresh requirements and actual requested clock. Registered/buffered DIMMs
remain rejected by LX even though common decoding succeeds. Existing ECC
wiring qualification, check-bit initialization, sampled address tests and CMOS
manual timing constraints remain controller-specific.

The common decoder also accepts different component widths between two ranks.
LX retains the inherited restriction against those mixed-width organizations
until their electrical/controller behavior is qualified.

The inherited LX whitelist still limits ranks to 64 MiB. A 256 MiB two-rank
module decodes correctly as two 128 MiB ranks, but the experimental LX policy
still rejects that organization. The known-working legacy RAM path remains
the default. This refactor does not invent undocumented LX geometry settings
or silently change DRB/RPS/DRT encodings to enable those modules.

For 440BX, VIA 694T or MVP3, select `USE_SDR_SDRAM`, read a complete snapshot
with that platform's existing transport, call the decoder and implement an
independent controller policy. Registered support requires controller command
latency, loading, voltage, buffer-strength and ECC initialization work; accepting
an SPD flag alone cannot supply that support. Do not import LX's rank, timing,
clock or refresh limits into common code.

## Later inventory and Windows 98 work

The result preserves rank capacity, widths, ECC, registration, refresh and
timing capability without formatting them as controller registers. A later
inventory adapter can combine these with populated slot locations, **enabled**
rank mappings and configured speed, then publish the appropriate records to
ramstage. Do not report the full SPD capacity as usable after policy disables
ranks. Preserve physical slot identity when doing so.

Optional identity reads (JEDEC manufacturer bytes 64–71, part number, serial,
date) are intentionally omitted. A later helper must handle legacy JEDEC
continuation/parity encodings before using `spd_manufacturer_name()`. Reuse
`get_spd_info()` and the existing SMBIOS writer after explicitly extending the
SDR mapping; supply the board-known DIMM/SODIMM form factor. Add no guesses
from module capacity or manufacturer text.

SMBIOS/DMI inventory does not itself make Windows 98 enumerate chipset devices.
That later work also needs correct PCI IDs/resources, PIRQ routing, legacy
device configuration and appropriate ACPI/APM/PnP services in coreboot/SeaBIOS.
This series prepares memory metadata and does not claim to implement that work.

## Validation and references

From the repository root, run the standalone host tests:

```sh
python3 tests/device/sdr-spd/run-sdr.py --tree .
python3 tests/device/sdr-spd/test-clock.py --tree .
python3 tests/device/sdr-spd/size-sdr.py --tree .
```

These compile actual implementation files with modeled I/O. They are separate
from the cmocka runners. The size check needs a 32-bit C compiler/linker but
does not execute its freestanding target binaries. Size figures include its
entry point and memset stub and do not predict a complete firmware's allocation.

The normative SDR field source is Intel's PC SDRAM SPD Specification revision
1.2B, November 1999, especially pages 12–24. Upstream style/reference files:

* https://github.com/coreboot/coreboot/blob/main/src/device/dram/ddr2.c
* https://github.com/coreboot/coreboot/blob/main/src/device/dram/ddr5.c
* https://github.com/coreboot/coreboot/blob/main/src/device/dram/spd.c
* https://github.com/coreboot/coreboot/blob/main/src/device/dram/ddr_common.c
