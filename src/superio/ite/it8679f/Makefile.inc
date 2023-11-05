# SPDX-License-Identifier: GPL-2.0-or-later

bootblock-$(CONFIG_SUPERIO_ITE_IT8679F) += early_serial.c
romstage-$(CONFIG_SUPERIO_ITE_IT8679F) += early_serial.c
ramstage-$(CONFIG_SUPERIO_ITE_IT8679F) += superio.c
