## SPDX-License-Identifier: GPL-2.0-only

bootblock-y += bootblock.c

ramstage-$(CONFIG_GENERATE_PIRQ_TABLE) += irq_tables.c

ramstage-y += irq.c
ramstage-$(CONFIG_GENERATE_SMBIOS_TABLES) += slots.c

romstage-$(CONFIG_MS6117_SC643) += clock.c
