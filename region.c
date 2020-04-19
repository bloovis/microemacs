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
 *		Region based commands.
 * Version:	29
 * Last edit:	15-Jul-86
 * By:		rex::conroy
 *		decvax!decwrl!dec-rhea!dec-rex!conroy
 * Modified by:	Mark Alexander
 *		drivax!alexande
 * What:	Region operations.
 *
 * The routines in this file
 * deal with the region, that magic space
 * between "." and mark. Some functions are
 * commands. Some functions are just for
 * internal use.
 */
#include	"def.h"


/*
 * Set size, and check for overflow.
 */
static int
setsize (REGION *rp, long size)
{
  rp->r_size = size;
  if (rp->r_size != size)
    {
      eprintf ("Region is too large");
      return (FALSE);
    }
  return (TRUE);
}

/*
 * This routine figures out the bound of the region
 * in the current window, and stores the results into the fields
 * of the REGION structure. Dot and mark are usually close together,
 * but I don't know the order, so I scan outward from dot, in both
 * directions, looking for mark. The size is kept in a long. At the
 * end, after the size is figured out, it is assigned to the size
 * field of the region structure. If this assignment loses any bits,
 * then we print an error. This is "type independent" overflow
 * checking. All of the callers of this routine should be ready to
 * get an ABORT status, because I might add a "if regions is big,
 * ask before clobberring" flag.
 */
int
getregion (REGION *rp)
{
  register LINE *flp;
  register LINE *blp;
  register long fsize;		/* Long now.            */
  register long bsize;

  if (curwp->w_mark.p == NULL)
    {
      eprintf ("No mark in this window");
      return (FALSE);
    }
  if (curwp->w_dot.p == curwp->w_mark.p)
    {				/* "r_size" always ok.  */
      rp->r_pos.p = curwp->w_dot.p;
      if (curwp->w_dot.o < curwp->w_mark.o)
	{
	  rp->r_pos.o = curwp->w_dot.o;
	  rp->r_size = curwp->w_mark.o - curwp->w_dot.o;
	}
      else
	{
	  rp->r_pos.o = curwp->w_mark.o;
	  rp->r_size = curwp->w_dot.o - curwp->w_mark.o;
	}
      return (TRUE);
    }

  /* Get region size, which is number of characters, not number of bytes. */
  blp = curwp->w_dot.p;
  flp = curwp->w_dot.p;
  bsize = curwp->w_dot.o;
  fsize = wllength (flp) - bsize + 1;	/* +1 for newline */
  while (flp != curbp->b_linep || blp != firstline (curbp))
    {
      if (flp != curbp->b_linep)
	{
	  flp = lforw (flp);
	  if (flp == curwp->w_mark.p)
	    {
	      rp->r_pos.p = curwp->w_dot.p;
	      rp->r_pos.o = curwp->w_dot.o;
	      return (setsize (rp, fsize + curwp->w_mark.o));
	    }
	  fsize += wllength (flp) + 1;
	}
      if (blp != firstline (curbp))
	{
	  blp = lback (blp);
	  bsize += wllength (blp) + 1;
	  if (blp == curwp->w_mark.p)
	    {
	      rp->r_pos.p = blp;
	      rp->r_pos.o = curwp->w_mark.o;
	      return (setsize (rp, bsize - curwp->w_mark.o));
	    }
	}
    }
  eprintf ("Bug: lost mark");	/* Gak!                 */
  return (FALSE);
}

/*
 * Kill the region. Ask "getregion"
 * to figure out the bounds of the region.
 * Move "." to the start, and kill the characters.
 * If an argument is provided, don't put the
 * characters in the kill buffer (useful if
 * you run out of memory while editing).
 */
int
killregion (int f, int n, int k)
{
  register int s;
  REGION region;

  if ((s = getregion (&region)) != TRUE)
    return (s);
  kdelete ();			/* Purge kill buffer    */
  curwp->w_dot = region.r_pos;
  saveundo (UMOVE, &curwp->w_dot);
  return (ldelete (region.r_size, !f));
}

/*
 * Copy all of the characters in the
 * region to the kill buffer. Don't move dot
 * at all. This is a bit like a kill region followed
 * by a yank.
 */
