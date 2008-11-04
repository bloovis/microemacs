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

/*----------------------------------------------------------------------
 *	bcopy - byte string operations for 4.2 BSD
 */

/*----------------------------------------------------------------------
 * BCOPYR - move BYTEs from source to destination, start at right end
 */
void
bcopyr (char *source, char *dest, int count)
{
  source += count;
  dest += count;
  while (count-- != 0)
    *--dest = *--source;
}

/*----------------------------------------------------------------------
 * BFILL - fill a buffer with a single BYTE value
 */
void
bfill (char c, char *buffer, int count)
{
  while (count-- != 0)
    *buffer++ = c;
#if 0
  memset (buffer, c, count);
#endif
}
