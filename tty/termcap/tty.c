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
 * Termcap/terminfo display driver
 * Adapted from MicroGNU back to MicroEMACS V30 by Mark Alexander 9-27-88.
 *
 * Termcap is a terminal information database and routines to describe 
 * terminals on most UNIX systems.  Many other systems have adopted
 * this as a reasonable way to allow for widly varying and ever changing
 * varieties of terminal types.  This should be used where practical.
 */
/* Known problems:
 *	tputs is always called with the number of lines affected set to
 *	one.  Therefore, padding may be insufficient on some sequences
 *	dispite termcap being set up correctly.
 *
 *	If you have a terminal with no clear to end of screen and
 *	memory of lines below the ones visible on the screen, display
 *	will be wrong in some cases.  I doubt that any such terminal
 *	was ever made, but I thought everyone with delete line would
 *	have clear to end of screen too...
 *
 *	Code for terminals without clear to end of screen and/or clear
 *	to end of line has not been extensivly tested.
 *
 *	Cost calculations are very rough.  Costs of insert/delete line
 *	may be far from the truth.  This is accentuated by display.c
 *	not knowing about multi-line insert/delete.
 *
 *	Using scrolling region vs insert/delete line should probably
 *	be based on cost rather than the assuption that scrolling
 *	region operations look better.
 */
#include	"def.h"
#include	<termcap.h>

#define	BEL	0x07		/* BEL character.               */
#define	LF	0x0A		/* Line feed.                   */

extern int ttrow;
extern int ttcol;
extern int tttop;
extern int ttbot;
extern int tthue;

int tceeol;			/* Costs are set later */
int tcinsl[NROW + 1];
int tcdell[NROW + 1];
int xterm_mouse;		/* Are we doing mouse control on an xterm? */

static int insdel;		/* Do we have both insert & delete line? */

#define TCAPSLEN 1024

char tcapbuf[TCAPSLEN];

/* PC, UP, and BC are used by termlib, so must be extern and have these
 * names unless you have a non-standard termlib.
 */

int LI;				/* standard # lines */
char PC, *CM, *CE, *UP, *BC, *IM,	/* insert mode */
 *IC,				/* insert a single space */
 *EI,				/* end insert mode */
 *DC, *AL,			/* add line */
 *DL,				/* del line */
 *pAL,				/* parameterized add line */
 *pDL,				/* parameterized delete line */
 *TI,				/* term init -- start using cursor motion */
 *TE,				/* term end --- end using cursor motion */
 *SO, *SE, *CD, *CS,		/* set scroll region                    */
 *SR;				/* back index (used with scroll region  */
#ifdef	XKEYS
char *K[NFKEYS],		/* other function key codes             */
 *L[NFKEYS],			/* labels for other functions keys      */
 *KS, *KE,			/* enter keypad mode, exit keypad mode  */
 *KH, *KU, *KD, *KL, *KR;	/* home, arrow keys                    */
#endif
int SG;				/* number of glitches, 0 for invisable, -1 for none     */
	/* (yes virginia, there are terminals with invisible glitches)  */

/*
 * Initialize the terminal when the editor
 * gets started up.
 */
static char tcbuf[1024];

/*
 * Forward declarations.
 */
static int charcost (const char *s);


