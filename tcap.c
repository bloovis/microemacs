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
#include <stdlib.h>
#include <string.h>

#if 0
#include	<sys/ioctl.h>
/* #include	<sys/filio.h> */
/* #include	<termios.h> */
#include <sgtty.h>
#else
#include <curses.h>
#include <term.h>
#endif

#ifdef  TIOCGWINSZ
/* #error    blorch; */
#endif

#define TCAPSLEN 1024

char tcapbuf[TCAPSLEN];
static char tcbuf[1024];
char *tgetstr ();

void
printesc (p)
     char *p;
{
  char c;
  while ((c = *p++) != '\0')
    if (c < ' ')
      printf ("^%c", c + '@');
    else
      printf ("%c", c);
}

int
main (argc, argv)
     int argc;
     char *argv[];
{
  char *tv_stype, *p, *t;
  int i;

/*	ttopen(); */
  if ((tv_stype = getenv ("TERM")) == NULL)	/* Don't want VAX C getenv() */
    {
      printf ("Environment variable TERM not defined!");
      return;
    }

  if ((tgetent (tcbuf, tv_stype)) != 1)
    {
      printf ("Unknown terminal type %s\n", tv_stype);
      return;
    }

  p = tcapbuf;
  for (i = 1; i < argc; i++)
    {
      t = tgetstr (argv[i], &p);
      if (t == NULL)
	printf ("%s not defined for this terminal\n", argv[i]);
      else
	{
	  printf ("%s: ", argv[i]);
	  printesc (t);
	  printf ("\n");
	}
      printf ("%s (numeric): %d\n", argv[i], tgetnum (argv[i]));
    }
  return;
}
