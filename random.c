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

/* $Header: /home/bloovis/cvsroot/pe/random.c,v 1.3 2005-10-18 02:17:58 bloovis Exp $
 * Name:	MicroEMACS
 *		Assorted commands.
 * Version:	29
 * By:		rex::conroy
 *		decvax!decwrl!dec-rhea!dec-rex!conroy
 *
 * The file contains the command
 * processors for a large assortment of unrelated
 * commands. The only thing they have in common is
 * that they are all command processors.
 *
 * $Log: random.c,v $
 * Revision 1.3  2005-10-18 02:17:58  bloovis
 * Rename some things to avoid conflict with ncurses.
 *
 * Revision 1.2  2003/12/03 22:14:34  bloovis
 * (vmwareindent): New function to do VMware-style indenting.
 *
 * Revision 1.1.1.1  2003/11/06 02:51:52  bloovis
 * Imported sources
 *
 * Revision 1.6  2002/01/23 22:36:08  malexander
 * (checkheap): Don't call heapcheck with bcc on Linux.
 *
 * Revision 1.5  2001/02/28 21:07:40  malexander
 * * def.h (POS): New structure for holding line position, which replaces
 * dot and mark variable pairs everywhere.
 *
 * Revision 1.4  2000/09/29 00:19:38  malexander
 * Numerous changes to eliminate warnings and add prototypes.
 *
 * Revision 1.3  2000/07/25 20:02:26  malexander
 * (borlandindent): New command to implement Borland-style
 * indentation.
 *
 * Revision 1.2  2000/07/21 16:20:32  malexander
 * Reformatted with GNU indent.
 *
 * Revision 1.1.1.1  2000/07/14 19:23:10  malexander
 * Imported sources
 *
 * Revision 1.1  1995/10/18  23:04:44  marka
 * Initial revision
 *
 * Revision 1.5  91/04/19  23:24:09  alexande
 * Added support for overstrike mode.
 * 
 * Revision 1.4  91/02/08  11:30:14  alexande
 * Showcpos() printed garbage line number if executed in empty buffer.
 * 
 * Revision 1.3  91/01/07  10:24:44  alexande
 * Changes for variable tab size.
 * 
 * Revision 1.2  90/07/03  13:21:38  alexande
 * Fixed new bug in [C-X =] where cursor was always moved to end of line.
 * 
 *
 */
#include	"def.h"

int overstrike = 0;		/* TRUE if in overstrike mode */

/*
 * Display a bunch of useful information about
 * the current location of dot. The character under the
 * cursor (in octal), the current line, row, and column, and
 * approximate position of the cursor in the file (as a percentage)
 * is displayed. The column position assumes an infinite position
 * display; it does not truncate just because the screen does.
 * This is normally bound to "C-X =".
 */
int
showcpos (int f, int n, int k)
{
  register LINE *clp;
  register LINE *dotp;
  register int doto;
  register long nchar;
  register long cchar;
  register int nline;
  register int cline;
  register int cbyte;
  register int ratio;
  register int row;

  clp = firstline (curbp);	/* Collect the data.    */
  dotp = curwp->w_dot.p;
  doto = curwp->w_dot.o;
  cchar = nchar = 0;
  nline = 0;
  cline = 1;
  cbyte = '\n';
  for (;;)
    {
      if (clp == curbp->b_linep)
	break;
      ++nline;			/* Count a line.        */
      if (clp == dotp)
	{
	  cline = nline;
	  cchar = nchar + doto;
	  if (doto == wllength (clp))
	    cbyte = '\n';
	  else
	    cbyte = wlgetc (clp, doto);
	}
      nchar += wllength (clp) + 1;	/* Count characters     */
#ifdef __TURBOC__
      nchar++;			/* Count carriage return */
#endif
      clp = lforw (clp);
    }

  /* Don't count the newline at the end of the last line, since
   * it serves only as the terminator for the actual last line.
   */
  if ((clp = lback (curbp->b_linep)) != curbp->b_linep)
#ifdef __TURBOC__
    nchar -= 2;
#else
    nchar--;
#endif

  row = curwp->w_toprow;	/* Determine row.       */
  clp = curwp->w_linep;
  while (clp != curbp->b_linep && clp != dotp)
    {
      ++row;
      clp = lforw (clp);
    }
  ++row;			/* Convert to origin 1. */
  ratio = 0;			/* Ratio before dot.    */
  if (nchar != 0)
    {
      ratio = (100L * cchar) / nchar;
      if (ratio == 0 && cchar != 0)	/* Allow 0% only at the */
	ratio = 1;		/* start of the file.   */
    }
  eprintf ("[CH:0x%x Line:%d Row:%d Col:%d %d%% of %l]",
	   cbyte, cline, row, getcolpos (), ratio, nchar);
  return (TRUE);
}

