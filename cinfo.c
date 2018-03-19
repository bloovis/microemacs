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
#include	<wctype.h>

/*
 * These flags, and the macros below them,
 * make up a do-it-yourself set of "ctype" macros that
 * understand only the ASCII character set, and let me ask
 * a slightly different set of questions.
 */
#define _W	0x01		/* Word.                        */
#define _U	0x02		/* Upper case letter.           */
#define _L	0x04		/* Lower case letter.           */
#define _C	0x08		/* Control.                     */
#define _P	0x10		/* End of sentence punctuation. */

#define AISWORD(c)	(cinfo[((c)&0x7f)]&_W)
#define AISCTRL(c)	(cinfo[((c)&0x7f)]&_C)
#define AISUPPER(c)	(cinfo[((c)&0x7f)]&_U)
#define AISLOWER(c)	(cinfo[((c)&0x7f)]&_L)
#define AISEOSP(c)	(cinfo[((c)&0x7f)]&_P)
#define ATOUPPER(c)	((c)-0x20)
#define ATOLOWER(c)	((c)+0x20)
#define	AEQ(c1,c2)	(upmap[(c1)&0x7f]==upmap[(c2)&0x7f])

/*
 * This table, indexed by a character drawn
 * from the 128-member character set, is used by my
 * own character type macros to answer questions about the
 * type of a character. It handles the full multinational
 * character set, and lets me ask some questions that the
 * standard "ctype" macros cannot ask.
 */
char cinfo[128] = {
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
};


/*
 * The following table, indexed by a character drawn
 * from the 128 member character set, converts a character
 * to upper case.  Unlike the TOUPPER macro, it will not
 * convert characters that are ALREADY upper case.
 * This table is used by the search functions for fast
 * character comparisons, using the EQ macro.
 */

char upmap[128] = { 0 };

/*
 * This function initializes the upmap table above.
 */

void
upmapinit (void)
{
  register int i;

  for (i = 0; i < 128; i++)
    {
      if (AISLOWER (i) && casefold)
	upmap[i] = ATOUPPER (i);
      else
	upmap[i] = i;
    }
}

/*
 * Return TRUE if c is a "word" character: alphanumeric or $ or '.
 */
int
cisword (wchar_t c)
{
  if (c < 0x80)
    return AISWORD(c);
  else
    return iswalnum (c);
}

/*
 * Return TRUE if c is a "letter" character.
 */
int
cisalpha (wchar_t c)
{
  if (c < 0x80)
    return AISUPPER(c) || AISLOWER(c);
  else
    return iswalpha (c);
}

/*
 * Return TRUE if c is a control character.
 */
int
cisctrl (wchar_t c)
{
  if (c < 0x80)
    return AISCTRL(c);
  else
    return FALSE;
}


/*
 * Return TRUE if c is an upper case letter.
 */
int
cisupper (wchar_t c)
{
  if (c < 0x80)
    return AISUPPER(c);
  else
    return iswupper(c);
}


/*
 * Return TRUE if c is a lower case letter.
 */
int
cislower (wchar_t c)
{
  if (c < 0x80)
    return AISLOWER(c);
  else
    return iswlower(c);
}


/*
 * Return TRUE if c is an end of sentence punctuation character.
 */
int
ciseosp (wchar_t c)
{
  if (c < 0x80)
    return AISEOSP(c);
  else
    return FALSE;
}


/*
 * Convert the character to upper case.
 */
wchar_t
ctoupper (wchar_t c)
{
  if (c < 0x80)
    return ATOUPPER(c);
  else
    return towupper(c);
}


/*
 * Convert the character to lower case.
 */
wchar_t
ctolower (wchar_t c)
{
  if (c < 0x80)
    return ATOLOWER(c);
  else
    return towlower(c);
}


/*
 * Are the two characters equal according to the
 * casefold global variable?
 */
int
ceq (wchar_t c1, wchar_t c2)
{
  if (c1 < 0x80 && c2 < 0x80)
    return AEQ(c1,c2);
  else if (casefold)
    return towupper(c1) == towupper(c2);
  else
    return c1 == c2;
}
