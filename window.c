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
 *		Window handling.
 * Version:	29
 * Last edit:	27-Oct-87
 * By:		rex::conroy
 *		decvax!decwrl!dec-rhea!dec-rex!conroy
 * Modified by:	Mark Alexander
 *		drivax!alexande
 */
#include	"def.h"

/*
 * Reposition dot in the current
 * window to line "n". If the argument is
 * positive, it is that line. If it is negative it
 * is that line from the bottom. If it is 0 the window
 * is centered (this is what the standard redisplay code
 * does). With no argument it defaults to 1.
 * Because of the default, it works like in
 * Gosling.
 */
int
reposition (int f, int n, int k)
{
  curwp->w_force = n;
  curwp->w_flag |= WFFORCE;
  return (TRUE);
}

/*
 * Refresh the display. A call is made to the
 * "ttresize" entry in the terminal handler, which tries
 * to reset "nrow" and "ncol". They will, however, never
 * be set outside of the NROW or NCOL range. If the display
 * changed size, arrange that everything is redone, then
 * call "update" to fix the display. We do this so the
 * new size can be displayed. In the normal case the
 * call to "update" in "main.c" refreshes the screen,
 * and all of the windows need not be recomputed.
 * Note that when you get to the "display unusable"
 * message, the screen will be messed up. If you make
 * the window bigger again, and send another command,
 * everything will get fixed!
 */
int
erefresh (int f, int n, int k)
{
  register EWINDOW *wp;
  register int oldnrow;
  register int oldncol;

  oldnrow = nrow;
  oldncol = ncol;
  if (f)
    npages = n;
  ttresize ();
  sgarbf = TRUE;		/* screen is garbage    */
  if (nrow != oldnrow || ncol != oldncol)
    {
      wp = wheadp;		/* Find last.           */
      while (wp->w_wndp != NULL)
	wp = wp->w_wndp;
      if (nrow < wp->w_toprow + 3)
	{			/* Check if too small.  */
	  eprintf ("Display unusable");
	  return (FALSE);
	}
      wp->w_ntrows = nrow - wp->w_toprow - 2;
      ALLWIND (wp)		/* Redraw all.          */
	wp->w_flag |= WFMODE | WFHARD;
      update ();
      eprintf ("[New size %d by %d]", nrow, ncol);
    }
  return (TRUE);
}

/*
 * The command make the next
 * window (next => down the screen)
 * the current window. There are no real
 * errors, although the command does
 * nothing if there is only 1 window on
 * the screen.
 */
int
nextwind (int f, int n, int k)
{
  register EWINDOW *wp;

  if ((wp = curwp->w_wndp) == NULL)
    wp = wheadp;
  curwp = wp;
  curbp = wp->w_bufp;
  return (TRUE);
}

/*
 * This command makes the previous
 * window (previous => up the screen) the
 * current window. There arn't any errors,
 * although the command does not do a lot
 * if there is 1 window.
 */
int
prevwind (int f, int n, int k)
{
  register EWINDOW *wp1;
  register EWINDOW *wp2;

  wp1 = wheadp;
  wp2 = curwp;
  if (wp1 == wp2)
    wp2 = NULL;
  while (wp1->w_wndp != wp2)
    wp1 = wp1->w_wndp;
  curwp = wp1;
  curbp = wp1->w_bufp;
  return (TRUE);
}

/*
 * This command moves the current
 * window down by "arg" lines. Recompute
 * the top line in the window. The move up and
 * move down code is almost completely the same;
 * most of the work has to do with reframing the
 * window, and picking a new dot. We share the
 * code by having "move down" just be an interface
 * to "move up".
 */
int
mvdnwind (int f, int n, int k)
{
  return (mvupwind (f, -n, KRANDOM));
}

/*
 * Move the current window up by "arg"
 * lines. Recompute the new top line of the window.
 * Look to see if "." is still on the screen. If it is,
 * you win. If it isn't, then move "." to center it
 * in the new framing of the window (this command does
 * not really move "."; it moves the frame).
 */
int
mvupwind (int f, int n, int k)
{
  register LINE *lp;
  register int i;

  lp = curwp->w_linep;
  if (n < 0)
    {
      while (n++ && lp != lastline (curbp))
	lp = lforw (lp);
    }
  else
    {
      while (n-- && lp != firstline (curbp))
	lp = lback (lp);
    }
  curwp->w_linep = lp;
  curwp->w_flag |= WFHARD;	/* Mode line is OK.     */
  for (i = 0; i < curwp->w_ntrows; ++i)
    {
      if (lp == curwp->w_dot.p)
	return (TRUE);
      if (lp == lastline (curbp))
	break;
      lp = lforw (lp);
    }
  lp = curwp->w_linep;
  i = curwp->w_ntrows / 2;
  while (i-- && lp != lastline (curbp))
    lp = lforw (lp);
  curwp->w_dot.p = lp;
  curwp->w_dot.o = 0;
  return (TRUE);
}

