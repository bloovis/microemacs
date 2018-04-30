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
 *		Ultrix-32 terminal I/O.
 * Version:	29
 * Last edit:	19-Jan-87
 * By:		Mark Alexander
 *		drivax!alexande
 *
 * The functions in this file
 * negotiate with the operating system for
 * keyboard characters, and write characters to
 * the display in a barely buffered fashion.
 */
#include	"def.h"

#ifdef __hpux
/* Need this kludge to get dirent.h to define POSIX stuff */
#define _INCLUDE_POSIX_SOURCE
#define _INCLUDE_HPUX_SOURCE
#define _INCLUDE_XOPEN_SOURCE
#endif

#include	<errno.h>
#include	<unistd.h>
#include	<sys/ioctl.h>
#ifdef sun
#include	<sys/filio.h>
#endif
#if HAVE_UNISTD_H
#include        <unistd.h>
#endif
#include	<termios.h>
#include	<termcap.h>

/* The following kludge is for SunOS */
#ifndef _POSIX_VDISABLE
#define _POSIX_VDISABLE 0
#endif

#define	NOBUF	512		/* Output buffer size.          */

static char obuf[NOBUF];	/* Output buffer.               */
static int nobuf;
static struct termios oldtty;	/* Old tty state		*/
static struct termios newtty;	/* New tty state		*/

int nrow;			/* Terminal size, rows.         */
int ncol;			/* Terminal size, columns.      */
int npages = 1;			/* Number of pages on terminal. */

/*
 * This function gets called once, to set up
 * the terminal channel. On Ultrix is's tricky, since
 * we want flow control, but we don't want any characters
 * stolen to send signals. Use CBREAK mode, and set all
 * characters but start and stop to 0xFF.
 */
