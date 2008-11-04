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
 *		Character class tables.
 * Version:	29
 * Last edit:	08-Jul-86
 * By:		rex::conroy
 *		decvax!decwrl!dec-rhea!dec-rex!conroy
 * Modified by:	Mark Alexander
 *		drivax!alexande
 *
 * Do it yourself character classification
 * macros, that understand the multinational character set,
 * and let me ask some questions the standard macros (in
 * ctype.h) don't let you ask.
 */
#include	"def.h"

/*
 * This table, indexed by a character drawn
 * from the 256 member character set, is used by my
 * own character type macros to answer questions about the
 * type of a character. It handles the full multinational
 * character set, and lets me ask some questions that the
 * standard "ctype" macros cannot ask.
 */
char cinfo[256] = {
	_C,		_C,		_C,		_C,	/* 0x0X	*/
	_C,		_C,		_C,		_C,
	_C,		_C,		_C,		_C,
	_C,		_C,		_C,		_C,
	_C,		_C,		_C,		_C,	/* 0x1X	*/
	_C,		_C,		_C,		_C,
	_C,		_C,		_C,		_C,
	_C,		_C,		_C,		_C,
	0,		_P,		0,		0,	/* 0x2X	*/
	_W,		0,		0,		_W,
	0,		0,		0,		0,
	0,		0,		_P,		0,
	_W,		_W,		_W,		_W,	/* 0x3X	*/
	_W,		_W,		_W,		_W,
	_W,		_W,		0,		0,
	0,		0,		0,		_P,
	0,		_U|_W,		_U|_W,		_U|_W,	/* 0x4X	*/
	_U|_W,		_U|_W,		_U|_W,		_U|_W,
	_U|_W,		_U|_W,		_U|_W,		_U|_W,
	_U|_W,		_U|_W,		_U|_W,		_U|_W,
	_U|_W,		_U|_W,		_U|_W,		_U|_W,	/* 0x5X	*/
	_U|_W,		_U|_W,		_U|_W,		_U|_W,
	_U|_W,		_U|_W,		_U|_W,		0,
	0,		0,		0,		_W,
	0,		_L|_W,		_L|_W,		_L|_W,	/* 0x6X	*/
	_L|_W,		_L|_W,		_L|_W,		_L|_W,
	_L|_W,		_L|_W,		_L|_W,		_L|_W,
	_L|_W,		_L|_W,		_L|_W,		_L|_W,
	_L|_W,		_L|_W,		_L|_W,		_L|_W,	/* 0x7X	*/
	_L|_W,		_L|_W,		_L|_W,		_L|_W,
	_L|_W,		_L|_W,		_L|_W,		0,
	0,		0,		0,		_C,
	0,		0,		0,		0,	/* 0x8X	*/
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0,	/* 0x9X	*/
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0,	/* 0xAX	*/
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0,	/* 0xBX	*/
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0,
	_U|_W,		_U|_W,		_U|_W,		_U|_W,	/* 0xCX	*/
	_U|_W,		_U|_W,		_U|_W,		_U|_W,
	_U|_W,		_U|_W,		_U|_W,		_U|_W,
	_U|_W,		_U|_W,		_U|_W,		_U|_W,
	0,		_U|_W,		_U|_W,		_U|_W,	/* 0xDX	*/
	_U|_W,		_U|_W,		_U|_W,		_U|_W,
	_U|_W,		_U|_W,		_U|_W,		_U|_W,
	_U|_W,		_U|_W,		0,		_W,
	_L|_W,		_L|_W,		_L|_W,		_L|_W,	/* 0xEX	*/
	_L|_W,		_L|_W,		_L|_W,		_L|_W,
	_L|_W,		_L|_W,		_L|_W,		_L|_W,
	_L|_W,		_L|_W,		_L|_W,		_L|_W,
	0,		_L|_W,		_L|_W,		_L|_W,	/* 0xFX	*/
	_L|_W,		_L|_W,		_L|_W,		_L|_W,
	_L|_W,		_L|_W,		_L|_W,		_L|_W,
	_L|_W,		_L|_W,		0,		0
};


/*
 * The following table, indexed by a character drawn
 * from the 256 member character set, converts a character
 * to upper case.  Unlike the TOUPPER macro, it will not
 * convert characters that are ALREADY upper case.
 * This table is used by the search functions for fast
 * character comparisons, using the EQ macro.
 */

char upmap[256] = { 0 };

/*
 * This function initializes the upmap table above.
 */

void
upmapinit (void)
{
  register int i;

  for (i = 0; i < 256; i++)
    {
      if (ISLOWER (i) && casefold)
	upmap[i] = TOUPPER (i);
      else
	upmap[i] = i;
    }
}
