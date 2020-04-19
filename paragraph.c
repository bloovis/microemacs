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

/* $Header: /home/bloovis/cvsroot/pe/paragraph.c,v 1.1 2003-11-06 02:51:52 bloovis Exp $
 *
 * Name:	MicroEMACS
 *		Paragraph and filling commands
 * Version:	29
 *
 * Code for dealing with paragraphs and filling. Adapted from MicroEMACS 3.6
 * and GNU-ified by mwm@ucbvax.  Several bug fixes by blarson@usc-oberon.
 *
 * $Log: paragraph.c,v $
 * Revision 1.1  2003-11-06 02:51:52  bloovis
 * Initial revision
 *
 * Revision 1.5  2001/03/05 16:04:13  malexander
 * (killpara): Use structure assignments for brevity.
 *
 * Revision 1.4  2001/02/28 21:07:40  malexander
 * * def.h (POS): New structure for holding line position, which replaces
 * dot and mark variable pairs everywhere.
 *
 * Revision 1.3  2000/09/29 00:19:38  malexander
 * Numerous changes to eliminate warnings and add prototypes.
 *
 * Revision 1.2  2000/07/21 16:20:32  malexander
 * Reformatted with GNU indent.
 *
 * Revision 1.1.1.1  2000/07/14 19:23:10  malexander
 * Imported sources
 *
 * Revision 1.3  91/01/07  10:25:00  alexande
 * Changes for variable tab size.
 * 
 * Revision 1.2  90/07/03  13:25:28  alexande
 * Eliminate Turbo C warning.
 * 
 *
 */
#include "def.h"

#define	MAXWORD	256

/*
 * Go back to the begining of the current paragraph.
 * We look for a <NL><NL> or <NL><TAB> or <NL><SPACE>
 * or <NL>@ or <NL>. (scribe and nroff commands)
 * combination to delimit the begining of a paragraph.
 */
int
gotobop (int f, int n, int k)
{
  register int c;		/* first character in current line */
  register LINE *prev;		/* previous line */

  if (n < 0)			/* the other way... */
    return (gotoeop (f, -n, KRANDOM));

  while (n-- > 0)
    {				/* for each one asked for */

      /* first scan back until we are in a word */
      while (backchar (FALSE, 1, KRANDOM))
	if (inword ())
	  break;
      curwp->w_dot.o = 0;	/* and go to the B-O-Line */

      /* and scan back until we hit a paragraph delimiter as
       * described above.
       */
      while ((prev = lback (curwp->w_dot.p)) != curbp->b_linep)
	{
	  if (llength (prev) == 0)
	    break;
	  c = lgetc (curwp->w_dot.p, 0);
	  if (c == ' ' || c == '\t')
	    break;
	  c = lgetc (prev, 0);
	  if (c == '@' || c == '.')
	    break;
	  else
	    curwp->w_dot.p = prev;
	}
    }
  curwp->w_flag |= WFMOVE;	/* force screen update */
  return TRUE;
}

/*
 * Go forward to the end of the current paragraph.
 * We look for a <NL><NL> or <NL><TAB> or <NL><SPACE>
 * or <NL>@ or <NL>. (scribe and nroff commands)
 * combination to delimit the begining of a paragraph.
 */
int
gotoeop (int f, int n, int k)
{
  register int c;		/* first character in current line */

  if (n < 0)			/* the other way... */
    return (gotobop (f, -n, KRANDOM));

  while (n-- > 0)
    {				/* for each one asked for */

      /* Find the first word on/after the current line */
      curwp->w_dot.o = 0;
      while (forwchar (FALSE, 1, KRANDOM))
	if (inword ())
	  break;
      curwp->w_dot.o = 0;
      curwp->w_dot.p = lforw (curwp->w_dot.p);

      /* Scan forward until we hit a paragraph delimiter as
       * described above.
       */
      while (curwp->w_dot.p != curbp->b_linep)
	{
	  if (llength (curwp->w_dot.p) == 0)
	    break;
	  c = lgetc (curwp->w_dot.p, 0);
	  if (c == ' ' || c == '\t' || c == '@' || c == '.')
	    break;
	  else
	    curwp->w_dot.p = lforw (curwp->w_dot.p);
	}
    }
  if (curwp->w_dot.p == curbp->b_linep)	/* at end of buffer?    */
    backchar (FALSE, 1, KRANDOM);	/* back up to prev line */
  curwp->w_flag |= WFMOVE;	/* force screen update */
  return (TRUE);
}

