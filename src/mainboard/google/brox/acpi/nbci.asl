/* SPDX-License-Identifier: GPL-2.0-or-later */

#define NBCI_FUNC_SUPPORT	0
#define NBCI_FUNC_PLATCAPS	1
#define NBCI_FUNC_GETOBJBYTYPE	16
#define NBCI_FUNC_GETCALLBACKS	19

/* 'DR' in ASCII, for DRiver Object */
#define NBCI_OBJTYPE_DR		0x4452

#define GPS_FUNC_GETCALLBACKS	0x13

Method (NBCI, 2, Serialized)
{
	Switch (ToInteger (Arg0))
	{
		Case (NBCI_FUNC_SUPPORT)
		{
			Return (ITOB(
				(1 << NBCI_FUNC_SUPPORT) |
				(1 << NBCI_FUNC_PLATCAPS) |
				(1 << NBCI_FUNC_GETOBJBYTYPE)))
		}
		Case (NBCI_FUNC_PLATCAPS)
		{
			Return (ITOB(
				(0 << 10) |	/* No 3D Hotkeys */
				(0 << 9)  |	/* Do not enumerate a dock */
				(0 << 7)  |	/* Does not have DISPLAYSTATUS */
				(0 << 5)  |	/* No LID support */
				(0 << 4)))	/* No Aux power state request */
		}
		Case (NBCI_FUNC_GETCALLBACKS)
		{
			/* Re-use the GPS subfunction's GETCALLBACKS Method */
			Return (GPS (GPS_FUNC_GETCALLBACKS, Arg1))
		}
		Case (NBCI_FUNC_GETOBJBYTYPE)
		{
			CreateWordField (Arg1, 2, BFF0)
			If (BFF0 == NBCI_OBJTYPE_DR)
			{
				Return (Buffer(0xa1)
				{
					/* DR ("Driver Object") is a data object which
					 * might vary depending on the eDP panel used. */
					0x57, 0x74, 0xdc, 0x86, 0x75, 0x84, 0xec, 0xe7,
					0x52, 0x44, 0xa1, 0x00, 0x00, 0x00, 0x00, 0x01,
					0x00, 0x00, 0x00, 0x00, 0xde, 0x10, 0x00, 0x00,
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x34, 0x00,
					0x00, 0x00, 0x01, 0x00, 0x47, 0x00, 0x00, 0x00,
					0x02, 0x00, 0x45, 0x00, 0x00, 0x00, 0x03, 0x00,
					0x51, 0x00, 0x00, 0x00, 0x04, 0x00, 0x4f, 0x00,
					0x00, 0x00, 0x05, 0x00, 0x4d, 0x00, 0x00, 0x00,
					0x06, 0x00, 0x4b, 0x00, 0x00, 0x00, 0x07, 0x00,
					0x49, 0x00, 0x00, 0x00, 0x08, 0x00, 0x47, 0x00,
					0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xd9, 0x1c,
					0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
					0x41, 0x5d, 0xc9, 0x00, 0x01, 0x24, 0x2e, 0x00,
					0x02, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x01,
					0x00, 0x00, 0x00, 0xd9, 0x1c, 0x04, 0x00, 0x00,
					0x00, 0x01, 0x00, 0x00, 0x00, 0x60, 0x68, 0x9e,
					0x35, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00
				})
			} Else {
				Return (NV_ERROR_UNSPECIFIED)
			}

		}
	}

	Return (NV_ERROR_UNSUPPORTED)
}