void
ttopen (void)
{
  extern char *getenv ();
  register char *tv_stype;
  char tcbuf[1024], err_str[72];

  if (tcgetattr (0, &oldtty) < 0)
    abort ();
  newtty = oldtty;
  newtty.c_iflag &= ~(ICRNL | INLCR | IGNCR);	/* Kill CR<=>NL */
  newtty.c_iflag |= IGNBRK;	/* Ignore break         */
  newtty.c_lflag &= ~(ISIG | ICANON | ECHO | ECHOE | ECHOK | ECHONL);
  /* Kill echo and signals */
  newtty.c_cc[VINTR] = _POSIX_VDISABLE;	/* Interrupt.           */
  newtty.c_cc[VQUIT] = _POSIX_VDISABLE;	/* Quit.                */
  if (xflag)			/* -x (xon/xoff) option? */
    {
      newtty.c_cc[VSTART] = 0x11;	/* ^Q, for terminal.    */
      newtty.c_cc[VSTOP] = 0x13;	/* ^S, for terminal.    */
#if defined(_AIX) || defined(__hpux)
      newtty.c_iflag |= IXON;
#endif
    }
  else				/* don't use xon/xoff       */
    {
      newtty.c_cc[VSTART] = _POSIX_VDISABLE;
      newtty.c_cc[VSTOP] = _POSIX_VDISABLE;
#if defined(_AIX) || defined(__hpux)
      newtty.c_iflag &= ~IXON;
#endif
    }
  newtty.c_cc[VEOF] = _POSIX_VDISABLE;
  newtty.c_cc[VKILL] = _POSIX_VDISABLE;
  newtty.c_cc[VSUSP] = _POSIX_VDISABLE;	/* Suspend #1.          */
#if !defined(__linux__) && !defined(__hpux)
  newtty.c_cc[VDSUSP] = _POSIX_VDISABLE;	/* Suspend #2.          */
#endif
#if !defined(__hpux)
  newtty.c_cc[VREPRINT] = _POSIX_VDISABLE;
#endif
#ifdef _AIX
  newtty.c_cc[VDISCRD] = _POSIX_VDISABLE;	/* Output flush.        */
  newtty.c_cc[VWERSE] = _POSIX_VDISABLE;
#elif !defined(__hpux)
  newtty.c_cc[VDISCARD] = _POSIX_VDISABLE;	/* Output flush.        */
  newtty.c_cc[VWERASE] = _POSIX_VDISABLE;
#endif
#ifndef __hpux
  newtty.c_cc[VLNEXT] = _POSIX_VDISABLE;	/* Literal next.        */
#endif
  newtty.c_cc[VMIN] = 1;	/* No. of characters    */
  if (tcsetattr (0, TCSANOW, &newtty) < 0)
    abort ();

/* do this the REAL way */
  if ((tv_stype = getenv ("TERM")) == NULL)
    {
      puts ("Environment variable TERM not defined!");
      exit (1);
    }

  if ((tgetent (tcbuf, tv_stype)) != 1)
    {
      (void) sprintf (err_str, "Unknown terminal type %s!", tv_stype);
      puts (err_str);
      exit (1);
    }

  setttysize ();
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
 * set the tty size. Functionized for 43BSD.
 */
void
setttysize (void)
{

#ifdef	TIOCGWINSZ
  struct winsize ws;

  if (ioctl (0, TIOCGWINSZ, (char *) &ws) == 0)
    {
      nrow = ws.ws_row;
      ncol = ws.ws_col;
    }
  else
    nrow = 0;
  if (nrow <= 0 || ncol <= 0)
#endif
    if ((nrow = tgetnum ("li")) <= 0 || (ncol = tgetnum ("co")) <= 0)
      {
	nrow = 24;
	ncol = 80;
      }
  if (nrow > NROW)		/* Don't crash if the   */
    nrow = NROW;		/* termcap entry is     */
  if (ncol > NCOL)		/* too big.             */
    ncol = NCOL;
}

/*
 * This function gets called just
 * before we go back home to the shell. Put all of
 * the terminal parameters back.
 */
void
ttclose (void)
{
  ttflush ();
  if (tcsetattr (0, TCSANOW, &oldtty) < 0)
    abort ();
}

/*
 * Check for keyboard typeahead.  Return TRUE if any characters have
 * been typed.
 */
int
ttstat (void)
{
  int n;

  if (ioctl (0, FIONREAD, &n) < 0)
    return (FALSE);
  return (n > 0);
}

/*
 * Write character to the display.
 * Characters are buffered up, to make things
 * a little bit more efficient.
 */
int
ttputc (int c)
{
  static uchar buf[6];
  int i, len;

  /* Convert Unicode to UTF-8 and output each byte.
   */
  len = uputc (c, buf);
  for (i = 0; i < len; i++)
    {
      if (nobuf >= NOBUF)
	ttflush ();
      obuf[nobuf++] = buf[i];
    }
  return c;
}

/*
 * Write multiple characters to the display.
 * Use this entry point to optimization on some systems.
 * Here we just call ttputc.
 */
void
ttputs (const wchar_t *buf, int size)
{
  while (size--)
    ttputc (*buf++);
}

/*
 * Flush output.
 */
void
ttflush (void)
{
  if (nobuf != 0)
    {
      if (write (1, obuf, nobuf) != nobuf)
	fprintf (stderr, "Write error!\n");
      nobuf = 0;
    }
}

/*
 * Read character from terminal.  Read
 * UTF-8 input and convert it to Unicode.
 */
int
ttgetc (void)
{
  uchar buf[6];
  int i, ret, len;

  if ((ret = read (0, &buf[0], 1)) != 1)
    {
      /* fprintf(stderr,"read returned %d, errno = %d\n", ret, errno); */
      return 0;
    }
  len = uclen (buf);
  for (i = 1; i < len; i++)
    {
      if ((ret = read (0, &buf[i], 1)) != 1)
	{
	  /* fprintf(stderr,"read returned %d, errno = %d\n", ret, errno); */
	  break;
	}
    }
  return ugetc (buf, 0, NULL);
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