void
ttinit (void)
{
  char *t, *p;
  char *tv_stype;
  int cinsl;			/* cost of inserting a line     */
  int cdell;			/* cost of deleting a line      */
  int i;
#ifdef	XKEYS
  char kname[3], lname[3];
#endif

#ifdef	VAXC
  if ((tv_stype = trnlnm ("TERM")) == NULL)
#else
  if ((tv_stype = getenv ("TERM")) == NULL)	/* Don't want VAX C getenv() */
#endif
    panic ("Environment variable TERM not defined!");

  /* If -m flag (mouse reporting) is enabled and we're running
   * on an xterm, enable X10-compatible mouse button press reporting.
   */
  if (mouse && (strncmp (tv_stype, "xterm", 5) == 0))
    {
      xterm_mouse = TRUE;
      ttputs (L"\033[?9h", 5);
    }

  if ((tgetent (tcbuf, tv_stype)) != 1)
    {
      strcpy (tcbuf, "Unknown terminal type ");
      strcat (tcbuf, tv_stype);
      panic (tcbuf);
    }

  p = tcapbuf;
  t = tgetstr ("pc", &p);
  if (t)
    PC = *t;

  LI = tgetnum ("li");
  CD = tgetstr ("cd", &p);
  CM = tgetstr ("cm", &p);
  CE = tgetstr ("ce", &p);
  UP = tgetstr ("up", &p);
  BC = tgetstr ("bc", &p);
  IM = tgetstr ("im", &p);
  IC = tgetstr ("ic", &p);
  EI = tgetstr ("ei", &p);
  DC = tgetstr ("dc", &p);
  AL = tgetstr ("al", &p);
  DL = tgetstr ("dl", &p);
  pAL = tgetstr ("AL", &p);	/* parameterized insert and del. line */
  pDL = tgetstr ("DL", &p);
  TI = tgetstr ("ti", &p);
  TE = tgetstr ("te", &p);
  SO = tgetstr ("so", &p);
  SE = tgetstr ("se", &p);
  CS = tgetstr ("cs", &p);	/* set scrolling region */
  SR = tgetstr ("sr", &p);
  SG = tgetnum ("sg");		/* standout glitch      */
#ifdef	XKEYS
  /* get the 10 standard termcap keys */
  strcpy (kname, "kx");
  strcpy (lname, "lx");
  for (i = 0; i < 10; i++)
    {
      kname[1] = i + '1';
      K[i] = tgetstr (kname, &p);
      lname[1] = i + '1';
      L[i] = tgetstr (lname, &p);
    }
  /* Hack to get another bunch */
  strcpy (kname, "Kx");
  strcpy (lname, "Lx");
  for (i = 0; i < 10; i++)
    {
      kname[1] = i + '1';
      K[10 + i] = tgetstr (kname, &p);
      lname[1] = i + '1';
      L[10 + i] = tgetstr (lname, &p);
    }

  /* Get the rest of the sequences */
  KS = tgetstr ("ks", &p);
  KE = tgetstr ("ke", &p);
  KH = tgetstr ("kh", &p);
  KU = tgetstr ("ku", &p);
  KD = tgetstr ("kd", &p);
  KL = tgetstr ("kl", &p);
  KR = tgetstr ("kr", &p);
#endif

  if (CM == NULL || UP == NULL)
    panic ("This terminal is to stupid to run MicroGnuEmacs\n");
  ttresize ();			/* set nrow & ncol      */

  /* watch out for empty capabilities (sure to be wrong)  */
  if (CE && !*CE)
    CE = NULL;
  if (CS && !*CS)
    CS = NULL;
  if (SR && !*SR)
    SR = NULL;
  if (AL && !*AL)
    AL = NULL;
  if (DL && !*DL)
    DL = NULL;
  if (pAL && !*pAL)
    pAL = NULL;
  if (pDL && !*pDL)
    pDL = NULL;
  if (CD && !*CD)
    CD = NULL;

  if (!CE)
    tceeol = ncol;
  else
    tceeol = charcost (CE);

  /* Estimate cost of inserting a line */
  if (CS && SR)
    cinsl = charcost (CS) * 2 + charcost (SR);
  else if (pAL)
    cinsl = charcost (pAL);
  else if (AL)
    cinsl = charcost (AL);
  else
    cinsl = NROW * NCOL;	/* make this cost high enough */


  /* MicroEMACS requires an array of costs, not just a single value.
   * Fill the array with a single value.
   */
  for (i = 0; i <= NROW; i++)
    tcinsl[i] = 20 * cinsl * (i + 1);

  /* Estimate cost of deleting a line */
  if (CS)
    cdell = charcost (CS) * 2 + 1;
  else if (pDL)
    cdell = charcost (pDL);
  else if (DL)
    cdell = charcost (DL);
  else
    cdell = NROW * NCOL;	/* make this cost high enough */

  /* MicroEMACS requires an array of costs, not just a single value.
   * Fill the array with a single value.
   */
  for (i = 0; i <= NROW; i++)
    tcdell[i] = 20 * cdell * (i + 1);

  /* Flag to indicate that we can both insert and delete lines */
  insdel = (AL || pAL) && (DL || pDL);

  if (p >= &tcapbuf[TCAPSLEN])
    panic ("Terminal description too big!\n");
  if (TI && *TI)
    putpad (TI);		/* init the term */
}