int
copyregion (int f, int n, int k)
{
  register LINE *linep;
  register int loffs;
  register int chunk;
  REGION region;

  if (getregion (&region) != TRUE)
    return (FALSE);
  kdelete ();			/* Purge kill buffer    */
  linep = region.r_pos.p;	/* Current line.        */
  loffs = region.r_pos.o;	/* Current offset.      */
  while (region.r_size > 0)
    {
      if (loffs == wllength (linep))
	{			/* End of line.         */
	  if (kinsert ("\n", 1) != TRUE)
	    return (FALSE);
	  linep = lforw (linep);
	  loffs = 0;
	  region.r_size--;
	}
      else
	{			/* Middle of line.      */
	  const uchar *cptr;
	  int bytes;

          chunk = wllength (linep) - loffs;
	  if (chunk > region.r_size)
	    chunk = region.r_size;
	  cptr = wlgetcptr (linep, loffs);
	  bytes = unblen (cptr, chunk);
	  if (kinsert ((char *) cptr, bytes) != TRUE)
	    return (FALSE);
	  loffs += chunk;
	  region.r_size -= chunk;
	}
    }
  eprintf ("[Region copied]");
  return (TRUE);
}

/*
 * Lower case region. Zap all of the upper
 * case characters in the region to lower case. Use
 * the region code to set the limits. Scan the buffer,
 * doing the changes. Call "lchange" to ensure that
 * redisplay is done in all buffers. 
 */
int
lowerregion (int f, int n, int k)
{
  register LINE *linep;
  register int loffs;
  register int c;
  register int s;
  REGION region;

  if ((s = getregion (&region)) != TRUE)
    return (s);
  if (checkreadonly () == FALSE)
    return FALSE;
  lchange (WFHARD);
  linep = region.r_pos.p;
  loffs = region.r_pos.o;
  saveundo (UMOVE, &curwp->w_dot);
  while (region.r_size--)
    {
      if (loffs == wllength (linep))
	{
	  linep = lforw (linep);
	  loffs = 0;
	}
      else
	{
	  c = wlgetc (linep, loffs);
	  if (CISUPPER (c) != FALSE)
	    {
	      POS pos;

	      pos.p = linep;
	      pos.o = loffs;
	      lputc (pos, CTOLOWER (c));
	    }
	  ++loffs;
	}
    }
  return (TRUE);
}

/*
 * Upper case region. Zap all of the lower
 * case characters in the region to upper case. Use
 * the region code to set the limits. Scan the buffer,
 * doing the changes. Call "lchange" to ensure that
 * redisplay is done in all buffers. 
 */
int
upperregion (int f, int n, int k)
{
  register LINE *linep;
  register int loffs;
  register int c;
  register int s;
  REGION region;

  if ((s = getregion (&region)) != TRUE)
    return (s);
  if (checkreadonly () == FALSE)
    return FALSE;
  lchange (WFHARD);
  linep = region.r_pos.p;
  loffs = region.r_pos.o;
  saveundo (UMOVE, &curwp->w_dot);
  while (region.r_size--)
    {
      if (loffs == wllength (linep))
	{
	  linep = lforw (linep);
	  loffs = 0;
	}
      else
	{
	  c = wlgetc (linep, loffs);
	  if (CISLOWER (c) != FALSE)
	    {
	      POS pos;

	      pos.p = linep;
	      pos.o = loffs;
	      lputc (pos, CTOUPPER (c));
	    }
	  ++loffs;
	}
    }
  return (TRUE);
}

/*
 * Indent region. Adjust the indentation of the lines
 * in the region by the number of spaces in the argument.
 * Call "lchange" to ensure that
 * redisplay is done in all buffers. 
 */
int
indentregion (int f, int n, int k)
{
  register int nicol;
  register int i;
  register int c;
  register int s;
  REGION region;
  int llen;

  if ((s = getregion (&region)) != TRUE)
    return (s);
  if (checkreadonly () == FALSE)
    return FALSE;
  lchange (WFHARD);
  curwp->w_dot.p = region.r_pos.p;
  curwp->w_dot.o = 0;
  region.r_size += region.r_pos.o;
  while (region.r_size > 0)
    {
      saveundo (UMOVE, &curwp->w_dot);
      llen = wllength (curwp->w_dot.p);
      region.r_size -= llen + 1;
      nicol = 0;

      /* Find the indentation level of this line.
       */
      for (i = 0; i < llen; ++i)
	{
	  c = wlgetc (curwp->w_dot.p, i);
	  if (c != ' ' && c != '\t')
	    break;
	  if (c == '\t')
	    nicol += (tabsize - nicol % tabsize) - 1;
	  ++nicol;
	}

      /* Delete the leading white space in this line, and replace
       * it with enough tabs and spaces to add the specified
       * indentation.  */
      if (llen != 0 && (nicol += n) >= 0)
	{
	  ldelete (i, FALSE);
	  if ((i = nicol / tabsize) != 0
	      && linsert (i, '\t', NULLPTR) == FALSE)
	    return (FALSE);
	  if ((i = nicol % tabsize) != 0
	      && linsert (i, ' ', NULLPTR) == FALSE)
	    return (FALSE);
	}
      curwp->w_dot.p = lforw (curwp->w_dot.p);
      curwp->w_dot.o = 0;
    }
  return (TRUE);
}
