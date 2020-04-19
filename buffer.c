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

/* $Header: /home/bloovis/cvsroot/pe/buffer.c,v 1.2 2005-10-18 02:18:07 bloovis Exp $
 *
 * Name:	MicroEMACS
 *		Buffer handling.
 * Version:	30
 * By:		rex::conroy
 *		decvax!decwrl!dec-rhea!dec-rex!conroy
 * Modified by:	Mark Alexander
 *		drivax!alexande
 *
 * $Log: buffer.c,v $
 * Revision 1.2  2005-10-18 02:18:07  bloovis
 * Rename some things to avoid conflict with ncurses.
 *
 * Revision 1.1.1.1  2003/11/06 02:51:52  bloovis
 * Imported sources
 *
 * Revision 1.7  2002/06/01 01:36:36  malexander
 * (makelist): Temp buffer wasn't big enough for ASCII
 * representation of large line counts, causing crash with big files.
 *
 * Revision 1.6  2001/03/05 16:04:11  malexander
 * (usebuf): Use structure assignments for brevity.
 * (bcreate): Clear mark ring.
 * (addwind): Use structure assignments for brevity; copy mark ring
 * to/from window as appropriate.
 *
 * Revision 1.5  2001/02/28 21:07:40  malexander
 * * def.h (POS): New structure for holding line position, which replaces
 * dot and mark variable pairs everywhere.
 *
 * Revision 1.4  2000/11/01 22:00:48  malexander
 * (bcreate): Make buffer read-only if -r option was used.
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
 * Revision 1.2  1996/10/22 15:57:12  marka
 * Change itoa to intoa to avoid conflict with standard libraries.
 *
 * Revision 1.1  1995/11/09 03:20:41  marka
 * Initial revision
 *
 * Revision 1.4  91/04/19  23:13:17  alexande
 * Added bufsearch() routine, which provides support for autocompletion
 * of buffer names on echo line.
 * 
 * Revision 1.3  91/01/07  10:27:42  alexande
 * Remove C++ warnings.
 * 
 * Revision 1.2  89/08/24  10:05:08  alexande
 * Set left column of pop-up buffer list buffer to zero.
 *
 */
#include	"def.h"

char oldbufn[NBUFN];		/* name of old buffer   */

/*
 * Forward declarations.
 */

static int getbufn (char bufn[NBUFN]);
static void intoa (char buf[], int width, long num);
static int usebuf (BUFFER *bp);
static int makelist (void);

/*
 * This command attach a buffer to a window. The
 * values of dot and mark come from the buffer
 * if the use count is 0. Otherwise, they come
 * from some other window.
 */
int
usebuffer (int f, int n, int k)
{
  register BUFFER *bp;
  register int s;
  char bufn[NBUFN];

  if ((s = getbufn (bufn)) != TRUE)
    return (s);
  if ((bp = bfind (bufn, TRUE)) == NULL)
    return (FALSE);
  return (usebuf (bp));
}

/*
 * The command make the next
 * buffer in the buffer list
 * the current buffer. There are no real
 * errors, although the command does
 * nothing if there is only 1 buffer.
 */
int
nextbuffer (int f, int n, int k)
{
  register BUFFER *bp;

  if ((bp = curbp->b_bufp) == NULL)
    bp = bheadp;
  return (usebuf (bp));
}

/*
 * This command makes the previous
 * buffer in the buffer list the
 * current buffer. There arn't any errors,
 * although the command does not do a lot
 * if there is 1 buffer.
 */
int
prevbuffer (int f, int n, int k)
{
  register BUFFER *bp1;
  register BUFFER *bp2;

  bp1 = bheadp;
  bp2 = curbp;
  if (bp1 == bp2)
    bp2 = NULL;
  while (bp1->b_bufp != bp2)
    bp1 = bp1->b_bufp;
  return (usebuf (bp1));
}

/*
 * Attach a buffer to a window. The
 * values of dot and mark come from the buffer
 * if the use count is 0. Otherwise, they come
 * from some other window.  This routine
 * differs from usebuffer() in that it isn't a user command,
 * but expects a buffer pointer, instead of prompting the
 * user for a buffer name.
 */
static int
usebuf (BUFFER *bp)
{
  register EWINDOW *wp;

  addwind (curwp, -1);
  strcpy (oldbufn, curbp->b_bname);	/* save current name    */
  curbp = bp;			/* Switch.              */
  curwp->w_bufp = bp;
  curwp->w_linep = firstline (bp);	/* For macros, ignored. */
  curwp->w_flag |= WFMODE | WFFORCE | WFHARD;	/* Quite nasty.         */
  addwind (curwp, 1);
  if (bp->b_nwnd == 1)		/* First use.           */
    return (TRUE);
  ALLWIND (wp)
  {				/* Look for old.        */
    if (wp != curwp && wp->w_bufp == bp)
      {
	curwp->w_dot = wp->w_dot;
	curwp->w_mark = wp->w_mark;
	curwp->w_leftcol = wp->w_leftcol;
	break;
      }
  }
  return (TRUE);
}

