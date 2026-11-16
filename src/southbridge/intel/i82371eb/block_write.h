/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef I82371EB_BLOCK_WRITE_H
#define I82371EB_BLOCK_WRITE_H
#include <stddef.h>
#include <stdint.h>

/* SMBus/PM decode must already be enabled. timer is a 24-bit PM timer port.
 * Return 0, -1 (invalid), -2 (busy/timeout), -3 (transaction failure).
 * No transaction retry: a write-only target may have accepted a prefix.
 */
int i82371eb_block_write(uint16_t base, uint16_t timer, uint8_t address,
			uint8_t command, const uint8_t *data, size_t count);
int i82371eb_pm_wait(uint16_t timer, unsigned int ticks);
#endif
