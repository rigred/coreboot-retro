/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef SUPERIO_ITE_IT8679F_H
#define SUPERIO_ITE_IT8679F_H

#define IT8679F_FDC  0x00 /* Floppy */
#define IT8679F_SP1  0x01 /* Com1 */
#define IT8679F_SP2  0x02 /* Com2 */
#define IT8679F_PP   0x03 /* Parallel port */
#define IT8679F_APC  0x04 /* Advanced Power Controller */
#define IT8679F_KBCK 0x05 /* Keyboard */
#define IT8679F_KBCM 0x06 /* Mouse */
#define IT8679F_GPIO 0x07 /* GPIO */

void it8679f_48mhz_clkin(void);
void it8679f_enable_serial(pnp_devfn_t dev, u16 iobase);

#endif /* SUPERIO_ITE_IT8679F_H */