/*
 * Get buffer name, use name of last buffer we switched from
 * if user doesn't supply a name.
 */
static int
getbufn (char bufn[NBUFN])
{
  register int s;

  if ((s = ereplyf ("Use buffer [%s]: ", bufn, NBUFN, EFNEW | EFCR | EFBUF,
		    oldbufn)) == ABORT)
    return (s);			/* CTRL-G abort         */
  if (s == FALSE || bufn[0] == 0)	/* no name specified?   */
    strcpy (bufn, oldbufn);	/* use old name         */
  return (bufn[0] != 0);	/* bad if null name     */
}


/*
 * Dispose of a buffer, by name.
 * Ask for the name. Look it up (don't get too
 * upset if it isn't there at all!). Get quite upset
 * if the buffer is being displayed. Clear the buffer (ask
 * if the buffer has been changed). Then free the header
 * line and the buffer header. Bound to "C-X K".
 */
int
killbuffer (int f, int n, int k)
{
  register BUFFER *bp;
  register BUFFER *bp1;
  register BUFFER *bp2;
  register int s;
  char bufn[NBUFN];

  if ((s = ereplyf ("Kill buffer: ", bufn, NBUFN, EFNEW | EFCR | EFBUF)) !=
      TRUE)
    return (s);
  if ((bp = bfind (bufn, FALSE)) == NULL)	/* Easy if unknown.     */
    {
      eprintf ("[Buffer not found]");
      return (TRUE);
    }
  if (bp->b_nwnd != 0)
    {				/* Error if on screen.  */
      eprintf ("Buffer is being displayed");
      return (FALSE);
    }
  if ((s = bclear (bp)) != TRUE)	/* Blow text away.      */
    return (s);
  free ((char *) bp->b_linep);	/* Release header line. */
  bp1 = NULL;			/* Find buffer header.  */
  bp2 = bheadp;
  while (bp2 != bp)
    {
      bp1 = bp2;
      bp2 = bp2->b_bufp;
    }
  bp2 = bp2->b_bufp;		/* Next one in chain.   */
  if (bp1 == NULL)		/* Unlink it.           */
    bheadp = bp2;
  else
    bp1->b_bufp = bp2;
  if (!strcmp (oldbufn, bp->b_bname))	/* same name as old?    */
    oldbufn[0] = 0;		/* clear old name       */
  killundo (bp);		/* Free undo records	*/
  removemode (bp);		/* Free mode record	*/
  free ((char *) bp);		/* Release buffer block */
  return (TRUE);
}

/*
 * Display the buffer list. This is done
 * in two parts. The "makelist" routine figures out
 * the text, and puts it in the buffer whoses header is
 * pointed to by the external "blistp". The "popblist"
 * then pops the data onto the screen. Bound to
 * "C-X C-B".
 */
int
listbuffers (int f, int n, int k)
{
  register int s;

  if ((s = makelist ()) != TRUE)
    return (s);
  return (popblist ());
}

/*
 * Pop the special buffer whose
 * buffer header is pointed to by the external
 * variable "blistp" onto the screen. This is used
 * by the "listbuffers" routine (above) and by
 * some other packages. Returns a status.
 */
int
popblist (void)
{
  register EWINDOW *wp;

  if (blistp->b_nwnd == 0)
    {				/* Not on screen yet.   */
      if ((wp = wpopup ()) == NULL)
	return (FALSE);
      addwind (wp, -1);		/* One less window      */
      wp->w_bufp = blistp;
      ++blistp->b_nwnd;
    }
  ALLWIND (wp)
  {				/* Update windows       */
    if (wp->w_bufp == blistp)
      {
	LINE *lp = firstline (blistp);
	wp->w_linep = lp;
	wp->w_dot.p = lp;
	wp->w_leftcol = 0;
	wp->w_dot.o = 0;
	wp->w_mark.p = NULL;
	wp->w_mark.o = 0;
	wp->w_flag |= WFMODE | WFHARD;
      }
  }
  return (TRUE);
}

/*
 * This routine rebuilds the
 * text in the special secret buffer
 * that holds the buffer list. It is called
 * by the list buffers command. Return TRUE
 * if everything works. Return FALSE if there
 * is an error (if there is no memory).
 */
