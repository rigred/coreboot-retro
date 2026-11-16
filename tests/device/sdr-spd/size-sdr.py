#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""32-bit freestanding GC check. Numbers include a tiny entry and memset stub."""
import argparse
import json
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
        'stdint.h': '#pragma once\ntypedef unsigned char uint8_t;typedef unsigned short uint16_t;typedef unsigned int uint32_t;\n',
        'stddef.h': '#pragma once\ntypedef unsigned int size_t;\n',
        'stdbool.h': '#pragma once\n#define bool _Bool\n#define true 1\n#define false 0\n',
        'types.h': '#include <stdint.h>\n#define u8 uint8_t\n#define u16 uint16_t\n',
        'console/console.h': '#include <types.h>\n',
        'string.h': '#include <stddef.h>\nvoid *memset(void *,int,size_t);\n',
        'lib.h': '#include <stdint.h>\n#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))\nstatic inline int log2(uint32_t x) {return x?31-__builtin_clz(x):-1;}\n',
    }
    for name, content in headers.items():
        f=d/name;f.parent.mkdir(parents=True, exist_ok=True);f.write_text(content)
    support = '#include <stddef.h>\n#include <device/dram/sdr.h>\nuint8_t spd[64];\n' + \
        'void *memset(void *p,int v,size_t n) {volatile uint8_t *b=p;while(n--)*b++=v;return p;}\n'
    profiles = {
        'geometry': 'struct sdr_dimm_geometry g;void _start(void){volatile int r=spd_decode_sdr_geometry(&g,spd);(void)r;for(;;);}',
        'timings': 'struct sdr_dimm_timings t;void _start(void){volatile int r=spd_decode_sdr_timings(&t,spd);(void)r;for(;;);}',
        'full': 'struct dimm_attr_sdr_st g;void _start(void){volatile int r=spd_decode_sdr(&g,spd);r+=spd_sdr_supports_cas(&g.timings,2,15000,9000);(void)r;for(;;);}',
    }
    results = {}
    for name, entry in profiles.items():
        (d/'entry.c').write_text(support+entry+'\n')
        binary=d/name
        subprocess.run(['cc','-m32','-Os','-std=c11','-ffreestanding','-fno-builtin',
                        '-fno-pie','-fno-stack-protector','-fno-asynchronous-unwind-tables',
                        '-fno-unwind-tables','-ffunction-sections','-fdata-sections',
                        '-nostdlib','-no-pie','-Wl,--gc-sections,-e,_start',
                        '-I',str(d),'-idirafter',str(tree/'src/include'),
                        str(d/'entry.c'),*[str(tree/'src/device/dram'/f)
                        for f in ['sdr.c','sdr_common.c']],'-o',str(binary)],check=True)
        symbols=subprocess.check_output(['nm',str(binary)],text=True)
        if name=='geometry':
            assert 'spd_decode_sdr_timings' not in symbols and 'sdr_spd_checksum' not in symbols
            assert 'spd_sdr_supports_cas' not in symbols
        if name=='timings':
            assert 'spd_decode_sdr_geometry' not in symbols and 'sdr_spd_checksum' not in symbols
        sections=subprocess.check_output(['size','-A',str(binary)],text=True)
        sizes={line.split()[0]:int(line.split()[1]) for line in sections.splitlines()
               if line.startswith(('.text ','.rodata ','.data ','.bss '))}
        results[name]={'code_and_rodata_bytes':sum(sizes.get(k,0) for k in ['.text','.rodata','.data']),
                       'bss_bytes':sizes.get('.bss',0),'gc_verified':True}
    print(json.dumps(results,indent=2))
