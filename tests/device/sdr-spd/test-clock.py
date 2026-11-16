#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Run actual image builder, board policy and PIIX4 transfer code with modeled I/O."""
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
        'types.h': '#include <stdint.h>\n#include <stddef.h>\n#include <stdbool.h>\nenum cb_err {CB_SUCCESS=0,CB_ERR_ARG=-2};\n',
        'arch/io.h': '#include <stdint.h>\nuint8_t inb(uint16_t); uint32_t inl(uint16_t); void outb(uint8_t,uint16_t);\n',
        'console/console.h': '#define BIOS_NOTICE 0\n#define printk(...) ((void)0)\n#define die(...) longjmp(failure,1)\n',
        'cpu/x86/tsc.h': 'unsigned long tsc_recalibrate(void);\n',
        'device/smbus_host.h': '#include <stddef.h>\n#include <stdint.h>\nuintptr_t smbus_base(void);\n',
        'northbridge/intel/i440lx/raminit.h': 'unsigned int mainboard_sdram_clock_ps(void); void mainboard_sdram_clock_apply(void);\n',
        'option.h': 'unsigned int get_uint_option(const char *,unsigned int);\n',
        'southbridge/intel/i82371eb/i82371eb.h': '#define DEFAULT_PMBASE 0x200\n#define PMTMR 8\n',
    }
    for name, content in headers.items():
        target = d/name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(content)
    (d/'model.c').write_text(r'''
#include <assert.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static jmp_buf failure;
static int qualified, overclock, program_outputs;
#define CONFIG(x) feature_##x
#define feature_MS6117_SC643_QUALIFIED qualified
#define feature_MS6117_SC643_OVERCLOCK overclock
#define feature_MS6117_SC643_PROGRAM_OUTPUTS program_outputs
#define CONFIG_MS6117_SC643_STRAP_CLOCK_PS 14900
#define CONFIG_MS6117_SC643_68_CLOCK_PS 14500
#define CONFIG_MS6117_SC643_75_CLOCK_PS 13300
#define CONFIG_MS6117_SC643_CPU_MASK 0xf
#define CONFIG_MS6117_SC643_PCI_MASK 0x7f
#define CONFIG_MS6117_SC643_SDRAM_MASK 0xfff
#define CONFIG_MS6117_SC643_REF_MASK 0x13
''' + f'#include "{tree}/src/southbridge/intel/i82371eb/block_write.c"\n' +
f'#include "{tree}/src/drivers/clock/imi_sc643/sc643.c"\n' +
f'#include "{tree}/src/mainboard/msi/ms6117/clock.c"\n' + r'''
static int sc643_image(unsigned int mode, unsigned int cpu, unsigned int pci,
 unsigned int sdram, unsigned int ref, uint8_t image[6])
{
 const struct sc643_config config = {.cpu=cpu,.pci=pci,.sdram=sdram,.ref=ref};
 return sc643_build_image(mode,&config,image);
}
static unsigned int board_clock(void)
{
 unsigned int ps=mainboard_sdram_clock_ps();
 mainboard_sdram_clock_apply();
 return ps;
}

static uint8_t regs[8], fifo[32], status;
static unsigned int loaded, started, killed, output_count, reads, recalibrations;
static uint32_t timer;
static int freeze_timer, busy_entry, never_finish, device_error, bad_clear, bad_calibration;
static unsigned int version, request, mode;
static void reset(void)
{
 memset(regs,0,sizeof(regs)); memset(fifo,0,sizeof(fifo));
 loaded=started=killed=output_count=reads=recalibrations=0;
 timer=0xffff00;status=0;
 freeze_timer=busy_entry=never_finish=device_error=bad_clear=bad_calibration=0;
 clock_selected=false;qualified=overclock=program_outputs=1;version=0x6401;request=0;mode=0;
}
uint8_t inb(uint16_t port)
{
 assert(port>=0x100 && port<0x108);
 unsigned int r=port-0x100;
 if(r==STATUS) {
   if(busy_entry)return BUSY;
   if(started && !killed) {
     if(never_finish || ++reads<3)return BUSY;
     return device_error?4:DONE;
   }
   return status;
 }
 assert(!started || killed); /* no FIFO/control reads while active */
 if(r==CONTROL)loaded=0;
 return regs[r];
}
uint32_t inl(uint16_t port)
{
 assert(port==0x208); if(!freeze_timer)timer+=1000; return timer&0xffffff;
}
void outb(uint8_t value,uint16_t port)
{
 assert(port>=0x100 && port<0x108);output_count++;
 unsigned int r=port-0x100;
 if(r==STATUS) { assert(value==CLEAR);status=bad_clear?4:0;return; }
 if(r==CONTROL && value==KILL) {killed++;return;}
 assert(!started || killed); /* all data must precede START */
 regs[r]=value;
 if(r==BLOCK) {assert(loaded<32);fifo[loaded++]=value;}
 if(r==CONTROL && value&START) {
   assert(value==(PROTOCOL_BLOCK|START));assert(loaded==regs[COUNT]);
   assert(!(regs[ADDRESS]&1));started++;
 }
}
uintptr_t smbus_base(void) {return 0x100;}
unsigned long tsc_recalibrate(void) {recalibrations++;return bad_calibration?0:450;}
unsigned int get_uint_option(const char *name,unsigned int fallback)
{
 if(!strcmp(name,"clock_policy_version"))return version;
 if(!strcmp(name,"overclocking"))return request;
 if(!strcmp(name,"bus_clock"))return mode;
 return fallback;
}
static int board_fails(void)
{
 if(setjmp(failure))return 1;
 (void)board_clock();return 0;
}
static int apply_fails(void)
{
 if(setjmp(failure))return 1;
 mainboard_sdram_clock_apply();return 0;
}
int main(void)
{
 const struct sc643_config valid={.cpu=15,.pci=127,.sdram=4095,.ref=19};
 uint8_t image[6],data[32];memset(data,0xa5,sizeof(data));
 assert(sc643_build_image(SC643_HARDWARE_STRAPS,0,image)==CB_ERR_ARG);
 assert(sc643_build_image(SC643_HARDWARE_STRAPS,&valid,0)==CB_ERR_ARG);
 assert(sc643_build_image((enum sc643_mode)-1,&valid,image)==CB_ERR_ARG);
 reset();assert(apply_fails() && !output_count);
 for(unsigned int m=0;m<3;m++) {
   assert(!sc643_image(m,15,127,4095,19,image));
   assert(image[0]==(m==0?0xf0:m==1?0xb8:0xd8));
   assert(image[1]==0xef && image[2]==0x7f && image[3]==0xff && image[4]==15 && image[5]==19);
 }
 for(unsigned int mask=0;mask<8192;mask++)
   assert((sc643_image(0,15,127,mask,19,image)==0)==(mask>0 && mask<4096));
 assert(sc643_image(3,15,127,4095,19,image));
 assert(sc643_image(0,0,127,4095,19,image));
 assert(sc643_image(0,15,63,4095,19,image));
 assert(sc643_image(0,15,127,4095,4,image));
 for(unsigned int n=1;n<=32;n++) {
   reset();assert(!i82371eb_block_write(0x100,0x208,0x69,0,data,n));
   assert(started==1 && !killed && loaded==n && regs[ADDRESS]==0xd2);
   assert(!memcmp(fifo,data,n));
 }
 reset();assert(i82371eb_block_write(0x100,0x208,0x69,0,data,0)==-1);assert(!output_count);
 assert(i82371eb_block_write(0x100,0x208,0x69,0,data,33)==-1);
 assert(i82371eb_block_write(0x100,0x208,0x80,0,data,1)==-1);
 reset();busy_entry=1;assert(i82371eb_block_write(0x100,0x208,0x69,0,data,1)==-2);assert(!output_count);
 reset();bad_clear=1;assert(i82371eb_block_write(0x100,0x208,0x69,0,data,1)==-3);assert(!started);
 reset();device_error=1;assert(i82371eb_block_write(0x100,0x208,0x69,0,data,1)==-3);assert(!killed);
 reset();never_finish=1;assert(i82371eb_block_write(0x100,0x208,0x69,0,data,1)==-2);assert(killed==1);
 reset();never_finish=freeze_timer=1;assert(i82371eb_block_write(0x100,0x208,0x69,0,data,1)==-2);assert(killed==1);
 reset();freeze_timer=1;assert(i82371eb_pm_wait(0x208,35796)==-2);
 reset();assert(!i82371eb_pm_wait(0x208,35796)); /* crosses 24-bit wrap */
 reset();qualified=0;assert(board_fails() && !output_count);
 for(unsigned int m=0;m<3;m++) {
   reset();request=1;mode=m;
   assert(board_clock()==(m==0?14900:m==1?14500:13300));
   assert(loaded==6 && recalibrations==1 && fifo[0]==(m==0?0xf0:m==1?0xb8:0xd8));
 }
 reset();mode=2;request=0;assert(board_clock()==14900 && fifo[0]==0xf0);
 reset();mode=2;request=1;version=0;assert(board_clock()==14900 && fifo[0]==0xf0);
 reset();mode=2;request=1;overclock=0;assert(board_fails() && !output_count);
 reset();mode=3;request=1;assert(board_fails() && !output_count);
 reset();program_outputs=0;assert(board_clock()==14900 && loaded==1);
 reset();device_error=1;assert(board_fails() && !recalibrations && started==1);
 reset();bad_calibration=1;assert(board_fails() && recalibrations==1);
 reset();assert(mainboard_sdram_clock_ps()==14900 && !output_count);
 mode=2;request=1;mainboard_sdram_clock_apply();assert(fifo[0]==0xf0);
 assert(!clock_selected); /* the SPD plan and applied mode use one snapshot */
 puts("PASS: image masks, PIIX4 FIFO/framing/errors/timeouts/wrap, CMOS gates, clock contracts, recalibration ordering");
 return 0;
}
''')
    binary=d/'clock-test'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-O2',
                    '-fsanitize=undefined','-I',str(d),'-I',str(tree/'src'),
                    str(d/'model.c'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