/*
 * Return the current column position of dot, taking into account tabs
 * and control characters.  The colum returned is normalized to
 * column one origin.
 */
int
getcolpos (void)
{
  register int col, i, c;

  col = 0;			/* Determine column.    */
  for (i = 0; i < curwp->w_dot.o; ++i)
    {
      c = wlgetc (curwp->w_dot.p, i);
      if (c == '\t')
	col += (tabsize - col % tabsize) - 1;
      else if (c < 0x80 && CISCTRL (c) != FALSE)
	++col;
      ++col;
    }
  return (col + 1);		/* Convert to origin 1. */
}

/*
 * Twiddle the two characters on either side of
 * dot. If dot is at the end of the line twiddle the
 * two characters before it. Return with an error if dot
 * is at the beginning of line; it seems to be a bit
 * pointless to make this work. This fixes up a very
 * common typo with a single stroke. Normally bound
 * to "C-T". This always works within a line, so
 * "WFEDIT" is good enough.
 */
int
twiddle (int f, int n, int k)
{
  register LINE *dotp;
  register int doto;
  register int cl;
  register int cr;
  POS dot;

  if (checkreadonly () == FALSE)
    return FALSE;
  dotp = curwp->w_dot.p;
  doto = curwp->w_dot.o;
  if (doto == wllength (dotp) && --doto < 0)
    return (FALSE);
  cr = lgetc (dotp, doto);
  if (--doto < 0)
    return (FALSE);
  cl = lgetc (dotp, doto);
  dot.p = dotp;
  dot.o = doto;
  lputc (dot, cr);
  ++dot.o;
  lputc (dot, cl);
  lchange (WFEDIT);
  return (TRUE);
}

/*
 * Quote the next character, and
 * insert it into the buffer. All the characters
 * are taken literally, with the exception of the newline,
 * which always has its line splitting meaning. The character
 * is always read, even if it is inserted 0 times, for
 * regularity.
 */
int
quote (int f, int n, int k)
{
  register int s;
  register int c;

  if (kbdmop != NULL)
    c = *kbdmop++;
  else
    {
      c = getinp ();
      if (kbdmip != NULL)
	{
	  if (kbdmip > &kbdm[NKBDM - 4])
	    {
	      ctrlg (FALSE, 0, KRANDOM);
	      return (ABORT);
	    }
	  *kbdmip++ = c;
	}
    }
  if (n < 0)
    return (FALSE);
  if (n == 0)
    return (TRUE);
  if (c == '\n')
    {
      do
	{
	  s = lnewline ();
	}
      while (s == TRUE && --n);
      return (s);
    }
  if (overstrike && curwp->w_dot.o != wllength (curwp->w_dot.p))
    ldelete (1, FALSE);
  return (linsert (n, c, NULLPTR));
}

/*
 * Ordinary text characters are bound to this function,
 * which inserts them into the buffer. Characters marked as control
 * characters (using the CTRL flag) may be remapped to their ASCII
 * equivalent. This makes TAB (C-I) work right, and also makes the
 * world look reasonable if a control character is bound to this
 * this routine by hand. Any META or CTLX flags on the character
 * are discarded. This is the only routine that actually looks
 * at the "k" argument.
 */
int
selfinsert (int f, int n, int k)
{
  register int c;

  if (n < 0)
    return (FALSE);
  if (n == 0)
    return (TRUE);
  c = k & KCHAR;
  if ((k & KCTRL) != 0 && c >= '@' && c <= '_')	/* ASCII-ify.           */
    c -= '@';
  if (overstrike && curwp->w_dot.o != wllength (curwp->w_dot.p))
    ldelete (1, FALSE);
  return (linsert (n, c, NULLPTR));
}

/*
 * Open up some blank space. The basic plan
 * is to insert a bunch of newlines, and then back
 * up over them. Everything is done by the subcommand
 * processors. They even handle the looping. Normally
 * this is bound to "C-O".
 */
