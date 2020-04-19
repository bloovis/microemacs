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
 *		Basic cursor motion commands.
 * Version:	29
 * Last edit:	19-Jan-87
 * By:		rex::conroy
 *		decvax!decwrl!dec-rhea!dec-rex!conroy
 * Modified by:	Mark Alexander
 *		drivax!alexande
 *
 * The routines in this file are the basic
 * command functions for moving the cursor around on
 * the screen, setting mark, and swapping dot with
 * mark. Only moves between lines, which might make the
 * current buffer framing bad, are hard.
 */
#include	"def.h"

/*
 * Go to beginning of line.
 */
int
gotobol (int f, int n, int k)
{
  curwp->w_dot.o = 0;
  return (TRUE);
}

/*
 * Move cursor backwards. Do the
 * right thing if the count is less than
 * 0. Error if you try to move back from
 * the beginning of the buffer.
 */
int
backchar (int f, int n, int k)
{
  register LINE *lp;

  if (n < 0)
    return (forwchar (f, -n, KRANDOM));
  while (n--)
    {
      if (curwp->w_dot.o == 0)
	{
	  if ((lp = lback (curwp->w_dot.p)) == curbp->b_linep)
	    return (FALSE);
	  curwp->w_dot.p = lp;
	  curwp->w_dot.o = wllength (lp);
	  curwp->w_flag |= WFMOVE;
	}
      else
	curwp->w_dot.o--;
    }
  return (TRUE);
}

/*
 * Go to end of line.
 */
int
gotoeol (int f, int n, int k)
{
  curwp->w_dot.o = wllength (curwp->w_dot.p);
  return (TRUE);
}

/*
 * Move cursor forwards. Do the
 * right thing if the count is less than
 * 0. Error if you try to move forward
 * from the end of the buffer.
 */
int
forwchar (int f, int n, int k)
{
  register LINE *lp;

  if (n < 0)
    return (backchar (f, -n, KRANDOM));
  while (n--)
    {
      if (curwp->w_dot.o == wllength (curwp->w_dot.p))
	{
	  if (curwp->w_dot.p == curbp->b_linep)
	    return (FALSE);
	  if ((lp = lforw (curwp->w_dot.p)) == curbp->b_linep)
	    return (FALSE);
	  curwp->w_dot.p = lp;
	  curwp->w_dot.o = 0;
	  curwp->w_flag |= WFMOVE;
	}
      else
	curwp->w_dot.o++;
    }
  return (TRUE);
}

/*
 * Go to the beginning of the
 * buffer. Setting WFHARD is conservative,
 * but almost always the case.
 */
int
gotobob (int f, int n, int k)
{
  curwp->w_dot.p = firstline (curbp);
  curwp->w_dot.o = 0;
  curwp->w_flag |= WFHARD;
  return (TRUE);
}

/*
 * Go to the end of the buffer.
 * Setting WFHARD is conservative, but
 * almost always the case.
 */
int
gotoeob (int f, int n, int k)
{
  curwp->w_dot.p = lastline (curbp);
  curwp->w_dot.o = wllength (curwp->w_dot.p);
  curwp->w_flag |= WFHARD;
  return (TRUE);
}

/*
 * Set the current goal column,
 * which is saved in the external variable "curgoal",
 * to the current cursor column. The column is never off
 * the edge of the screen; it's more like display then
 * show position.
 */
static void
setgoal (void)
{
  curgoal = getcolpos () - 1;	/* Get the position.    */
}

/*
 * This routine looks at a line (pointed
 * to by the LINE pointer "dlp") and the current
 * vertical motion goal column (set by the "setgoal"
 * routine above) and returns the best offset to use
 * when a vertical motion is made into the line.
 */
