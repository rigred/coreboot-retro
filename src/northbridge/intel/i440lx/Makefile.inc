## SPDX-License-Identifier: GPL-2.0-or-later

ifeq ($(CONFIG_NORTHBRIDGE_INTEL_I440LX),y)

bootblock-y += bootblock.c
ramstage-y += northbridge.c
ramstage-y += smram.c
romstage-y += smram.c
romstage-y += dimm_errata.c
ramstage-$(CONFIG_I440LX_SMM) += smm.c

romstage-y += raminit.c
romstage-$(CONFIG_I440LX_SPD_RAMINIT) += sdram_policy.c
romstage-$(CONFIG_I440LX_SPD_RAMINIT) += spd_raminit.c
romstage-$(CONFIG_I440LX_SPD_RAMINIT) += sdram_finish.c
romstage-y += romstage.c
romstage-$(CONFIG_DEBUG_RAM_SETUP) += debug.c
romstage-y += memmap.c

endif