int
openline (int f, int n, int k)
{
  register int i;
  register int s;

  if (n < 0)
    return (FALSE);
  if (n == 0)
    return (TRUE);
  i = n;			/* Insert newlines.     */
  do
    {
      s = lnewline ();
    }
  while (s == TRUE && --i);
  if (s == TRUE)		/* Then back up overtop */
    s = backchar (f, n, KRANDOM);	/* of them all.         */
  return (s);
}

/*
 * Insert a newline.
 * If you are at the end of the line and the
 * next line is a blank line, just move into the
 * blank line. This makes "C-O" and "C-X C-O" work
 * nicely, and reduces the ammount of screen
 * update that has to be done. This would not be
 * as critical if screen update were a lot
 * more efficient.  The NLMOVE definition in
 * "def.h" enables this feature.  Now that the
 * fast Gosling display code is in place, I
 * don't see any reason to enable NLMOVE.
 */
int
newline (int f, int n, int k)
{
  register int s;

  if (n < 0)
    return (FALSE);
  while (n--)
    {
#if	NLMOVE
      register LINE *lp;
      lp = curwp->w_dot.p;
      if (wllength (lp) == curwp->w_dot.o
	  && lp != curbp->b_linep
	  && lp != lastline (curbp) && wllength (lforw (lp)) == 0)
	{
	  if ((s = forwchar (FALSE, 1, KRANDOM)) != TRUE)
	    return (s);
	}
      else
#endif
      if ((s = lnewline ()) != TRUE)
	return (s);
    }
  return (TRUE);
}

/*
 * Delete blank lines around dot.
 * What this command does depends if dot is
 * sitting on a blank line. If dot is sitting on a
 * blank line, this command deletes all the blank lines
 * above and below the current line. If it is sitting
 * on a non blank line then it deletes all of the
 * blank lines after the line. Normally this command
 * is bound to "C-X C-O". Any argument is ignored.
 */
int
deblank (int f, int n, int k)
{
  register LINE *lp1;
  register LINE *lp2;
  register int nld;

  lp1 = curwp->w_dot.p;
  while (llength (lp1) == 0 && (lp2 = lback (lp1)) != curbp->b_linep)
    lp1 = lp2;
  lp2 = lp1;
  nld = 0;
  while ((lp2 = lforw (lp2)) != curbp->b_linep && llength (lp2) == 0)
    ++nld;
  if (nld == 0)
    return (TRUE);
  curwp->w_dot.p = lforw (lp1);
  curwp->w_dot.o = 0;
  saveundo (UMOVE, &curwp->w_dot);
  return (ldelete (nld, FALSE));
}

/*
 * Delete any whitespace around dot, then insert a space.
 */
int /*ARGSUSED*/
delwhite (int f, int n, int k)
{
  register int col, c, len;

  /* Scan forward to find end of whitespace.  If cursor
   * isn't already in some whitespace, just insert a space.
   */
  len = wllength (curwp->w_dot.p);
  for (col = curwp->w_dot.o; col < len; col++)
    if ((c = lgetc (curwp->w_dot.p, col)) != ' ' && c != '\t')
      break;
  if (col == curwp->w_dot.o)	/* no whitespace seen? */
    return linsert (1, ' ', NULLPTR);

  /* Move dot back to start of whitespace.
   */
  while (curwp->w_dot.o != 0)
    {
      if ((c = lgetc (curwp->w_dot.p, curwp->w_dot.o - 1)) != ' ' && c != '\t')
	break;
      if (backchar (FALSE, 1, KRANDOM) == FALSE)
	break;
    }

  /* Delete all the whitespace found, then insert a single space.
   */
  saveundo (UMOVE, &curwp->w_dot);
  ldelete ((col - curwp->w_dot.o), FALSE);
  return linsert (1, ' ', NULLPTR);
}

/*
 * Return TRUE if the string appears in the current line at offset i.
 */

static int
testline (int i, char *string)
{
  int stlen = strlen (string);

  if (i + stlen > llength (curwp->w_dot.p))
    return (FALSE);
  return (strncmp ((const char *) lgets (curwp->w_dot.p) + i, string, stlen) == 0);
}

/*
 * Return indentation of the the current line, and store the offset
 * of the first non-white character at *offset.
 */
static int
currentindent (int *offset)
{
  int nicol = 0;
  int len = wllength (curwp->w_dot.p);
  int i, c;

  for (i = 0; i < len; ++i)
    {
      c = lgetc (curwp->w_dot.p, i);
      if (c != ' ' && c != '\t')
	break;
      if (c == '\t')
	nicol += (tabsize - nicol % tabsize) - 1;
      ++nicol;
    }
  *offset = i;
  return nicol;
}

