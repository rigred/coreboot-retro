/* SPDX-License-Identifier: GPL-2.0-only */
/* Board policy shared with firmware IRQ setup and $PIR. */
Mutex (IRQM, 0)
OperationRegion (ELCR, SystemIO, 0x4d0, 2)
Field (ELCR, ByteAcc, NoLock, Preserve) { ELVB, 16 }
Name (IRQP, ResourceTemplate () {
	IRQ (Level, ActiveLow, Shared) {
#if CONFIG_MS6117_PCI_IRQ_MASK & 0x0020
		5,
#endif
#if CONFIG_MS6117_PCI_IRQ_MASK & 0x0400
		10,
#endif
#if CONFIG_MS6117_PCI_IRQ_MASK & 0x0800
		11,
#endif
	}
})

#define PCI_INTX_DEV(intx, pinx, uid) \
Device (intx) { \
	Name (_HID, EisaId ("PNP0C0F")) \
	Name (_UID, uid) \
	Name (IRQB, ResourceTemplate () { IRQ (Level, ActiveLow, Shared) {} }) \
	Method (_STA, 0, Serialized) { \
		If (pinx & 0x80) { Return (0x09) } \
		Return (0x0b) \
	} \
	Method (_DIS, 0, Serialized) { pinx = 0x80 } \
	Method (_PRS, 0, Serialized) { Return (IRQP) } \
	Method (_CRS, 0, Serialized) { \
		CreateWordField (IRQB, 1, IRQN) \
		IRQN = Zero \
		Local0 = pinx \
		If (!(Local0 & 0x80)) { \
			Local1 = One << (Local0 & 0x0f) \
			If (Local1 & MS6117_IRQ_MASK) { IRQN = Local1 } \
		} \
		Return (IRQB) \
	} \
	Method (_SRS, 1, Serialized) { \
		If (ObjectType (Arg0) != 3) { Return () } \
		If (SizeOf (Arg0) < 5) { Return () } \
		Local0 = DerefOf (Arg0 [0]) \
		If ((Local0 != 0x22) && (Local0 != 0x23)) { Return () } \
		If ((Local0 == 0x23) && (SizeOf (Arg0) < 6)) { Return () } \
		CreateWordField (Arg0, 1, MASK) \
		Local0 = MASK \
		If (!Local0 || (Local0 & ~MS6117_IRQ_MASK)) { Return () } \
		If (Local0 & (Local0 - One)) { Return () } \
		FindSetRightBit (Local0, Local1) \
		Acquire (IRQM, 0xffff) \
		ELVB |= Local0 \
		pinx = Local1 - One \
		Release (IRQM) \
	} \
}
