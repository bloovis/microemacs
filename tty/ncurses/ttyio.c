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

#include	"def.h"

#include	<errno.h>
#include	<signal.h>
#include	<unistd.h>
#include	<termios.h>
#include	<sys/ioctl.h>
#include	<ncurses.h>

static struct termios oldtty;	/* Old tty state		*/
static struct termios newtty;	/* New tty state		*/

int nrow;			/* Terminal size, rows.         */
int ncol;			/* Terminal size, columns.      */
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

int ttold (void)
{
  return tcsetattr (0, TCSANOW, &oldtty) >= 0;
}


/*
 * Set the tty to the "new" state, i.e., the state
 * it had after we changed it.  Return TRUE if successful,
 * FALSE otherwise.
 */

int ttnew (void)
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
  addch (c);
  return c;
}

/*
 * Write multiple characters to the display.
 * Use this entry point to optimization on some systems.
 * Here we just call ttputc.
 */
void
ttputs (const char *buf, int size)
{
  addnstr(buf, size);
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
  int c;

  waiting = TRUE;

  /* We get an error character if the window is resized, probably
   * because of an interrupted system call.  Just ignore those.
   */
  while ((c = getch ()) == ERR)
    ;
  waiting = FALSE;
  return c;
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