/*
 * Insert a newline followed by the correct number of
 * tabs and spaces to get the desired indentation. If nonwhitepos
 * (the offset of the first nonwhitepos in the current line) is
 * the end of the line, the line is completely white, so zero it out;
 * then if an argument was specified to the command, instead of
 * inserting a new line, just readjust the newly blanked line's indentation.
 */
static int
nlindent (int nicol, int nonwhitepos, int f)
{
  int i;
  int linelen = wllength (curwp->w_dot.p);

  /* Zero out the current line if it's all whitespace;
   * otherwise insert a new line.
   */
  if (linelen != 0 && nonwhitepos == linelen)
    {
      if (gotobol (FALSE, 1, KRANDOM) == FALSE)
	return FALSE;
      saveundo (UMOVE, &curwp->w_dot);
      if (ldelete (linelen, FALSE) == FALSE)
	return FALSE;
      if (f == FALSE && lnewline () == FALSE)
	return FALSE;
    }
  else
    {
      if (lnewline () == FALSE)
	return FALSE;
    }

  /* Adjust the indentation of the current line.
   */
  if (((i = nicol / tabsize) != 0 && linsert (i, '\t', NULLPTR) == FALSE)
      || ((i = nicol % tabsize) != 0 && linsert (i, ' ', NULLPTR) == FALSE))
    return (FALSE);
  return (TRUE);
}

/*
 * Insert a newline, then enough
 * tabs and spaces to duplicate the indentation
 * of the previous line.  Use the current tab size to determine
 * the indentation. Quite simple. Figure out the indentation
 * of the current line. Insert a newline by calling
 * the standard routine. Insert the indentation by
 * inserting the right number of tabs and spaces.
 * Return TRUE if all ok. Return FALSE if one
 * of the subcomands failed. Normally bound
 * to "C-J".
 */
int
indent (int f, int n, int k)
{
  int nicol, i;

  /* Find indentation of current line.
   */
  nicol = currentindent (&i);

  /* Insert a newline followed by the correct number of
   * tabs and spaces to get the desired indentation.
   * Pass 0 as the second parameter so that nlindent
   * won't truncate the current line if it is all whitespace.
   */
  return nlindent (nicol, 0, f);
}

/*
 * Similar to indent, but do it according to GNU coding standards.
 * Insert a newline, then enough tabs and spaces to match the indentation
 * of the previous line.  If the previous line starts with "if" or "{",
 * indent by two spaces. If the previous line starts with "}", reduce
 * indentation by two spaces.  Otherwise retain the same indentation.
 */
int
gnuindent (int f, int n, int k)
{
  int nicol, i;

  /* Find indentation of current line.
   */
  nicol = currentindent (&i);

  /* Look at the string following the whitespace in the
   * current line to determine the indentation of the next line.
   * If any of certain magic tokens was found, or a single C-U argument
   * was specified, indent by two spaces.  If a two-C-U argument was
   * specified, unindent by two spaces.
   */
  if (testline (i, "{") || testline (i, "if")
      || testline (i, "while") || testline (i, "for")
      || testline (i, "else") || testline (i, "case")
      || (f && (n != 16)))
    nicol += 2;
  else if (testline (i, "}") || (f && (n == 16)))
    nicol = nicol < 2 ? 0 : nicol - 2;

  /* Insert a newline followed by the correct number of
     tabs and spaces to get the desired indentation.
   */
  return nlindent (nicol, i, f);
}

/*
 * Similar to indent, but do it according to Ruby conventions.
 * Insert a newline, then enough tabs and spaces to match the indentation
 * of the previous line.  If the previous line starts with a block-start
 * keyword, indent by two spaces. If a two-C-U argument was specified,
 * reduce indentation by two spaces.  Otherwise retain the same indentation.
 */
int
rubyindent (int f, int n, int k)
{
  int nicol, i;
  int len = wllength (curwp->w_dot.p);

  /* Find indentation of current line.
   */
  nicol = currentindent (&i);

  /* Look at the string following the whitespace in the
   * current line to determine the indentation of the next line.
   * If any of certain magic tokens was found, or a single C-U argument
   * was specified, indent by two spaces.  If a two-C-U argument was
   * specified, unindent by two spaces.
   */
  if ((len > 0 && testline (len - 1, "{"))
      || testline (i, "def") || testline (i, "if")
      || testline (i, "when") || testline (i, "for")
      || testline (i, "else") || testline (i, "class")
      || testline (i, "module") || (f && (n != 16)))
    nicol += 2;
  else if (f && (n == 16))
    nicol = nicol < 2 ? 0 : nicol - 2;

  /* Insert a newline followed by the correct number of
     tabs and spaces to get the desired indentation.
   */
  return nlindent (nicol, i, f);
}

