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

#include <ncurses.h>
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

/*
 * Initialize the terminal.  Get the handles for console input and output.
 * Take a peek at the video buffer to see what video attributes are being used.
 */
void ttinit()
{
}

/*
 * The PC needs no tidy up.
 */
void tttidy()
{
}

/*
 * Move the cursor to the specified
 * origin 0 row and column position. Try to
 * optimize out extra moves; redisplay may
 * have left the cursor in the right
 * location last time!
 */
void ttmove(row, col)
{
  move(row, col);
  ttrow = row;
  ttcol = col;
}

/*
 * Erase to end of line.
 */
void tteeol()
{
  clrtoeol ();
}

/*
 * Erase to end of page.
 */
void tteeop()
{
  clrtobot ();
}

/*
 * Make a noise.
 */

void ttbeep()
{
    write(1,"\007",1);
}

/*
 * No-op.
 */
void ttwindow(top, bot)
{
}

/*
 * No-op.
 */
void ttnowindow()
{
}

/*
 * Set display color on IBM PC.  Just convert MicroEMACS
 * color to ncurses background attribute.
 */
void ttcolor(color)
int     color;
{
  tthue = color;
  bkgdset(' ' | (color == CMODE ? A_REVERSE : A_NORMAL));
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
}

/*
 * High speed screen update.  row and col are 1-based.
 */
void
putline(int row, int col, const char *buf)
{
  mvaddnstr(row - 1, col - 1, buf, ncol - col + 1);
}
