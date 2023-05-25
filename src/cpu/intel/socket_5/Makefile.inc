## SPDX-License-Identifier: GPL-2.0-or-later

ramstage-y += socket_5.c
subdirs-y += ../model_5xx

bootblock-y += ../car/p3/cache_as_ram.S
bootblock-y += ../car/bootblock.c
postcar-y += ../car/p4-netburst/exit_car.S
romstage-y += ../car/romstage.c