/*
 * Clean up the terminal, in anticipation of
 * a return to the command interpreter. This is a no-op
 * on the ANSI display. On the SCALD display, it sets the
 * window back to half screen scrolling. Perhaps it should
 * query the display for the increment, and put it
 * back to what it was.
 */
void
tttidy (void)
{
  if (TE && *TE)
    putpad (TE);		/* set the term back to normal mode */
#ifdef	XKEYS
  ttykeymaptidy ();
#endif

  /* Disable xterm mouse event reporting.
   */
  if (xterm_mouse)
      ttputs (L"\033[?9l", 5);
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
  if (ttrow != row || ttcol != col)
    {
      putpad (tgoto (CM, col, row));
      ttrow = row;
      ttcol = col;
    }
}

/*
 * Erase to end of line.
 */
void
tteeol (void)
{
  if (CE)
    putpad (CE);
  else
    {
      register int i = ncol - ttcol;
      while (i--)
	ttputc (' ');
      ttrow = ttcol = HUGE;
    }
}

/*
 * Erase to end of page.
 */
void
tteeop (void)
{
  if (CD)
    putpad (CD);
  else
    {
      putpad (CE);
      if (insdel)
	ttdell (ttrow + 1, LI, LI - ttrow - 1);
      else
	{			/* do it by hand */
	  register int line;
	  for (line = ttrow + 1; line <= LI; ++line)
	    {
	      ttmove (line, 0);
	      tteeol ();
	    }
	}
      ttrow = ttcol = HUGE;
    }
}

/*
 * Make a noise.
 */
void
ttbeep (void)
{
/*	ttputc (BEL); */
  ttflush ();
}

/*
 * Insert nchunk blank line(s) onto the
 * screen, scrolling the last line on the
 * screen off the bottom.  Use the scrolling
 * region if possible for a smoother display.
 * If no scrolling region, use a set
 * of insert and delete line sequences
 */
void
ttinsl (int row, int bot, int nchunk)
{
  register int i;

  if (row == bot)
    {				/* Case of one line insert is   */
      ttmove (row, 0);		/*      special                 */
      tteeol ();
      return;
    }
  if (CS && SR)
    {				/* Use scroll region and back index     */
      ttwindow (row, bot);
      ttmove (row, 0);
      while (nchunk--)
	putpad (SR);
      ttnowindow ();
      return;
    }
  else if (insdel)
    {
      ttmove (1 + bot - nchunk, 0);
      if (pDL)
	putpad (tgoto (pDL, 0, nchunk));
      else
	for (i = 0; i < nchunk; i++)	/* For all lines in the chunk   */
	  putpad (DL);
      ttmove (row, 0);
      if (pAL)
	putpad (tgoto (pAL, 0, nchunk));
      else
	for (i = 0; i < nchunk; i++)	/* For all lines in the chunk   */
	  putpad (AL);
      ttrow = HUGE;
      ttcol = HUGE;
    }
  else
    panic ("ttinsl: Can't insert/delete line");
}

/*
 * Delete nchunk line(s) from "row", replacing the
 * bottom line on the screen with a blank line. 
 * Unless we're using the scrolling region, this is 
 * done with a crafty sequences of insert and delete 
 * lines.  The presence of the echo area makes a
 * boundry condition go away.
 */
