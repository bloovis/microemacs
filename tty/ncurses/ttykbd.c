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

/*
 * Name:	MicroEMACS
 * 		Ncurses Keyboard
 *
 * By:		Mark Alexander
 *		marka@pobox.com
 *
 * $Log: ttykbd.c,v $
 * Revision 1.1  2005-10-18 02:18:44  bloovis
 * New files to implement ncurses screen handling.
 *
 * 
 */

#include	"def.h"

#ifdef __FreeBSD__
#include	<ncurses.h>
#else
#include	<ncursesw/ncurses.h>
#endif

#define CTRL(x) ((x) & 0x1f)

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
	0,		KEY_UP,		KEY_DOWN,	KEY_LEFT,
	KEY_RIGHT,	KEY_PPAGE,	KEY_NPAGE,	KEY_HOME,
	KEY_END,	KEY_IC,		KEY_DC,		KEY_F(1),
	KEY_F(2),	KEY_F(3),	KEY_F(4),	KEY_F(5),
	KEY_F(6),	KEY_F(7),	KEY_F(8),	KEY_F(9),
	KEY_F(10),	KEY_F(11),	KEY_F(12),	KEY_F(13),
	KEY_F(14),	KEY_F(15),	KEY_F(16),	KEY_F(17),
	KEY_F(18),	KEY_F(19),	KEY_F(20),	0
};

/*
 * Read in a key, doing the low level mapping
 * of Unicode to 32 bit internal key code. This level deals with
 * mapping the special keys into their spots in the C1
 * control area. The C0 controls go right through, and
 * get remapped by "getkey".  The keys ALT-A through
 * ALT-Z are mapped to META-A through META-Z.
 */
int
getkbd (void)
{
  register int	c;
  register int	i;

  c = ttgetc ();
  if (c < KEY_MIN)			/* normal key?		*/
     return (c);			/* just return it	*/

  /* Treat backspace as Ctrl-H. */
  if (c == KEY_BACKSPACE)
    return CTRL('H');

  /* Treat window resize as Ctrl-L, which will force a screen redraw. */
  if (c == KEY_RESIZE)
    return CTRL('L');

  for (i = 0; i < 32; i++)		/* search SPECIAL map	*/
    if (c == specmap[i])		/* found it?		*/
      return (KFIRST + i);		/* return internal code */
  return (c);				/* not found, maybe Unicode? */
}

/*
 * Terminal specific keymap initialization.
 * Attach the special keys to the appropriate built
 * in functions. Bind all of the assigned graphics in the
 * PC supplemental character set to "ins-self".
 * As is the case of all the keymap routines, errors
 * are very fatal.
 */
void
ttykeymapinit (void)
{
  keydup (KUP,		"back-line");
  keydup (KDOWN,	"forw-line");
  keydup (KLEFT,	"back-char");
  keydup (KRIGHT,	"forw-char");
  keydup (KCTRL|KLEFT,	"back-word");
  keydup (KCTRL|KRIGHT,	"forw-word");
  keydup (KPGUP,	"back-page");
  keydup (KPGDN,	"forw-page");
  keydup (KCTRL|KPGUP,	"up-window");
  keydup (KCTRL|KPGDN,	"down-window");
  keydup (KHOME,	"goto-bol");
  keydup (KEND,		"goto-eol");
  keydup (KCTRL|KHOME,	"goto-bob");
  keydup (KCTRL|KEND,	"goto-eob");
  keydup (KINS,		"set-overstrike");
  keydup (KDEL,		"forw-del-char");
  keydup (KF1,		"help");
  keydup (KF2,		"file-save");
  keydup (KF3,		"file-visit");
  keydup (KF4,		"quit");
  keydup (KF5,		"undo");
#if USE_RUBY
  keydup (KF6,		"ruby-string");
#else
  keydup (KF6,		"display-buffers");
#endif
  keydup (KF7,		"redo");
  keydup (KF8,		"forw-buffer");
  keydup (KF9,		"search-again");
  keydup (KF10,		"only-window");
  keydup (KF11,		"find-cscope");
  keydup (KF12,		"next-cscope");
}