static int
makelist (void)
{
  register char *cp1;
  register char *cp2;
  register int c;
  register BUFFER *bp;
  register LINE *lp;
  register long nbytes;
  register int s;
  char b[12 + 1];
  static char line[128];

  blistp->b_flag &= ~BFCHG;	/* Blow away old.       */
  if ((s = bclear (blistp)) != TRUE)
    return (s);
  strcpy (blistp->b_fname, "");
  if (addline    ("C         Size Buffer                           File") == FALSE
      || addline ("-         ---- ------                           ----") == FALSE)
    return (FALSE);
  ALLBUF (bp)
  {				/* For all buffers      */
    cp1 = &line[0];		/* Start at left edge   */
    if ((bp->b_flag & BFCHG) != 0)	/* "*" if changed       */
      *cp1++ = '*';
    else
      *cp1++ = ' ';
    *cp1++ = ' ';		/* Gap.                 */
    nbytes = 0;			/* Count bytes in buf.  */
    lp = firstline (bp);
    while (lp != bp->b_linep)
      {
	nbytes += llength (lp) + 1;
	lp = lforw (lp);
      }
    intoa (b, 12, nbytes);	/* 6 digit buffer size. */
    cp2 = &b[0];
    while ((c = *cp2++) != 0)
      *cp1++ = c;
    *cp1++ = ' ';		/* Gap.                 */
    cp2 = &bp->b_bname[0];	/* Buffer name          */
    while ((c = *cp2++) != 0)
      *cp1++ = c;
    cp2 = &bp->b_fname[0];	/* File name            */
    if (*cp2 != 0)
      {
	while (cp1 < &line[1 + 1 + 12 + 1 + NBUFN + 1])
	  *cp1++ = ' ';
	while ((c = *cp2++) != 0)
	  {
	    if (cp1 < &line[128 - 1])
	      *cp1++ = c;
	  }
      }
    *cp1 = 0;			/* Add to the buffer.   */
    if (addline (line) == FALSE)
      return (FALSE);
  }
  return TRUE;			/* All done             */
}

/*
 * Integer to ascii conversion, used only above.
 * The number must be a long value.  It used to be an int, which
 * wasn't always large enough on machines with 16-bit ints.
 */
static void
intoa (char buf[], int width, long num)
{
  buf[width] = 0;		/* End of string.       */
  while (num >= 10)
    {				/* Conditional digits.  */
      buf[--width] = (num % 10) + '0';
      num /= 10;
    }
  buf[--width] = num + '0';	/* Always 1 digit.      */
  while (width != 0)		/* Pad with blanks.     */
    buf[--width] = ' ';
}

/*
 * The argument "text" points to
 * a string. Append this line to the buffer list buffer.
 * Handcraft the EOL on the end. Return TRUE if it worked and
 * FALSE if you ran out of room.
 */
int
addline (const char *text)
{
  register LINE *lp, *endp;
  register int ntext;

  ntext = strlen (text);
  if ((lp = lalloc (ntext)) == NULL)
    return (FALSE);
  lputs (lp, text, ntext);
  endp = lastline (blistp);
  endp->l_bp->l_fp = lp;	/* Hook onto the end    */
  lp->l_bp = endp->l_bp;
  endp->l_bp = lp;
  lp->l_fp = endp;
  if (blistp->b_dot.p == endp)	/* If "." is at the end */
    blistp->b_dot.p = lp;	/* move it to new line  */
  return (TRUE);
}

/*
 * Look through the list of
 * buffers. Return TRUE if there
 * are any changed buffers. Special buffers
 * like the buffer list buffer don't count, as
 * they are not in the list. Return FALSE if
 * there are no changed buffers.
 */
int
anycb (void)
{
  register BUFFER *bp;

  ALLBUF (bp) if ((bp->b_flag & BFCHG) != 0)
    return (TRUE);
  return (FALSE);
}

/*
 * Search for a buffer, by name.
 * If not found, and the "cflag" is TRUE,
 * create a buffer and put it in the list of
 * all buffers. Return pointer to the BUFFER
 * block for the buffer.
 */
BUFFER *
bfind (const char *bname, int cflag)
{
  register BUFFER *bp;

  ALLBUF (bp) if (strcmp (bname, bp->b_bname) == 0)
    return (bp);
  if (cflag != FALSE && (bp = bcreate (bname)) != NULL)
    {
      bp->b_bufp = bheadp;
      bheadp = bp;
    }
  return (bp);
}

/*
 * Search for a buffer, given a partial name.
 * If prev is null, start searching at the beginning of the list.
 * Otherwise resume the search from the previous point.  Return the name
 * of the first buffer that matches the partial name, or NULL
 * if no match can be found.
 */
