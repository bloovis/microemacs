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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "regexp.h"

int
main (int argc, const char *argv[])
{
  const char *rex, *str, *repl;
  int i, len;
  regexp *r;
  static char dest[256];
  char *copy;
  const char *start;
  const char *end;
  const char *match;

  if (argc != 4)
    {
      printf("usage: regtest regexp replacement string\n");
      return 1;
    }
  rex = argv[1];
  repl = argv[2];
  str = argv[3];
  r = regcomp (rex);
  i = regexec (r, str);
  printf ("Result is of regexec (/%s/, \"%s\" is %d\n", rex, str, i);
  if (i != 0)
    {
      for (i = 0; i < NSUBEXP; i++)
	{
	  start = r->startp[i];
	  if (start == NULL)
	    continue;
	  end = r->endp[i];
	  len = end - start;
	  copy = malloc (len + 1);
	  strncpy (copy, start, len);
	  copy[len] = '\0';
	  printf("group %d = \"%s\"\n", i, copy);
	  if (i == 0)
	    match = copy;
	  else
	    free (copy);
	}

      /* Do the substitution */
      if (regsub (r, repl, dest, sizeof (dest)))
	printf ("Replaced '%s' with '%s' using replacement pattern '%s'\n", match, dest, repl);
      else
	printf ("Error in replacement pattern '%s'\n", repl);
    }
  return 0;
}

void
regerror (const char *s)
{
  printf ("regerror: %s\n", s);
}
