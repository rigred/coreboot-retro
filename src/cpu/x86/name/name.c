/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/cpu.h>
#include <cpu/x86/name.h>
#include <stdint.h>
#include <string.h>

void fill_processor_name(char *processor_name)
{
	struct cpuid_result regs;
	char *processor_name_start;
	uint32_t name_as_ints[13];
	int i;

	// Check if the CPUID leaf is supported.
    regs = cpuid(0x80000000);
    if (regs.eax < 0x80000004) {
        // The required CPUID leaves are not supported.
        strcpy(processor_name, "Unknown Processor");
        return;
    }


	for (i = 0; i < 3; i++) {
		regs = cpuid(0x80000002 + i);
		name_as_ints[i * 4 + 0] = regs.eax;
		name_as_ints[i * 4 + 1] = regs.ebx;
		name_as_ints[i * 4 + 2] = regs.ecx;
		name_as_ints[i * 4 + 3] = regs.edx;
	}

	name_as_ints[12] = 0;

	// Skip leading spaces and copy the name to the provided buffer.
    for (i = 0; i < 13; i++) {
        if (name_as_ints[i] == ' ')
            continue;
        *processor_name_start = (char)name_as_ints[i];
        processor_name_start++;
    }

	// Null-terminate the processor name.
    *processor_name_start = '\0';
}
