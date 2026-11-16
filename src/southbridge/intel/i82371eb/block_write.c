/* SPDX-License-Identifier: GPL-2.0-only */
#include <arch/io.h>
#include "block_write.h"

/* Intel 290562-001 sections 7.3 and 11.5.4.1. PIIX4 has no BYTE_DONE bit.
 * Reset its FIFO pointer with a control read, preload, then start.
 */
#define STATUS 0
#define CONTROL 2
#define COMMAND 3
#define ADDRESS 4
#define COUNT 5
#define BLOCK 7
#define BUSY 0x01
#define DONE 0x02
#define ERRORS 0x1c
#define CLEAR 0x1e
#define PROTOCOL_BLOCK 0x14
#define START 0x40
#define KILL 0x02
#define PM_MASK 0xffffff
#define TIMEOUT_TICKS 357955 /* 100 ms, independent of CPU/PCI clock */
#define POLL_LIMIT 1000000 /* Also bound a stuck/absent PM timer. */

int i82371eb_pm_wait(uint16_t timer, unsigned int ticks)
{
	uint32_t start = inl(timer);

	for (unsigned int n = 0; n < POLL_LIMIT; n++)
		if (((inl(timer) - start) & PM_MASK) >= ticks)
			return 0;
	return -2;
}

static int wait_idle(uint16_t base, uint16_t timer, int completion)
{
	uint32_t start = inl(timer);

	for (unsigned int n = 0; n < POLL_LIMIT; n++) {
		uint8_t status = inb(base + STATUS);

		if (!(status & BUSY) && (!completion || (status & CLEAR)))
			return completion && ((status & ERRORS) || !(status & DONE)) ? -3 : 0;
		if (((inl(timer) - start) & PM_MASK) >= TIMEOUT_TICKS)
			break;
	}
	return -2;
}

int i82371eb_block_write(uint16_t base, uint16_t timer, uint8_t address,
			uint8_t command, const uint8_t *data, size_t count)
{
	if (!data || !count || count > 32 || address > 0x7f)
		return -1;
	/* Do not abort an unrelated transaction that was busy on entry. */
	if (wait_idle(base, timer, 0))
		return -2;
	outb(CLEAR, base + STATUS);
	if (inb(base + STATUS) & (BUSY | CLEAR))
		return -3;
	outb(PROTOCOL_BLOCK, base + CONTROL); /* interrupts disabled */
	(void)inb(base + CONTROL); /* reset the 32-byte FIFO pointer */
	outb(address << 1, base + ADDRESS);
	outb(command, base + COMMAND);
	outb(count, base + COUNT);
	for (size_t i = 0; i < count; i++)
		outb(data[i], base + BLOCK);
	outb(PROTOCOL_BLOCK | START, base + CONTROL);
	int ret = wait_idle(base, timer, 1);

	if (ret == -2) {
		outb(KILL, base + CONTROL);
		if (wait_idle(base, timer, 0))
			return ret;
		outb(0, base + CONTROL);
	}
	/* Only touch completion registers after BUSY deasserts. */
	outb(CLEAR, base + STATUS);
	return ret;
}