/*
 * Fill the current paragraph according to the current fill column.
 */
int
fillpara (int f, int n, int k)
{
  register int c;		/* current char durring scan    */
  register int wordlen;		/* length of current word       */
  register int clength;		/* position on line during fill */
  register int eopflag;		/* Are we at the End-Of-Paragraph? */
  int firstflag;		/* first word? (needs no space) */
  int newlength;		/* tentative new line length    */
  int eolflag;			/* was at end of line           */
  LINE *eopline;		/* pointer to line just past EOP */
  wchar_t wbuf[MAXWORD];	/* buffer for current word      */
  int i;

  /* Record the pointer to the line just past the
   * end of the paragraph.
   */
  gotoeop (FALSE, 1, KRANDOM);
  eopline = curwp->w_dot.p;
  if (eopline == lastline (curbp))
    eopline = curbp->b_linep;

  /* Move to the begining of the paragraph.
   */
  gotobop (FALSE, 1, KRANDOM);

  /* Skip to the start of the first word in the paragraph,
   * and set our current column position.
   */
  while (!inword ())
    if (forwchar (FALSE, 1, KRANDOM) == FALSE)
      break;
  clength = getcolpos () - 1;
  wordlen = 0;
  saveundo (UMOVE, &curwp->w_dot);

  /* scan through lines, filling words
   */
  firstflag = TRUE;
  eopflag = FALSE;
  while (!eopflag)
    {
      /* get the next character in the paragraph
       */
      if ((eolflag = (curwp->w_dot.o == wllength (curwp->w_dot.p))) == TRUE)
	{
	  c = ' ';
	  if (lforw (curwp->w_dot.p) == eopline)
	    eopflag = TRUE;
	}
      else
	c = wlgetc (curwp->w_dot.p, curwp->w_dot.o);

      /* and then delete it
       */
      if (ldelete (1, FALSE) == FALSE)
	return (FALSE);

      /* if not a separator, just add it in
       */
      if (c != ' ' && c != '\t')
	{
	  if (wordlen < MAXWORD - 1)
	    wbuf[wordlen++] = c;
	  else
	    {
	      /* You lose chars beyond MAXWORD if the word
	       * is to long. I'm to lazy to fix it now; it
	       * just silently truncated the word before, so
	       * I get to feel smug.
	       */
	      eprintf ("Word too long!");
	    }
	}
      else if (wordlen)
	{
	  /* calculate tentative new length with word added
	   */
	  newlength = clength + 1 + wordlen;

	  /* if at end of line or at doublespace and previous
	   * character was one of '.','?','!' doublespace here.
	   */
	  if ((eolflag || curwp->w_dot.o == wllength (curwp->w_dot.p)
	       || (c = wlgetc (curwp->w_dot.p, curwp->w_dot.o)) == ' '
	       || c == '\t')
	      && CISEOSP (wbuf[wordlen - 1]) && wordlen < MAXWORD - 1)
	    wbuf[wordlen++] = ' ';

	  /* at a word break with a word waiting
	   */
	  if (newlength <= fillcol)
	    {
	      /* add word to current line */
	      if (!firstflag)
		{
		  linsert (1, ' ', NULLPTR);
		  ++clength;
		}
	      firstflag = FALSE;
	    }
	  else
	    {
	      if (curwp->w_dot.o > 0 &&
		  wlgetc (curwp->w_dot.p, curwp->w_dot.o - 1) == ' ')
		{
		  curwp->w_dot.o -= 1;
		  saveundo (UMOVE, &curwp->w_dot);
		  ldelete (1, FALSE);
		}
	      /* start a new line */
	      lnewline ();
	      clength = 0;
	    }

	  /* and add the word in in either case */
	  for (i = 0; i < wordlen; i++)
	    linsert (1, wbuf[i], NULLPTR);
	  clength += wordlen;
	  wordlen = 0;
	}
    }
  /* and add a last newline for the end of our new paragraph */
  lnewline ();

  /* we realy should wind up where we started, (which is hard to keep
   * track of) but I think the end of the last line is better than the
   * begining of the blank line.
   */
  backchar (FALSE, 1, KRANDOM);
  return (TRUE);
}

