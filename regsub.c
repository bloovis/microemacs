/*
    Copyright (C) 2018 Mark Alexander

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
 * regsub
 *
 *	Copyright (c) 1986 by University of Toronto.
 *	Written by Henry Spencer.  Not derived from licensed software.
 *
 *	Permission is granted to anyone to use this software for any
 *	purpose on any computer system, and to redistribute it freely,
 *	subject to the following restrictions:
 *
 *	1. The author is not responsible for the consequences of use of
 *		this software, no matter how awful, even if they arise
 *		from defects in it.
 *
 *	2. The origin of this software must not be misrepresented, either
 *		by explicit claim or by omission.
 *
 *	3. Altered versions must be plainly marked as such, and must not
 *		be misrepresented as being the original software.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <regexp.h>
#include "regmagic.h"

/*
 * regsub - perform substitutions after a regexp match
 *
 * Return 1 if success, 0 if failure.
 */
int
regsub (const regexp * rp, const char *source, char *dest, int destlen)
{
  regexp *const prog = (regexp *) rp;
  char *src = (char *) source;
  char *dst = dest;
  const char *end = dest + destlen;
  char c;
  int no;
  size_t len;

  if (prog == NULL || source == NULL || dest == NULL)
    {
      regerror ("NULL parameter to regsub");
      return 0;
    }
  if ((unsigned char) *(prog->program) != MAGIC)
    {
      regerror ("damaged regexp");
      return 0;
    }

  while ((c = *src++) != '\0')
    {
      if (c == '&')
	no = 0;
      else if (c == '\\' && isdigit (*src))
	no = *src++ - '0';
      else
	no = -1;

      if (no < 0)
	{			/* Ordinary character. */
	  if (c == '\\' && (*src == '\\' || *src == '&'))
	    c = *src++;
	  if (dst >= end)
	    {
	      regerror ("destination buffer too small");
	      return 0;
	    }
	  *dst++ = c;
	}
      else if (prog->startp[no] != NULL && prog->endp[no] != NULL &&
	       prog->endp[no] > prog->startp[no])
	{
	  len = prog->endp[no] - prog->startp[no];
	  if (dst + len >= end)
	    {
	      regerror ("destination buffer too small");
	      return 0;
	    }

	  (void) strncpy (dst, prog->startp[no], len);
	  dst += len;
	  if (*(dst - 1) == '\0')
	    {			/* strncpy hit NUL. */
	      regerror ("damaged match string");
	      return 0;
	    }
	}
    }

  if (dst >= end)
    {
      regerror ("destination buffer too small");
      return 0;
    }
  *dst++ = '\0';
  return 1;
}
