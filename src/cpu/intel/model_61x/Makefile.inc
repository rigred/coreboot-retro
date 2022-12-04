## SPDX-License-Identifier: GPL-2.0-or-later

ramstage-y += model_61x_init.c

cpu_microcode_bins += $(wildcard 3rdparty/intel-microcode/intel-ucode/06-01-*)