/*
 * Delete n paragraphs starting with the current one.
 */
int
killpara (int f, int n, int k)
{
  register int status;		/* returned status of functions */

  while (n--)
    {				/* for each paragraph to delete */

      /* mark out the end and begining of the para to delete */
      gotoeop (FALSE, 1, KRANDOM);

      /* set the mark here */
      curwp->w_mark = curwp->w_dot;

      /* go to the begining of the paragraph */
      gotobop (FALSE, 1, KRANDOM);
      curwp->w_dot.o = 0;	/* force us to the begining of line */

      /* and delete it */
      if ((status = killregion (FALSE, 1, KRANDOM)) != TRUE)
	return (status);

      /* and clean up the 2 extra lines */
      ldelete (1, TRUE);
    }
  return (TRUE);
}

/*
 * Add a character, checking for word wrapping.
 * Check to see if we're past fillcol, and if so,
 * justify this line. As a last step, justify the line.
 */
int
fillword (int f, int n, int k)
{
  register char c;
  register int col, i, nce;

  for (i = col = 0; col <= fillcol; ++i, ++col)
    {
      if (i == curwp->w_dot.o)
	return selfinsert (f, n, k);
      c = lgetc (curwp->w_dot.p, i);
      if (c == '\t')
	col += (tabsize - col % tabsize) - 1;
      else if (CISCTRL (c) != FALSE)
	++col;
    }
  if (curwp->w_dot.o != wllength (curwp->w_dot.p))
    {
      selfinsert (f, n, k);
      nce = wllength (curwp->w_dot.p) - curwp->w_dot.o;
    }
  else
    nce = 0;
  curwp->w_dot.o = i;

  if ((c = lgetc (curwp->w_dot.p, curwp->w_dot.o)) != ' ' && c != '\t')
    do
      {
	backchar (FALSE, 1, KRANDOM);
      }
    while ((c = lgetc (curwp->w_dot.p, curwp->w_dot.o)) != ' '
	   && c != '\t' && curwp->w_dot.o > 0);

  if (curwp->w_dot.o == 0)
    do
      {
	forwchar (FALSE, 1, KRANDOM);
      }
    while ((c = lgetc (curwp->w_dot.p, curwp->w_dot.o)) != ' '
	   && c != '\t' && curwp->w_dot.o < wllength (curwp->w_dot.p));

  delwhite (FALSE, 1, KRANDOM);
  backdel (FALSE, 1, KRANDOM);
  lnewline ();
  curwp->w_dot.o = wllength (curwp->w_dot.p) - nce;
  curwp->w_flag |= WFMOVE;
  if (nce == 0 && curwp->w_dot.o != 0)
    return (fillword (f, n, k));
  return (TRUE);
}

/*
 * Set fill column to n.
 */
int
setfillcol (int f, int n, int k)
{
  fillcol = ((f == FALSE) ? getcolpos () : n);
  if (kbdmop == NULL)
    eprintf ("[Fill column set to %d]", fillcol);
  return (TRUE);
}
