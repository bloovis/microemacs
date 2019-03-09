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
 *		Ncurses terminal I/O.
 *
 * By:		Mark Alexander
 *		marka@pobox.com
 *
 * The functions in this file
 * negotiate with the operating system for
 * keyboard characters, and write characters to
 * the display in a barely buffered fashion.
 */

#define _XOPEN_SOURCE_EXTENDED

#include	"def.h"

#include	<errno.h>
#include	<signal.h>
#include	<unistd.h>
#include	<termios.h>
#include	<sys/ioctl.h>
#ifdef __FreeBSD__
#include	<ncurses.h>
#else
#include	<ncursesw/ncurses.h>
#endif
#include	<locale.h>

static struct termios oldtty;	/* Old tty state		*/
static struct termios newtty;	/* New tty state		*/

int nrow;			/* Terminal size, rows.         */
int ncol;			/* Terminal size, columns.      */
int npages = 1;			/* Number of pages on terminal. */
int waiting;
int interrupted;

/*
 * set the tty size. Functionized for 43BSD.
 */
void
setttysize (void)
{
  getmaxyx (stdscr, nrow, ncol);
  if (nrow > NROW)		/* Don't crash if the   */
    nrow = NROW;		/* actual window size   */
  if (ncol > NCOL)		/* is too big.          */
    ncol = NCOL;
}

/*
 * This function gets called once, to set up
 * the terminal channel.
 */
void
ttopen (void)
{
  setlocale (LC_ALL, "");
  tcgetattr (0, &oldtty);
  initscr ();			/* initialize the curses library */
  keypad (stdscr, TRUE);	/* enable keyboard mapping */
  nonl ();			/* tell curses not to do NL->CR/NL on output */
  cbreak ();			/* take input chars one at a time, no wait for \n */
  noecho ();
  raw ();
  setttysize ();
  tcgetattr (0, &newtty);
}


/*
 * Set the tty to the "old" state, i.e., the state
 * it had before we changed it.  Return TRUE if successful,
 * FALSE otherwise.
 */

int
ttold (void)
{
  return tcsetattr (0, TCSANOW, &oldtty) >= 0;
}


/*
 * Set the tty to the "new" state, i.e., the state
 * it had after we changed it.  Return TRUE if successful,
 * FALSE otherwise.
 */

int
ttnew (void)
{
  return tcsetattr (0, TCSANOW, &newtty) >= 0;
}


/*
 * This function gets called just
 * before we go back home to the shell. Put all of
 * the terminal parameters back.
 */
void
ttclose (void)
{
  endwin ();
  tcsetattr (0, TCSANOW, &oldtty);
}

/*
 * Check for keyboard typeahead.  Return TRUE if any characters have
 * been typed.
 */
int
ttstat (void)
{
  return FALSE;
}

/*
 * Write character to the display.
 * Characters are buffered up, to make things
 * a little bit more efficient.
 */
int
ttputc (int c)
{
  cchar_t wcval;
  wchar_t wch[2];

  wch[0] = c;
  wch[1] = 0;
  setcchar (&wcval, wch, 0, 0, NULL);
  add_wch (&wcval);
  return c;
}

/*
 * Insert character in the display.  Characters to the right
 * of the insertion point are moved one space to the right.
 */
int
ttinsertc (int c)
{
  cchar_t wcval;
  wchar_t wch[2];

  wch[0] = c;
  wch[1] = 0;
  setcchar (&wcval, wch, 0, 0, NULL);
  ins_wch (&wcval);
  return c;
}

/*
 * Delete character in the display.  Characters to the right
 * of the deletion point are moved one space to the left.
 */
void
ttdelc (void)
{
  delch ();
}

/*
 * Write multiple characters to the display.
 * Use this entry point to do optimization on some systems.
 * Here we use the wide character functions of curses
 * so that Unicode characters will be displayed correctly.
 */
void
ttputs (const wchar_t *buf, int size)
{
  static cchar_t wcval[NCOL];
  wchar_t wch[3];
  wchar_t modifier = 0;
  int i;
  int wsize = 0;

  for (i = 0; i < size; i++)
    {
      wch[0] = buf[i];
      if (modifier != 0)
	{
	  wch[1] = modifier;
	  modifier = 0;
	  wch[2] = 0;
	}
      else
	{
	  if (ucombining (wch[0]))
	    {
	      modifier = wch[0];
	      continue;
	    }
	  wch[1] = 0;
	}
      setcchar (&wcval[wsize], wch, 0, 0, NULL);
      ++wsize;
    }
  add_wchnstr (wcval, wsize);
}

/*
 * Flush output.
 */
void
ttflush (void)
{
   refresh ();
}

/*
 * Read character from terminal.
 * All 8 bits are returned, so that you can use
 * a multi-national terminal.
 */
int
ttgetc (void)
{
  wint_t c;

  waiting = TRUE;

  /* We get an error character if the window is resized, probably
   * because of an interrupted system call.  Just ignore those.
   */
  while (get_wch (&c) == ERR)
    ;
  waiting = FALSE;
  return (int) c;
}

/*
 * panic - just exit, as quickly as we can.
 */
void
panic (char *s)
{
  fprintf (stderr, "panic: %s\n", s);
  abort ();			/* To leave a core image. */
}