/*
 * This command makes the current
 * window the only window on the screen.
 * Try to set the framing
 * so that "." does not have to move on
 * the display. Some care has to be taken
 * to keep the values of dot and mark
 * in the buffer structures right if the
 * distruction of a window makes a buffer
 * become undisplayed.
 */
int
onlywind (int f, int n, int k)
{
  register EWINDOW *wp;
  register LINE *lp;
  register int i;

  while (wheadp != curwp)
    {
      wp = wheadp;
      wheadp = wp->w_wndp;
      addwind (wp, -1);
      free ((char *) wp);
    }
  while (curwp->w_wndp != NULL)
    {
      wp = curwp->w_wndp;
      curwp->w_wndp = wp->w_wndp;
      addwind (wp, -1);
      free ((char *) wp);
    }
  lp = curwp->w_linep;
  i = curwp->w_toprow;
  while (i != 0 && lp != firstline (curbp))
    {
      --i;
      lp = lback (lp);
    }
  curwp->w_toprow = 0;
  curwp->w_ntrows = nrow - 2;	/* 2 = mode, echo.      */
  curwp->w_linep = lp;
  curwp->w_flag |= WFMODE | WFHARD;
  return (TRUE);
}

/*
 * Split the current window. A window
 * smaller than 3 lines cannot be split.
 * The only other error that is possible is
 * a "malloc" failure allocating the structure
 * for the new window.
 */
int
splitwind (int f, int n, int k)
{
  register EWINDOW *wp;
  register LINE *lp;
  register int ntru;
  register int ntrl;
  register int ntrd;
  register EWINDOW *wp1;
  register EWINDOW *wp2;

  if (curwp->w_ntrows < 3)
    {
      eprintf ("Cannot split a %d line window", curwp->w_ntrows);
      return (FALSE);
    }
  if ((wp = (EWINDOW *) malloc (sizeof (EWINDOW))) == NULL)
    {
      eprintf ("Cannot allocate EWINDOW block");
      return (FALSE);
    }
  ++curbp->b_nwnd;		/* Displayed twice.     */
  wp->w_bufp = curbp;
  wp->w_dot = curwp->w_dot;
  wp->w_mark = curwp->w_mark;
  wp->w_ring = curwp->w_ring;
  wp->w_flag = 0;
  wp->w_force = 0;
  wp->w_leftcol = curwp->w_leftcol;
  ntru = (curwp->w_ntrows - 1) / 2;	/* Upper size           */
  ntrl = (curwp->w_ntrows - 1) - ntru;	/* Lower size           */
  lp = curwp->w_linep;
  ntrd = 0;
  while (lp != curwp->w_dot.p)
    {
      ++ntrd;
      lp = lforw (lp);
    }
  lp = curwp->w_linep;
  if (ntrd <= ntru)
    {				/* Old is upper window. */
      if (ntrd == ntru)		/* Hit mode line.       */
	lp = lforw (lp);
      curwp->w_ntrows = ntru;
      wp->w_wndp = curwp->w_wndp;
      curwp->w_wndp = wp;
      wp->w_toprow = curwp->w_toprow + ntru + 1;
      wp->w_ntrows = ntrl;
    }
  else
    {				/* Old is lower window  */
      wp1 = NULL;
      wp2 = wheadp;
      while (wp2 != curwp)
	{
	  wp1 = wp2;
	  wp2 = wp2->w_wndp;
	}
      if (wp1 == NULL)
	wheadp = wp;
      else
	wp1->w_wndp = wp;
      wp->w_wndp = curwp;
      wp->w_toprow = curwp->w_toprow;
      wp->w_ntrows = ntru;
      ++ntru;			/* Mode line.           */
      curwp->w_toprow += ntru;
      curwp->w_ntrows = ntrl;
      while (ntru--)
	lp = lforw (lp);
    }
  curwp->w_linep = lp;		/* Adjust the top lines */
  wp->w_linep = lp;		/* if necessary.        */
  curwp->w_flag |= WFMODE | WFHARD;
  wp->w_flag |= WFMODE | WFHARD;
  wp->w_savep = NULL;
  return (TRUE);
}

/*
 * Enlarge the current window.
 * Find the window that loses space. Make
 * sure it is big enough. If so, hack the window
 * descriptions, and ask redisplay to do all the
 * hard work. You don't just set "force reframe"
 * because dot would move.
 */