void
ttdell (int row, int bot, int nchunk)
{
  register int i;

  if (row == bot)
    {				/* One line special case        */
      ttmove (row, 0);
      tteeol ();
      return;
    }
  if (CS)
    {				/* scrolling region     */
      ttwindow (row, bot);
      ttmove (bot, 0);
      while (nchunk--)
	ttputc (LF);
      ttnowindow ();
    }
  else if (insdel)
    {
      ttmove (row, 0);		/* Else use insert/delete line  */
      if (pDL)
	putpad (tgoto (pDL, 0, nchunk));
      else
	for (i = 0; i < nchunk; i++)	/* For all lines in the chunk   */
	  putpad (DL);
      ttmove (1 + bot - nchunk, 0);
      if (pAL)
	putpad (tgoto (pAL, 0, nchunk));
      else
	for (i = 0; i < nchunk; i++)	/* For all lines in the chunk   */
	  putpad (AL);
      ttrow = HUGE;
      ttcol = HUGE;
    }
  else
    panic ("ttdell: Can't insert/delete line");
}

/*
 * This routine sets the scrolling window
 * on the display to go from line "top" to line
 * "bot" (origin 0, inclusive). The caller checks
 * for the pathalogical 1 line scroll window that
 * doesn't work right, and avoids it. The "ttrow"
 * and "ttcol" variables are set to a crazy value
 * to ensure that the next call to "ttmove" does
 * not turn into a no-op (the window adjustment
 * moves the cursor).
 * 
 */
void
ttwindow (int top, int bot)
{
  if (CS && (tttop != top || ttbot != bot))
    {
      putpad (tgoto (CS, bot, top));
      ttrow = HUGE;		/* Unknown.             */
      ttcol = HUGE;
      tttop = top;		/* Remember region.     */
      ttbot = bot;
    }
}

/*
 * Switch to full screen scroll. This is
 * used by "spawn.c" just before is suspends the
 * editor, and by "display.c" when it is getting ready
 * to exit.  This function gets to full screen scroll 
 * by telling the terminal to set a scrolling regin
 * that is LI or nrow rows high, whichever is larger.
 * This behavior seems to work right on systems
 * where you can set your terminal size.
 */
void
ttnowindow (void)
{
  if (CS)
    {
      putpad (tgoto (CS, (nrow > LI ? nrow : LI) - 1, 0));
      ttrow = HUGE;		/* Unknown.             */
      ttcol = HUGE;
      tttop = HUGE;		/* No scroll region.    */
      ttbot = HUGE;
    }
}

/*
 * Set the current writing color to the
 * specified color. Watch for color changes that are
 * not going to do anything (the color is already right)
 * and don't send anything to the display.
 * The rainbow version does this in putline.s on a
 * line by line basis, so don't bother sending
 * out the color shift.
 */
void
ttcolor (int color)
{
  if (color != tthue)
    {
      if (color == CTEXT)
	{			/* Normal video.        */
	  putpad (SE);
	}
      else if (color == CMODE)
	{			/* Reverse video.       */
	  putpad (SO);
	}
      tthue = color;		/* Save the color.      */
    }
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
  /* ask OS for tty size  */
  if (nrow < 1)			/* Check limits.        */
    nrow = 1;
  else if (nrow > NROW)
    nrow = NROW;
  if (ncol < 1)
    ncol = 1;
  else if (ncol > NCOL)
    ncol = NCOL;
}

#ifdef NO_RESIZE
static void
setttysize (void)
{
  nrow = tgetnum ("li");
  ncol = tgetnum ("co");
}
#endif

/*
 * Insert character in the display.  Characters to the right
 * of the insertion point are moved one space to the right.
 */
int
ttinsertc (int c)
{
  putpad (IM);
  ttputc (c);
  putpad (EI);
  return c;
}

/*
 * Delete character in the display.  Characters to the right
 * of the deletion point are moved one space to the left.
 */
void
ttdelc (void)
{
  putpad (DC);
}

static int cci;

static int				/* fake char output for charcost() */
fakec (int c)
{
#ifdef	lint
  c++;
#endif
  cci++;
  return c;
}

/* calculate the cost of doing string s */
static int
charcost (const char *s)
{
  cci = 0;

  tputs (s, nrow, fakec);
  return cci;
}

void
putpad (const char *str)
{
  tputs (str, 1, ttputc);
}