/*
 * Similar to indent, but do it according to Borland coding standards.
 * Insert a newline, then enough tabs and spaces to match the indentation
 * of the previous line.  If the previous line starts with "{", or an argument
 * of four (i.e. a single C-U) is specified, indent by four spaces. If an
 * argument of 16 (i.e. two C-Us) is specified, reduce indentation by four
 * spaces.  Otherwise retain the same indentation.
 */
int
borlandindent (int f, int n, int k)
{
  int nicol, i;

  /* Find indentation of current line.
   */
  nicol = currentindent (&i);

  /* Look at the string following the whitespace in the
   * current line to determine the indentation of the next line.
   * If a single C-U argument was specified, or the previous line started
   * with a curly brace, indent by four spaces.
   * If two C-U arguments were specified, unindent by four spaces.
   */
  if (testline (i, "{") || (f && (n != 16)))
    nicol += 4;
  else if (f && (n == 16))
    nicol = nicol < 4 ? 0 : nicol - 4;

  /* Insert a newline followed by the correct number of
     tabs and spaces to get the desired indentation.
   */
  return nlindent (nicol, i, f);
}

/*
 * Similar to indent, but do it according to VMware coding standards.
 * Insert a newline, then enough tabs and spaces to match the indentation
 * of the previous line.  If the previous line ends with "{", or an argument
 * of four (i.e. a single C-U) is specified, indent by three spaces. If an
 * argument of 16 (i.e. two C-Us) is specified, reduce indentation by three
 * spaces.  Otherwise retain the same indentation.
 */
int
vmwareindent (int f, int n, int k)
{
  int nicol, i;
  int len = llength (curwp->w_dot.p);

  /* Find indentation of current line.
   */
  nicol = currentindent (&i);

  /* Look at the string following the whitespace in the
   * current line to determine the indentation of the next line.
   * If a single C-U argument was specified, or the previous line started
   * with a curly brace, indent by three spaces.
   * If two C-U arguments were specified, unindent by three spaces.
   */
  if ((len > 0 && testline (len - 1, "{")) || (f && (n != 16)))
    nicol += 3;
  else if (f && (n == 16))
    nicol = nicol < 3 ? 0 : nicol - 3;

  /* Insert a newline followed by the correct number of
     tabs and spaces to get the desired indentation.
   */
  return nlindent (nicol, i, f);
}

/*
 * Delete forward. This is real
 * easy, because the basic delete routine does
 * all of the work. Watches for negative arguments,
 * and does the right thing. If any argument is
 * present, it kills rather than deletes, to prevent
 * loss of text if typed with a big argument.
 * Normally bound to "C-D".
 */
int
forwdel (int f, int n, int k)
{
  if (n < 0)
    return (backdel (f, -n, KRANDOM));
  if (f != FALSE)		/* Really a kill.       */
    kdelete ();
  return (ldelete (n, f));
}

/*
 * Delete backwards. This is quite easy too,
 * because it's all done with other functions. Just
 * move the cursor back, and delete forwards.
 * Like delete forward, this actually does a kill
 * if presented with an argument.
 */
int
backdel (int f, int n, int k)
{
  register int s;

  if (n < 0)
    return (forwdel (f, -n, KRANDOM));
  if (f != FALSE)		/* Really a kill.       */
    kdelete ();
  if ((s = backchar (f, n, KRANDOM)) == TRUE)
    {
      saveundo(UMOVE, &curwp->w_dot);
      s = ldelete (n, f);
    }
  return (s);
}

/*
 * Kill line. If called without an argument,
 * it kills from dot to the end of the line, unless it
 * is at the end of the line, when it kills the newline.
 * If called with an argument of 0, it kills from the
 * start of the line to dot. If called with a positive
 * argument, it kills from dot forward over that number
 * of newlines. If called with a negative argument it
 * kills any text before dot on the current line,
 * then it kills back abs(arg) lines.
 */
