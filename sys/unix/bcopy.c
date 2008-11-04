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
 *		Ultrix-32 byte copy routines
 * Version:	1
 * Last edit:	01-Jul-86
 * By:		Mark Alexander
 *		drivax!alexande
 *
 * The functions in this file copy and fill bytes.  They should be
 * translated into assembler for speed, as they were on the 68K
 * and the 8088.  For now, the C versions will do until I figure
 * out VAX assembler.
 */


/*----------------------------------------------------------------------*/

bcopy (source, dest, count)
     register char *source, *dest;
     register int count;
{
  /* This function copies count bytes from source to dest
   * in ascending order.
   */

  while (count-- != 0)
    *dest++ = *source++;
}


/*----------------------------------------------------------------------*/

bcopyr (source, dest, count)
     register char *source, *dest;
     register int count;
{
  /* This function copies count bytes from source to dest
   * in descending order, starting at the right end of the
   * buffers.
   */

  source += count;
  dest += count;
  while (count-- != 0)
    *--dest = *--source;
}


/*----------------------------------------------------------------------*/

bfill (c, buffer, count)
     register char c, *buffer;
     register int count;
{
  /* This function fills count bytes at buffer with the character c.
   */

  while (count-- != 0)
    *buffer++ = c;
}
