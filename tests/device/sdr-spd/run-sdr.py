#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Compile actual common SDR code and LX policy, without SMBus/hardware access."""
import argparse
import pathlib
import subprocess
import tempfile

p = argparse.ArgumentParser()
p.add_argument('--tree', type=pathlib.Path, required=True)
a = p.parse_args()
tree = a.tree.resolve()
with tempfile.TemporaryDirectory(dir=pathlib.Path(__file__).resolve().parent) as tmp:
    d = pathlib.Path(tmp)
    headers = {
        'types.h': '#include <host.h>\n',
        'console/console.h': '#include <host.h>\n#define BIOS_DEBUG 0\n#define printk(...) ((void)0)\n_Noreturn void host_die(void);\n#define die_with_post_code(...) host_die()\n',
        'commonlib/console/post_codes.h': '#define POSTCODE_RAM_FAILURE 0\n',
        'device/pci_ops.h': '#include <host.h>\n#define PCI_DEV(b,d,f) 0\nuint8_t pci_read_config8(unsigned,unsigned);\nuint16_t pci_read_config16(unsigned,unsigned);\nvoid pci_write_config8(unsigned,unsigned,uint8_t);\nvoid pci_write_config16(unsigned,unsigned,uint16_t);\n',
        'device/smbus_host.h': 'int smbus_read_byte(unsigned int,unsigned int);\n',
        'option.h': 'unsigned int get_uint_option(const char *,unsigned int);\n',
        'lib.h': '#include <host.h>\nstatic inline int log2(uint32_t x) {return x ? 31-__builtin_clz(x) : -1;}\n',
        'device/pci_type.h': '#include <host.h>\ntypedef uint32_t pci_devfn_t;\n',
        'host.h': '#pragma once\n#include <stdint.h>\n#include <stddef.h>\n#include <stdbool.h>\n#define u8 uint8_t\n#define u16 uint16_t\n#define u32 uint32_t\n#define __weak __attribute__((weak))\n#define CONFIG(x) config_##x\n#define config_SDRAM_4DIMM 0\n#define config_SDRAM_3DIMM 1\n#define config_SDRAM_3DIMM_REVERSE_OFFSET 0\n#define config_I440LX_SDRAM_FAST_MA 0\n#define CONFIG_I440LX_SDRAM_CLOCK_PS 15000\n#define CONFIG_I440LX_SDRAM_MAX_TAC_PS 9000\n#define CONFIG_I440LX_SDRAM_MIN_CLOCK_PS 13300\n#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))\n',
    }
    for name, content in headers.items():
        target = d/name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(content)
    sources = ['src/device/dram/sdr.c', 'src/device/dram/sdr_common.c',
               'src/device/dram/ddr_common.c', 'src/northbridge/intel/i440lx/sdram_policy.c']
    binary = d/'test-sdr'
    args = ['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-Wno-builtin-declaration-mismatch',
            '-fsanitize=undefined', '-include', str(d/'host.h'), '-I', str(d),
            '-idirafter', str(tree/'src/include'), '-I', str(tree/'src/northbridge/intel/i440lx'),
            str(pathlib.Path(__file__).with_name('test-sdr.c'))]
    subprocess.run(args + [str(tree/s) for s in sources] + ['-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)

    base_args = args[:args.index(str(pathlib.Path(__file__).with_name('test-sdr.c')))]
    subprocess.run(base_args + [str(pathlib.Path(__file__).with_name('test-integration.c'))] +
                   [str(tree/s) for s in sources] +
                   [str(tree/'src/northbridge/intel/i440lx/spd_raminit.c'), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)

    # Execute the actual DDR2 checksum wrapper, with unchanged short-input behavior.
    text = (tree/'src/device/dram/ddr2.c').read_text()
    start = text.index('u8 spd_ddr2_calc_checksum(')
    end = text.index('\n/**', start)
    (d/'ddr2-checksum.c').write_text('#include <assert.h>\n#include <device/dram/common.h>\n' + text[start:end] +
        '\nint main(void) {uint8_t a[128];for(int i=0;i<128;i++)a[i]=i;'
        'assert(spd_ddr2_calc_checksum(a,62)==0);'
        'assert(spd_ddr2_calc_checksum(a,63)==(uint8_t)1953);'
        'assert(spd_ddr2_calc_checksum(a,128)==(uint8_t)1953);return 0;}\n')
    subprocess.run(base_args +
                   [str(d/'ddr2-checksum.c'), str(tree/'src/device/dram/ddr_common.c'),
                    '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
    print('PASS: unchanged DDR2 checksum wrapper behavior')