int
killline (int f, int n, int k)
{
  register int chunk;
  register LINE *nextp;

  kdelete ();			/* Purge kill buffer.   */
  if (f == FALSE)
    {
      chunk = wllength (curwp->w_dot.p) - curwp->w_dot.o;
      if (chunk == 0)
	chunk = 1;
    }
  else if (n > 0)
    {
      chunk = wllength (curwp->w_dot.p) - curwp->w_dot.o + 1;
      nextp = lforw (curwp->w_dot.p);
      while (--n)
	{
	  if (nextp == curbp->b_linep)
	    return (FALSE);
	  chunk += wllength (nextp) + 1;
	  nextp = lforw (nextp);
	}
    }
  else
    {				/* n <= 0               */
      chunk = curwp->w_dot.o;
      curwp->w_dot.o = 0;
      while (n++)
	{
	  if (curwp->w_dot.p == firstline (curbp))
	    break;
	  curwp->w_dot.p = lback (curwp->w_dot.p);
	  curwp->w_flag |= WFMOVE;
	  chunk += wllength (curwp->w_dot.p) + 1;
	}
    }
  return (ldelete (chunk, TRUE));
}

/*
 * Yank text back from the kill buffer. This
 * is really easy. All of the work is done by the
 * standard insert routines. All you do is run the loop,
 * and check for errors. The blank
 * lines are inserted with a call to "newline"
 * instead of a call to "lnewline" so that the magic
 * stuff that happens when you type a carriage
 * return also happens when a carriage return is
 * yanked back from the kill buffer.
 * An attempt has been made to fix the cosmetic bug
 * associated with a yank when dot is on the top line of
 * the window (nothing moves, because all of the new
 * text landed off screen).
 */
int
yank (int f, int n, int k)
{
  register int c;
  register int i, j;
  register LINE *lp;
  register int nline;
  char lbuf[80];
  uchar ubuf[6];
  int ulen;

  if (n < 0)
    return (FALSE);
  nline = 0;			/* Newline counting.    */
  while (n--)
    {
      i = j = 0;
      while ((ulen = kremove (i, ubuf)) > 0)
	{
	  c = ubuf[0];
	  if (c == '\n')
	    {			/* Newline is special   */
	      if (j > 0)
		{		/* Flush the buffer.    */
		  linsert (j, 0, lbuf);
		  j = 0;
		}
	      if (newline (FALSE, 1, KRANDOM) == FALSE)
		return (FALSE);
	      ++nline;
	    }
	  else
	    {			/* Not newline  */
	      if (j + ulen >= sizeof (lbuf))
		{		/* Need flush?  */
		  linsert (j, 0, lbuf);
		  j = 0;
		}
	      /* Buffer it    */
	      memcpy (&lbuf[j], ubuf, ulen);
	      j += ulen;
	    }
	  ++i;
	}
      if (j > 0)		/* Flush the buffer.    */
	linsert (j, 0, lbuf);
    }
  lp = curwp->w_linep;		/* Cosmetic adjustment  */
  if (curwp->w_dot.p == lp)
    {				/* if offscreen insert. */
      while (nline-- && lp != firstline (curbp))
	lp = lback (lp);
      curwp->w_linep = lp;	/* Adjust framing.      */
      curwp->w_flag |= WFHARD;
    }
  return (TRUE);
}

/*
 * Set the tab size according to the numeric argument.
 */
int
settabsize (int f, int n, int k)
{
  register EWINDOW *wp;

  if (!f)			/* no argument?         */
    n = 8;			/* reset to default     */
  else if (n < 2 || n > 32)
    {
      eprintf ("Illegal tab size %d", n);
      return (FALSE);
    }
  eprintf ("[Tab size set to %d characters]", n);
  tabsize = n;
  ALLWIND (wp)
  {
    wp->w_flag |= WFHARD;	/* redraw all windows */
  }
  return (TRUE);
}

/*
 * If an argument is present, toggle the overstrike flag;
 * otherwise, set it to the argument.
 */
int
setoverstrike (int f, int n, int k)
{
  overstrike = f ? n : !overstrike;
  eprintf (overstrike ? "[Overstrike mode]" : "[Insert mode]");
  return (TRUE);
}

/*
 * Check for heap corruption.
 */
int
checkheap (int f, int n, int k)
{
#if defined(__TURBOC__) && !defined(__linux__)
  if (heapcheck () >= 0)
    eprintf ("[Heap OK]");
  else
    eprintf ("[Heap corrupted]");
#else
  eprintf ("[Not implemented]");
#endif
  return (TRUE);
}
