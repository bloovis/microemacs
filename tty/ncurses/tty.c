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
 * Name:        MicroEMACS
 *              Ncurses text-mode display
 * By:      	Mark Alexander
 *              marka@pobox.com
 *
 * $Log: tty.c,v $
 * Revision 1.2  2005-10-19 02:06:27  bloovis
 * (putline): Fix off-by-one bug.
 *
 * Revision 1.1  2005/10/18 02:18:44  bloovis
 * New files to implement ncurses screen handling.
 *
 *
 */

#include "def.h"

#ifdef __FreeBSD__
#include	<ncurses.h>
#else
#include <ncursesw/ncurses.h>
#endif
#include <unistd.h>

extern  int     tttop;
extern  int     ttbot;
extern  int     tthue;

/*
 * Local variables.
 */
#if     GOSLING
int     tceeol  =       2;              /* Costs.                       */
int     tcinsl  =       11;
int     tcdell  =       11;
#endif

int actual_nrow;
int actual_ncol;

/*
 * Draw vertical borders between pages when npages is greater than zero.
 */
static void
drawborders (void)
{
  int row, col;

  for (row = 0; row < actual_nrow; row++)
    {
      for (col = ncol; col < actual_ncol; col += ncol + 1)
	{
	  mvaddch (row, col, ACS_VLINE);
	}
    }
}

/*
 * Initialize the terminal.  Get the handles for console input and output.
 * Take a peek at the video buffer to see what video attributes are being used.
 *
 * The tricky code deals with splitting the window into side-by-side
 * pages (when npages is greater than one).  It tricks the rest of the
 * editor into thinking the screen is longer and narrower than it really is,
 * and leaves room for a vertical border between pages.
 */
void
ttinit (void)
{
  int new_nrow, new_ncol;

  actual_nrow = nrow;
  actual_ncol = ncol;

  if (npages < 1)
    {
      npages = 1;
    }
  
  new_nrow = nrow * npages;
  new_ncol = (ncol - npages + 1) / npages;

  if (new_nrow > NROW || new_ncol > NCOL)
    {
      npages = 1;
    }
  else
    {
      nrow = new_nrow;
      ncol = new_ncol;
      drawborders ();
    }
}

/*
 * Curses needs no tidy up.
 */
void
tttidy (void)
{
}

static void
get_actual_pos (int row, int col, int *actual_row, int *actual_col)
{
  if (npages > 1)
    {
      int page = row / actual_nrow;
      *actual_row = row % actual_nrow;
      *actual_col = (page * (ncol + 1)) + col;
    }
  else
    {
      *actual_row = row;
      *actual_col = col;
    }
}

/*
 * Move the cursor to the specified
 * origin 0 row and column position. Try to
 * optimize out extra moves; redisplay may
 * have left the cursor in the right
 * location last time!
 */
void
ttmove (int row, int col)
{
  int actual_row, actual_col;

  get_actual_pos (row, col, &actual_row, &actual_col);
  move (actual_row, actual_col);
  ttrow = row;
  ttcol = col;
}

/*
 * Erase to end of line.
 */
void
tteeol (void)
{
  if (npages > 1)
    {
      int actual_row, actual_col, i;

      get_actual_pos (ttrow, ttcol, &actual_row, &actual_col);
      for (i = ttcol; i < ncol; i++)
	addch (' ');
      move (actual_row, actual_col);
    }
  else
    clrtoeol ();
}

/*
 * Erase to end of page.
 */
void
tteeop (void)
{
  clrtobot ();
  drawborders ();
}

/*
 * Make a noise.
 */

void
ttbeep (void)
{
  if (write (1,"\007",1) != 1)
    {}	/* suppress warning about "ignoring return value of write" */
}

/*
 * No-op.
 */
void
ttwindow (int top, int bot)
{
}

/*
 * No-op.
 */
void
ttnowindow (void)
{
}

/*
 * Set display color.  Just convert MicroEMACS
 * color to ncurses background attribute.
 */
void
ttcolor (int color)
{
  tthue = color;
  bkgdset (' ' | (color == CMODE ? A_REVERSE : A_NORMAL));
}

/*
 * This routine is called by the
 * "refresh the screen" command to try and resize
 * the display. The new size, which must be deadstopped
 * to not exceed the NROW and NCOL limits, it stored
 * back into "nrow" and "ncol". Display can always deal
 * with a screen NROW by NCOL. Look in "window.c" to
 * see how the caller deals with a change.
 */
void
ttresize (void)
{
  setttysize ();		/* found in "ttyio.c",  */
  ttinit ();
  wrefresh (curscr);
}

/*
 * High speed screen update.  row and col are 1-based.
 */
void
putline (int row, int col, const wchar_t *buf)
{
  int actual_row, actual_col;

  get_actual_pos (row - 1, col - 1, &actual_row, &actual_col);
  move (actual_row, actual_col);
  ttputs (buf, ncol - col + 1);
}
