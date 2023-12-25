## SPDX-License-Identifier: GPL-2.0-only

bootblock-y += bootblock.c

ramstage-$(CONFIG_GENERATE_PIRQ_TABLE) += irq_tables.c