static int
getgoal (LINE *dlp)
{
  register int c;
  register int col;
  register int newcol;
  int dbo;
  int ulen;
  const uchar *s, *end;

  col = 0;
  dbo = 0;
  s = lgets (dlp);
  end = s + llength (dlp);
  while (s < end)
    {
      c = ugetc (s, 0, &ulen);
      newcol = col;
      if (c == '\t')
	newcol += tabsize - newcol % tabsize - 1;
      else if (c < 0x80 && CISCTRL (c) != FALSE)
	++newcol;
      ++newcol;
      if (newcol > curgoal)
	break;
      col = newcol;
      ++dbo;
      s += ulen;
    }
  return (dbo);
}

/*
 * Move forward by full lines.
 * If the number of lines to move is less
 * than zero, call the backward line function to
 * actually do it. The last command controls how
 * the goal column is set.  Return TRUE if the
 * move was actually performed; FALSE otherwise
 * (e.g., if we were already on the last line).
 */
int
forwline (int f, int n, int k)
{
  register LINE *dlp;
  int ret;

  if (n < 0)
    return (backline (f, -n, KRANDOM));
  if ((lastflag & CFCPCN) == 0)	/* Fix goal.            */
    setgoal ();
  thisflag |= CFCPCN;
  dlp = curwp->w_dot.p;
  ret = FALSE;
  while (n-- && dlp != lastline (curbp))
    {
      dlp = lforw (dlp);
      ret = TRUE;
    }
  curwp->w_dot.p = dlp;
  curwp->w_dot.o = getgoal (dlp);
  curwp->w_flag |= WFMOVE;
  return ret;
}

/*
 * This function is like "forwline", but
 * goes backwards. The scheme is exactly the same.
 * Check for arguments that are less than zero and
 * call your alternate. Figure out the new line and
 * call "movedot" to perform the motion.  Return TRUE if the
 * move was actually performed; FALSE otherwise
 * (e.g., if we were already on the first line).
 */
int
backline (int f, int n, int k)
{
  register LINE *dlp;
  int ret;

  if (n < 0)
    return (forwline (f, -n, KRANDOM));
  if ((lastflag & CFCPCN) == 0)	/* Fix goal.            */
    setgoal ();
  thisflag |= CFCPCN;
  dlp = curwp->w_dot.p;
  ret = FALSE;
  while (n-- && dlp != firstline (curbp))
    {
      dlp = lback (dlp);
      ret = TRUE;
    }
  curwp->w_dot.p = dlp;
  curwp->w_dot.o = getgoal (dlp);
  curwp->w_flag |= WFMOVE;
  return ret;
}

/*
 * Check if the dot must move after the current window has been
 * scrolled by forw-page or back-page.
 */
void
checkdot (void)
{
  register LINE *lp, *dotp;
  LINE *newdotp;
  register int total, half;

  total = curwp->w_ntrows;	/* size of window       */
  half = total / 2;		/* half of window size  */
  lp = curwp->w_linep;		/* top of window        */
  dotp = curwp->w_dot.p;	/* current value of dot */
  newdotp = NULL;

  while (total-- && lp != curbp->b_linep)
    {
      if (lp == dotp)		/* is dot in window?    */
	return;			/* yes - do nothing     */
      if (total == half)	/* halfway point?       */
	newdotp = lp;		/* this is new dot      */
      lp = lforw (lp);		/* move forward         */
    }

  if (newdotp == NULL)		/* near end of buffer?  */
    curwp->w_dot.p = lback (lp);	/* move dot to eob      */
  else
    curwp->w_dot.p = newdotp;	/* move dot to middle   */

  curwp->w_dot.o = 0;		/* goto start of line   */
}

/*
 * Scroll forward by a specified number
 * of lines, or by a full page if no argument.
 * The "2" is the window overlap (this is the default
 * value from ITS EMACS). Because the top line in
 * the window is zapped, we have to do a hard
 * update and get it back.
 */