const char *
bufsearch (
     const char *bname,		/* partial buffer name to match         */
     int cpos,			/* number of characters in buffer name  */
     const char *prev)		/* NULL if starting from beginning      */
{
  static BUFFER *bp;
  char *name;

  if (prev == NULL)
    bp = bheadp;
  for (;;)
    {
      if (bp == NULL)
	return (NULL);
      name = bp->b_bname;
      bp = bp->b_bufp;
      if (strncmp (bname, name, cpos) == 0)
	return (name);
    }
}

/*
 * Add an empty line to the end of buffer.
 */
static void
addemptyline (BUFFER *bp)
{
  LINE *lp, *last;

  if ((lp = lallocx (0)) == NULL)
    {
      free ((char *) bp);
      return;
    }
  last = lastline(bp);
  lp->l_bp = last;
  lp->l_fp = last->l_fp;
  lp->l_fp->l_bp = lp;
  last->l_fp = lp;
}

/*
 * Create a buffer, by name.
 * Return a pointer to the BUFFER header
 * block, or NULL if the buffer cannot
 * be created. The BUFFER is not put in the
 * list of all buffers; this is called by
 * "edinit" to create the buffer list
 * buffer.
 */
BUFFER *
bcreate (const char *bname)
{
  register BUFFER *bp;
  register LINE *lp;

  if ((bp = (BUFFER *) malloc (sizeof (BUFFER))) == NULL)
    return (NULL);
  if ((lp = lallocx (0)) == NULL)
    {				/* header line          */
      free ((char *) bp);
      return (NULL);
    }
  bp->b_bufp = NULL;
  clearmarks (&bp->b_ring);
  bp->b_flag = rflag ? BFRO : 0;
  bp->b_nwnd = 0;
  bp->b_linep = lp;
  lp->l_fp = lp->l_bp = lp;	/* Header line  */
  addemptyline (bp);
  bp->b_dot.p = lforw (lp);
  bp->b_dot.o = 0;
  bp->b_leftcol = 0;
  strcpy (bp->b_fname, "");
  strcpy (bp->b_bname, bname);
  bp->b_undo = NULL;
  bp->b_mode = NULL;
  return (bp);
}

/*
 * This routine blows away all of the text
 * in a buffer. If the buffer is marked as changed
 * then we ask if it is ok to blow it away; this is
 * to save the user the grief of losing text. The
 * window chain is nearly always wrong if this gets
 * called; the caller must arrange for the updates
 * that are required. Return TRUE if everything
 * looks good.
 */
int
bclear (BUFFER *bp)
{
  register LINE *lp, *nextlp;
  register EWINDOW *wp;
  register int s;

  if ((bp->b_flag & BFCHG) != 0	/* Changed.             */
      && (s = eyesno ("Discard changes")) != TRUE)
    return (s);
  bp->b_flag &= ~BFCHG;		/* Not changed          */
  lp = firstline (bp);
  while (lp != bp->b_linep)
    {
      nextlp = lforw (lp);
      free ((char *) lp);
      lp = nextlp;
    }
  lp = bp->b_linep;		/* Header line          */
  lp->l_fp = lp->l_bp = lp;	/* Point it to itself   */
  addemptyline (bp);		/* Add an empty line	*/
  lp = firstline (bp);
  bp->b_dot.p = lp;		/* Make this the dot    */
  bp->b_dot.o = 0;
  bp->b_mark.p = NULL;		/* Invalidate "mark"    */
  bp->b_mark.o = 0;
  ALLWIND (wp)
  {				/* Update all windows   */
    if (wp->w_bufp == bp)	/*  viewing this buffer */
      {
	wp->w_linep = lp;
	wp->w_dot.p = lp;
	wp->w_dot.o = 0;
	wp->w_mark.p = NULL;
	wp->w_mark.o = 0;
      }
  }
  return (TRUE);
}


/*
 * Add or subtract 1 from the window count of the buffer associated
 * with a window.  Copy the dot and mark from the window to the buffer
 * if the count goes to zero, or from the buffer to the window if
 * the count goes to one.
 */

void
addwind (EWINDOW *wp, int n /* either +1 or -1 */)
{
  register BUFFER *bp;

  bp = wp->w_bufp;
  if (bp->b_nwnd == 0)
    {				/* First use.           */
      wp->w_dot = bp->b_dot;
      wp->w_mark = bp->b_mark;
      wp->w_ring = bp->b_ring;
      wp->w_leftcol = bp->b_leftcol;
    }
  bp->b_nwnd += n;
  if (bp->b_nwnd == 0)
    {				/* Last use.            */
      bp->b_dot = wp->w_dot;
      bp->b_mark = wp->w_mark;
      bp->b_ring = wp->w_ring;
      bp->b_leftcol = wp->w_leftcol;
    }
}
