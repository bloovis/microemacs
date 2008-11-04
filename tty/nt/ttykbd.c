/*
    Copyright (C) 2008 Mark Alexander

    This file is part of MicroEMACS, a small text editor.

    MicroEMACS is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* $Header: /home/bloovis/cvsroot/pe/nt/ttykbd.c,v 1.1 2003-11-06 02:51:52 bloovis Exp $
 *
 * Name:	MicroEMACS
 * 		IBM PC Keyboard
 * Version:	1
 * By:		Mark Alexander
 *		drivax!alexande
 *
 * $Log: ttykbd.c,v $
 * Revision 1.1  2003-11-06 02:51:52  bloovis
 * Initial revision
 *
 * Revision 1.1  2001/04/19 20:26:08  malexander
 * New files for NT version of MicroEMACS.
 *
 * Revision 1.2  91/04/19  23:04:40  alexande
 * Bind Insert key to new set-overstrike function.
 * 
 * Revision 1.1  91/04/19  23:02:44  alexande
 * Initial revision
 * 
 */
#include	"def.h"

/*
 * Names for the keys with basic keycode
 * between KFIRST and KLAST (inclusive). This is used by
 * the key name routine in "kbd.c".
 */
char	*keystrings[32] = {
	NULL,		"Up",		"Down",		"Left",
	"Right",	"PgUp",		"PgDn",		"Home",
	"End",		"Insert",	"Delete",	"F1",
	"F2",		"F3",		"F4",		"F5",
	"F6",		"F7",		"F8",		"F9",
	"F10",		"S-F1",		"S-F2",		"S-F3",
	"S-F4",		"S-F5",		"S-F6",		"S-F7",
	"S-F8",		"S-F9",		"S-F10",	NULL
};

/*
 * The special keys of the PC keyboard as returned by getakey()
 * are values greater than 0x100.  The following table gives
 * the values of the most commonly used special function keys.
 * These values correspond to the keys named in
 * the keystrings table above, unmodified by ALT, or CTRL keys.
 * These keys map into the MicroEMACS internal key values K01 to K1F.
 */
int	specmap[32] = {
	0,		0x148,		0x150,		0x14b,
	0x14d,		0x149,		0x151,		0x147,
	0x14f,		0x152,		0x153,		0x13b,
	0x13c,		0x13d,		0x13e,		0x13f,
	0x140,		0x141,		0x142,		0x143,
	0x144,		0x154,		0x155,		0x156,
	0x157,		0x158,		0x159,		0x15a,
	0x15b,		0x15c,		0x15d,		0
};

/* This table gives the values of the special function keys when modified
 * by the ALT key.  Note that S-F1 through S-F10 cannot be modified
 * by ALT and are not in this table.  This is a BIOS limitation.
 */
int	altmap[21] = {
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0x168,
	0x169,		0x16a,		0x16b,		0x16c,
	0x16d,		0x16e,		0x16f,		0x170,
	0x171
};

/* This table gives the values of the special function keys when modified
 * by the CTRL key.  Note that S-F1 through S-F10 cannot be modified
 * by CTRL and are not in this table.  This is a BIOS limitation.
 */
int	ctrlmap[21] = {
	0,		0,		0,		0x173,
	0x174,		0x184,		0x176,		0x177,
	0x175,		0,		0,		0x15e,
	0x15f,		0x160,		0x161,		0x162,
	0x163,		0x164,		0x165,		0x166,
	0x167
};

/*
 * This table lists all the values of ALT-A through ALT-Z, as
 * returned by getakey().  This table is searched so we can
 * convert ALT keys to META characters.
 */
int altaz[26] =
{
	0x11E,	0x130,	0x12E,	0x120,	0x112,	0x121,	0x122,	0x123,
	0x117,	0x124,	0x125,	0x126,	0x132,	0x131,	0x118,	0x119,
	0x110,	0x113,	0x11F,	0x114,	0x116,	0x12F,	0x111,	0x12D,
	0x115,	0x12C
};

/*
 * This table lists all the values of ALT-0 through ALT-9, as
 * returned by getakey().  This table is searched so we can
 * convert ALT keys to META characters.
 */
int alt09[10] =
{
	0x181,	0x178,	0x179,	0x17a,	0x17b,
	0x17c,	0x17d,	0x17e,	0x17f,	0x180
};

/*
 * Read in a key, doing the low level mapping
 * of ASCII code to 11 bit code. This level deals with
 * mapping the special keys into their spots in the C1
 * control area. The C0 controls go right through, and
 * get remapped by "getkey".  The keys ALT-A through
 * ALT-Z are mapped to META-A through META-Z.
 */
int getkbd()
{
	register int	c;
	register int	i;

	c = ttgetc();
	if (c < 0x100)				/* normal key?		*/
		return (c);			/* just return it	*/

	for (i = 0; i < 32; i++)		/* search SPECIAL map	*/
		if (c == specmap[i])		/* found it?		*/
			return (KFIRST + i);	/* return internal code */

	for (i = 0; i < 21; i++)		/* search ALT map	*/
		if (c == altmap[i])
			return (KMETA | (KFIRST + i));

	for (i = 0; i < 21; i++)		/* search CTRL map	*/
		if (c == ctrlmap[i])
			return (KCTRL | (KFIRST + i));

	for (i = 0; i < 26; i++)		/* search ALT A-Z map	*/
		if (c == altaz[i])
			return (KMETA | (i + 'A'));

	for (i = 0; i < 10; i++)		/* search ALT 0-9 map	*/
		if (c == alt09[i])
			return (KMETA | (i + '0'));

	return (KRANDOM);			/* not found		*/
}

/*
 * Terminal specific keymap initialization.
 * Attach the special keys to the appropriate built
 * in functions. Bind all of the assigned graphics in the
 * PC supplemental character set to "ins-self".
 * As is the case of all the keymap routines, errors
 * are very fatal.
 */
void ttykeymapinit()
{
	register SYMBOL	*sp;
	register int	i;

	keydup(KUP,		"back-line");
	keydup(KDOWN,		"forw-line");
	keydup(KLEFT,		"back-char");
	keydup(KRIGHT,		"forw-char");
	keydup(KCTRL|KLEFT,	"back-word");
	keydup(KCTRL|KRIGHT,	"forw-word");
	keydup(KPGUP,		"back-page");
	keydup(KPGDN,		"forw-page");
	keydup(KCTRL|KPGUP,	"up-window");
	keydup(KCTRL|KPGDN,	"down-window");
	keydup(KHOME,		"goto-bol");
	keydup(KEND,		"goto-eol");
	keydup(KCTRL|KHOME,	"goto-bob");
	keydup(KCTRL|KEND,	"goto-eob");
	keydup(KINS,		"set-overstrike");
	keydup(KDEL,		"forw-del-char");
	keydup(KF1,		"help");
	keydup(KF2,		"file-save");
	keydup(KF3,		"file-visit");
	keydup(KF4,		"quit");
	keydup(KF5,		"display-bindings");
	keydup(KF6,		"display-buffers");
	keydup(KF7,		"forw-window");
	keydup(KF8,		"forw-buffer");
	keydup(KF9,		"search-again");
	keydup(KF10,		"only-window");
	/*
	 * Bind all GR positions that correspond
	 * to assigned characters in the IBM PC special
	 * character set to "ins-self". These characters may
	 * be used just like any other character.
	 */

	if ((sp=symlookup("ins-self")) == NULL)
		abort();
	for (i=0xA0; i<0xFF; ++i) {
		if (binding[i] != NULL)
			abort();
		binding[i] = sp;
		++sp->s_nkey;
	}
}