int
forwpage (int f, int n, int k)
{
  register LINE *lp;
  register int page;

  /* Compute how much to scroll to get to next page
   * (80% of the screen size is what EMACS seems to use).
   */
  page = curwp->w_ntrows - (curwp->w_ntrows / 5);
  if (page <= 0)
    page = 1;

  if (f == FALSE)
    n = page;			/* Default scroll.      */
  else if (n < 0)
    return (backpage (f, -n, KRANDOM));
#if	CVMVAS
  else				/* Convert from pages       */
    n *= page;			/* to lines.            */
#endif
  lp = curwp->w_linep;
  while (n-- && lp != lastline (curbp))
    lp = lforw (lp);
  curwp->w_linep = lp;		/* move the window ptr  */
  checkdot ();			/* see if dot must move */
  curwp->w_flag |= WFHARD;
  return (TRUE);
}

/*
 * This command is like "forwpage",
 * but it goes backwards. The "2", like above,
 * is the overlap between the two windows. The
 * value is from the ITS EMACS manual. The
 * hard update is done because the top line in
 * the window is zapped.
 */
int
backpage (int f, int n, int k)
{
  register LINE *lp;
  register int page;

  /* Compute how much to scroll to get to next page
   * (80% of the screen size is what EMACS seems to use).
   */
  page = curwp->w_ntrows - (curwp->w_ntrows / 5);
  if (page <= 0)
    page = 1;

  if (f == FALSE)
    n = page;			/* Default scroll.      */
  else if (n < 0)
    return (forwpage (f, -n, KRANDOM));
#if	CVMVAS
  else				/* Convert from pages       */
    n *= page;			/* to lines.            */
#endif
  lp = curwp->w_linep;
  while (n-- && lp != firstline (curbp))
    lp = lback (lp);
  curwp->w_linep = lp;
  checkdot ();
  curwp->w_flag |= WFHARD;
  return (TRUE);
}

/*
 * Set the mark in the current window
 * to the value of dot. A message is written to
 * the echo line unless we are running in a keyboard
 * macro, when it would be silly.
 */
int
setmark (int f, int n, int k)
{
  if (f == FALSE)
    {
      pushmark (curwp->w_dot);
      if (kbdmop == NULL)
	eprintf ("[Mark set]");
    }
  else
    {
      if (curwp->w_mark.p == NULL)
	{
	  eprintf ("No mark in this window");
	  return (FALSE);
	}
      curwp->w_dot = popmark ();
      curwp->w_flag |= WFMOVE;
    }
  return (TRUE);
}

/*
 * Swap the values of "dot" and "mark" in
 * the current window. This is pretty easy, because
 * all of the hard work gets done by the standard routine
 * that moves the mark about. The only possible
 * error is "no mark".
 */
int
swapmark (int f, int n, int k)
{
  POS tmp;

  if (curwp->w_mark.p == NULL)
    {
      eprintf ("No mark in this window");
      return (FALSE);
    }
  tmp = curwp->w_dot;
  curwp->w_dot = curwp->w_mark;
  curwp->w_mark = tmp;
  curwp->w_flag |= WFMOVE;
  return (TRUE);
}

/*
 * Go to a specific line, mostly for
 * looking up errors in C programs, which give the
 * error a line number. If an argument is present, then
 * it is the line number, else prompt for a line number
 * to use.
 */
int
gotoline (int f, int n, int k)
{
  register LINE *clp;
  register int s;
  char buf[32];

  if (f == FALSE)
    {
      if ((s = ereply ("Goto line: ", buf, sizeof (buf))) != TRUE)
	return (s);
      n = atoi (buf);
    }
  if (n <= 0)
    {
      eprintf ("Bad line");
      return (FALSE);
    }
  clp = firstline (curbp);	/* "clp" is first line  */
  while (n != 1 && clp != curbp->b_linep)
    {
      clp = lforw (clp);
      --n;
    }
  if (clp == curbp->b_linep)
    {
      eprintf ("Line number too large");
      return (FALSE);
    }
  curwp->w_dot.p = clp;
  curwp->w_dot.o = 0;
  curwp->w_flag |= WFMOVE;
  return (TRUE);
}