int
enlargewind (int f, int n, int k)
{
  register EWINDOW *adjwp;
  register LINE *lp;
  register int i;

  if (n < 0)
    return (shrinkwind (f, -n, KRANDOM));
  if (wheadp->w_wndp == NULL)
    {
      eprintf ("Only one window");
      return (FALSE);
    }
  if ((adjwp = curwp->w_wndp) == NULL)
    {
      adjwp = wheadp;
      while (adjwp->w_wndp != curwp)
	adjwp = adjwp->w_wndp;
    }
  if (adjwp->w_ntrows <= n)
    {
      eprintf ("Impossible change");
      return (FALSE);
    }
  if (curwp->w_wndp == adjwp)
    {				/* Shrink below.        */
      lp = adjwp->w_linep;
      for (i = 0; i < n && lp != lastline (adjwp->w_bufp); ++i)
	lp = lforw (lp);
      adjwp->w_linep = lp;
      adjwp->w_toprow += n;
    }
  else
    {				/* Shrink above.        */
      lp = curwp->w_linep;
      for (i = 0; i < n && lp != firstline (curbp); ++i)
	lp = lback (lp);
      curwp->w_linep = lp;
      curwp->w_toprow -= n;
    }
  curwp->w_ntrows += n;
  adjwp->w_ntrows -= n;
  curwp->w_flag |= WFMODE | WFHARD;
  adjwp->w_flag |= WFMODE | WFHARD;
  return (TRUE);
}

/*
 * Shrink the current window.
 * Find the window that gains space. Hack at
 * the window descriptions. Ask the redisplay to
 * do all the hard work.
 */
int
shrinkwind (int f, int n, int k)
{
  register EWINDOW *adjwp;
  register LINE *lp;
  register int i;

  if (n < 0)
    return (enlargewind (f, -n, KRANDOM));
  if (wheadp->w_wndp == NULL)
    {
      eprintf ("Only one window");
      return (FALSE);
    }
  if ((adjwp = curwp->w_wndp) == NULL)
    {
      adjwp = wheadp;
      while (adjwp->w_wndp != curwp)
	adjwp = adjwp->w_wndp;
    }
  if (curwp->w_ntrows <= n)
    {
      eprintf ("Impossible change");
      return (FALSE);
    }
  if (curwp->w_wndp == adjwp)
    {				/* Grow below.          */
      lp = adjwp->w_linep;
      for (i = 0; i < n && lp != firstline (adjwp->w_bufp); ++i)
	lp = lback (lp);
      adjwp->w_linep = lp;
      adjwp->w_toprow -= n;
    }
  else
    {				/* Grow above.          */
      lp = curwp->w_linep;
      for (i = 0; i < n && lp != lastline (curbp); ++i)
	lp = lforw (lp);
      curwp->w_linep = lp;
      curwp->w_toprow += n;
    }
  curwp->w_ntrows -= n;
  adjwp->w_ntrows += n;
  curwp->w_flag |= WFMODE | WFHARD;
  adjwp->w_flag |= WFMODE | WFHARD;
  return (TRUE);
}

/*
 * Adjust windows so that they all have approximately
 * the same height.
 */
int
balancewindows (int f, int n, int k)
{
  register EWINDOW *wp;
  register LINE *lp;
  register int toprow, size, nwind, i;

  if (wheadp->w_wndp == NULL)
    {
      eprintf ("Only one window");
      return (FALSE);
    }
  nwind = 0;
  ALLWIND(wp)
    {
      nwind++;
    }
  toprow = 0;
  size = ((nrow - 1) / nwind) - 1;
  ALLWIND(wp)
    {
      if (wp->w_wndp == NULL)
	size = nrow - toprow - 2;
      if (size < wp->w_ntrows)
	{
	  /* Shrink this window. */
	  n = wp->w_ntrows - size;
	  lp = wp->w_linep;
	  for (i = 0; i < n && lp != lastline (wp->w_bufp); ++i)
	    lp = lforw (lp);
	  wp->w_linep = lp;
	}
      wp->w_toprow = toprow;
      wp->w_ntrows = size;
      wp->w_flag |= WFMODE | WFHARD;
      toprow += size + 1;
    }
  return (TRUE);
}

/*
 * Pick a window for a pop-up.
 * Split the screen if there is only
 * one window. Pick the uppermost window that
 * isn't the current window. An LRU algorithm
 * might be better. Return a pointer, or
 * NULL on error.
 */
EWINDOW *
wpopup (void)
{
  register EWINDOW *wp;

  if (wheadp->w_wndp == NULL && splitwind (FALSE, 0, KRANDOM) == FALSE)
    return (NULL);
  wp = wheadp;			/* Find window to use   */
  while (wp != NULL && wp == curwp)
    wp = wp->w_wndp;
  return (wp);
}
