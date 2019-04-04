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
 *		Text line handling.
 * Version:	29
 * Last edit:	6-Jul-88
 * By:		rex::conroy
 *		decvax!decwrl!dec-rhea!dec-rex!conroy
 * Modified by:	Mark Alexander
 *		drivax!alexande
 *
 * The functions in this file
 * are a general set of line management
 * utilities. They are the only routines that
 * touch the text. They also touch the buffer
 * and window structures, to make sure that the
 * necessary updating gets done. There are routines
 * in this file that handle the kill buffer too.
 * It isn't here for any good reason.
 *
 * Note that this code only updates the dot and
 * mark values in the window list. Since all the code
 * acts on the current window, the buffer that we
 * are editing must be being displayed, which means
 * that "b_nwnd" is non zero, which means that the
 * dot and mark values in the buffer headers are
 * nonsense.
 */
#include	"def.h"

#define	NBLOCK	16		/* Line block chunk size        */

#ifndef	KBLOCK
#define	KBLOCK	256		/* Kill buffer block size.      */
#endif

#define UPPER 0x01
#define LOWER 0x02

static char *kbufp = NULL;		/* Kill buffer data.            */
static int kindex = -1;			/* kremove UTF-8 index into KB.	*/
static const uchar *kptr = NULL;	/* kremove pointer into KB.	*/
static int kused = 0;			/* # of bytes used in KB.       */
static int ksize = 0;			/* # of bytes allocated in KB.  */
static int kchars = 0;			/* # of UTF-8 chars in KB.	*/

/*
 * Forward declarations.
 */
static int ldelnewline (void);

/*
 * This routine allocates a block
 * of memory large enough to hold a LINE
 * containing "used" characters. The block is
 * always rounded up a bit. Return a pointer
 * to the new block, or NULL if there isn't
 * any memory left. Print a message in the
 * message line if no space.
 */
LINE *
lalloc (int used)
{
  register LINE *lp;
  register int size;

  size = (used + NBLOCK - 1) & ~(NBLOCK - 1);
  if (size == 0)		/* Assume that an empty */
    size = NBLOCK;		/* line is for type-in. */
  if ((lp = (LINE *) malloc (LINEHDR_SIZE + size)) == NULL)
    {
      eprintf ("Cannot allocate %d bytes", size);
      return (NULL);
    }
  lp->l_size = size;
  lp->l_used = used;
  return (lp);
}

/*
 * This routine is similar to lalloc, except
 * that it does not round up the allocation size.
 * This is called by the file read functions,
 * and saves lots of memory reading in a new file.
 */
LINE *
lallocx (int used)
{
  register LINE *lp;

  if ((lp = (LINE *) malloc (LINEHDR_SIZE + used)) == NULL)
    {
      eprintf ("Cannot allocate %d bytes", used);
      return (NULL);
    }
  lp->l_size = lp->l_used = used;
  return (lp);
}

/*
 * Delete line "lp". Fix all of the
 * links that might point at it (they are
 * moved to offset 0 of the next line.
 * Unlink the line from whatever buffer it
 * might be in. Release the memory. The
 * buffers are updated too; the magic conditions
 * described in the above comments don't hold
 * here.
 */
#if 0
void
lfree (lp)
     register LINE *lp;
{
  register BUFFER *bp;
  register EWINDOW *wp;

  ALLWIND (wp)
  {
    if (wp->w_linep == lp)
      wp->w_linep = lp->l_fp;
    if (wp->w_savep == lp)
      wp->w_savep = lp->l_fp;
    if (wp->w_dot.p == lp)
      {
	wp->w_dot.p = lp->l_fp;
	wp->w_dot.o = 0;
      }
    if (wp->w_mark.p == lp)
      {
	wp->w_mark.p = lp->l_fp;
	wp->w_mark.o = 0;
      }
  }
  ALLBUF (bp)
  {
    if (bp->b_nwnd == 0)
      {
	if (bp->b_dot.p == lp)
	  {
	    bp->b_dot.p = lp->l_fp;
	    bp->b_dot.o = 0;
	  }
	if (bp->b_mark.p == lp)
	  {
	    bp->b_mark.p = lp->l_fp;
	    bp->b_mark.o = 0;
	  }
      }
  }
  lp->l_bp->l_fp = lp->l_fp;
  lp->l_fp->l_bp = lp->l_bp;
  free ((char *) lp);
}
#endif

/*
 * This routine gets called when
 * a character is changed in place in the
 * current buffer. It updates all of the required
 * flags in the buffer and window system. The flag
 * used is passed as an argument; if the buffer is being
 * displayed in more than 1 window we change EDIT to
 * HARD. Set MODE if the mode line needs to be
 * updated (the "*" has to be set).
 */
void
lchange (int flag)
{
  register EWINDOW *wp;

  if ((curbp->b_flag & BFCHG) == 0)
    {				/* First change, so     */
      flag |= WFMODE;		/* update mode lines.   */
      curbp->b_flag |= BFCHG;
    }
  ALLWIND (wp)
  {
    if (wp->w_bufp == curbp)
      {				/* Same buffer?         */
	wp->w_flag |= flag;
	if (wp != curwp)	/* Different window?    */
	  wp->w_flag |= WFHARD;
      }
  }
}

/*
 * Adjust line positions in wp->w_ring to account for an insertion of nchars characters
 * at position *oldpos, where newlp is the line that replaced oldpos.p.
 */
static void
adjustforinsert (const POS *oldpos, LINE *newlp, int nchars, EWINDOW *wp)
{
  int i;

  for (i = 0; i < wp->w_ring.m_count; i++)
    {
      POS *pos = &wp->w_ring.m_ring[i];

      if (pos->p == oldpos->p)
	{
	  pos->p = newlp;
	  if (pos->o > oldpos->o)
	    pos->o += nchars;
	}
    }
}

/*
 * If the string pointer "s" is NULL,
 * insert "n" copies of the Uncode character "c", converted to UTF-8
 * at the current location of dot.  Otherwise,
 * insert "n" bytes from the UTF-8 string "s" at the current
 * location of dot.  In the easy case,
 * all that happens is the text is stored in the line.
 * In the hard case, the line has to be reallocated.
 * When the window list is updated, take special
 * care; I screwed it up once. You always update dot
 * in the current window. You update mark, and a
 * dot in another window, if it is greater than
 * the place where you did the insert. Return TRUE
 * if all is well, and FALSE on errors.
 */
int
linsert (int n, int c, char *s)
{
  register EWINDOW *wp;
  register LINE *lp2;
  register LINE *lp3;
  POS dot;
  int chars, bytes, offset, buflen;
  char buf[6];

  if (checkreadonly () == FALSE)
    return FALSE;
  lchange (WFEDIT);
  dot = curwp->w_dot;		/* Save current line.	*/

  /* Get byte offset of insertion point. */
  offset = wloffset (dot.p, dot.o);

  if (s != NULL)
    {
      /* Set chars to number of characters in UTF-8 string.
       * Set bytes to number of bytes in the string.
       */
      chars = unslen ((uchar *)s, n);
      bytes = n;
      saveundo (UINSERT, NULL, 1, chars, bytes, s);
    }
 else
    {
      /* Convert character to UTF-8, store in buf. Set chars to
       * total number of characters to insert.  Set bytes to total number
       * of bytes to insert.
       */
      buflen = uputc (c, (uchar *)buf);
      chars = n;
      bytes = n * buflen;
      saveundo (UINSERT, NULL, n, 1, buflen, buf);
    }

  if (dot.p == curbp->b_linep)
    {				/* At the end: special  */
      if (offset != 0)
	{
	  eprintf ("bug: linsert");
	  return (FALSE);
	}
      if ((lp2 = lalloc (bytes)) == NULL)	/* Allocate new line    */
	return (FALSE);
      lp3 = dot.p->l_bp;		/* Previous line        */
      lp3->l_fp = lp2;			/* Link in              */
      lp2->l_fp = dot.p;
      dot.p->l_bp = lp2;
      lp2->l_bp = lp3;
    }
  else if (dot.p->l_used + bytes > dot.p->l_size)
    {					/* Hard: reallocate     */
      if ((lp2 = lalloc (dot.p->l_used + bytes)) == NULL)
	return (FALSE);
      memcpy (&lp2->l_text[0], &dot.p->l_text[0], offset);
      memcpy (&lp2->l_text[offset + bytes], &dot.p->l_text[offset],
	      dot.p->l_used - offset);	/* make room            */
      dot.p->l_bp->l_fp = lp2;
      lp2->l_fp = dot.p->l_fp;
      dot.p->l_fp->l_bp = lp2;
      lp2->l_bp = dot.p->l_bp;
      free ((char *) dot.p);
    }
  else
    {				/* Easy: in place       */
      lp2 = dot.p;		/* Pretend new line     */
      memmove (&dot.p->l_text[offset + bytes], &dot.p->l_text[offset],
	       dot.p->l_used - offset);	/* make room            */
      lp2->l_used += bytes;		/* bump length up       */
    }

  if (s == NULL)		/* fill or copy?        */
    {
      int i, o;

      for (i = 0, o = offset; i < chars; i++, o += buflen)
	memcpy (&lp2->l_text[o], buf, buflen);	/* fill the characters  */
    }
  else
    memcpy (&lp2->l_text[offset], s, bytes);	/* copy the characters  */

  ALLWIND (wp)
  {				/* Update windows       */
    if (wp->w_linep == dot.p)
      wp->w_linep = lp2;
    if (wp->w_savep == dot.p)
      wp->w_savep = lp2;
    if (wp->w_dot.p == dot.p)
      {
	wp->w_dot.p = lp2;
	if (wp == curwp || wp->w_dot.o > dot.o)
	  wp->w_dot.o += chars;
      }
    adjustforinsert (&dot, lp2, chars, wp);
  }
  return (TRUE);
}

/*
 * Insert the string s containing len bytes,
 * but treat \n characters properly, i.e., as the starts of
 * new lines instead of raw characters.  Return TRUE
 * if successful, or FALSE if an error occurs.
 */
int
insertwithnl (const char *s, int len)
{
  int status = TRUE;
  const char *end = s + len;

  while (status == TRUE && s < end)
    {
      const char *nl = memchr (s, '\n', end - s);
      if (nl == NULL)
	{
	  status = linsert (end - s, 0, (char *) s);
	  s = end;
	}
      else
	{
	  if (nl != s)
	    {
	      status = linsert (nl - s, 0, (char *) s);
	      if (status != TRUE)
		break;
	    }
	  status = lnewline ();
	  s = nl + 1;
	}
    }
  return status;
}

/*
 * Adjust line position *pos to account for an insertion of a newline
 * at position *oldpos, where newlp is the new line that replaced the first
 * part of line pos.p.
 */
static void
adjustfornewline (const POS *oldpos, LINE *newlp, EWINDOW *wp)
{
  int i;

  for (i = 0; i <= wp->w_ring.m_count; i++)
    {
      POS *pos;

      if (i == wp->w_ring.m_count)
	pos = &wp->w_dot;
      else
	pos = &wp->w_ring.m_ring[i];

      if (pos->p == oldpos->p)
	{
	  if (pos->o < oldpos->o)
	    pos->p = newlp;
	  else
	    pos->o -= oldpos->o;
	}
    }
}

/*
 * Insert a newline into the buffer
 * at the current location of dot in the current
 * window. The funny ass-backwards way it does things
 * is not a botch; it just makes the last line in
 * the file not a special case. Return TRUE if everything
 * works out and FALSE on error (memory allocation
 * failure). The update of dot and mark is a bit
 * easier then in the above case, because the split
 * forces more updating.
 */
int
lnewline (void)
{
  register LINE *lp1;
  register LINE *lp2;
  register int doto;
  register EWINDOW *wp;
  int offset;

  if (checkreadonly () == FALSE)
    return FALSE;
  lchange (WFHARD);
  lp1 = curwp->w_dot.p;			/* Get the address and  */
  doto = curwp->w_dot.o;		/* offset of "."        */
  offset = wloffset (lp1, doto);	/* Actual byte offset	*/

  /* Save undo information. */
  saveundo (UINSERT, NULL, 1, 1, 1, "\n");

  if ((lp2 = lalloc (offset)) == NULL)	/* New first half line  */
    return (FALSE);
  memcpy (&lp2->l_text[0], &lp1->l_text[0], offset);	/* shuffle text */
  if (offset != 0) {
    memmove (&lp1->l_text[0], &lp1->l_text[offset], lp1->l_used - offset);
  }
  lp1->l_used -= offset;
  lp2->l_bp = lp1->l_bp;
  lp1->l_bp = lp2;
  lp2->l_bp->l_fp = lp2;
  lp2->l_fp = lp1;

  ALLWIND (wp)
  {				/* Update windows       */
    POS dot;
    if (wp->w_linep == lp1)
      wp->w_linep = lp2;
    if (wp->w_savep == lp1)
      wp->w_savep = lp2;
    dot.p = lp1;
    dot.o = doto;
    adjustfornewline (&dot, lp2, wp);
  }
  return (TRUE);
}

/*
 * Adjust line position *pos to account for a deletion of nchars characters
 * at position *oldpos.
 */
static void
adjustfordelete (POS *oldpos, int nchars, EWINDOW *wp)
{
  int i;

  for (i = 0; i <= wp->w_ring.m_count; i++)
    {
      POS *pos;

      if (i == wp->w_ring.m_count)
	pos = &wp->w_dot;
      else
	pos = &wp->w_ring.m_ring[i];

      if (pos->p == oldpos->p && pos->o >= oldpos->o)
	{
	  int o = pos->o - nchars;
	  if (o < oldpos->o)
	    pos->o = oldpos->o;
	  else
	    pos->o = o;
	}
    }
}


/*
 * This function deletes "n" characters,
 * starting at dot. Because lines are stored as UTF-8
 * characters, a character may contain more than one byte.
 *
 * It understands how do deal
 * with end of lines, etc. It returns TRUE if all
 * of the characters were deleted, and FALSE if
 * they were not (because dot ran into the end of
 * the buffer. The "kflag" is TRUE if the text
 * should be put in the kill buffer.
 */
int
ldelete (int n, int kflag)
{
  uchar *cp1, *cp2, *end;
  POS dot;
  int bytes, chars;
  register EWINDOW *wp;

  if (n < 0)
    {
      eprintf ("Region is too large");
      return (FALSE);
    }
  if (checkreadonly () == FALSE)
    return FALSE;
  while (n != 0)
    {
      dot = curwp->w_dot;
      if (dot.p == curbp->b_linep)	/* Hit end of buffer.   */
	return (FALSE);

      /* Get pointer to first byte of the UTF-8 character
       * indexed by dot.  Then get the number of UTF-8 characters
       * in the rest of line.
       */
      cp1 = (uchar *) wlgetcptr (dot.p, dot.o);
      end = lend(dot.p);
      bytes = end - cp1;
      chars = unslen (cp1, bytes);
      if (chars > n)
	{
	  chars = n;
	  bytes = unblen (cp1, chars);
	}
      if (chars == 0)
	{			/* End of line, merge.  */
	  lchange (WFHARD);
	  if (ldelnewline () == FALSE
	      || (kflag != FALSE && kinsert ("\n", 1) == FALSE))
	    return (FALSE);
          saveundo(UDELETE, NULL, 1, 1, "\n");
	  --n;
	  continue;
	}
      lchange (WFEDIT);
      cp2 = cp1 + bytes;			/* Scrunch text.        */
      if (kflag != FALSE)	/* Kill?                */
	if (kinsert ((const char *) cp1, bytes) == FALSE)
	  return (FALSE);
      saveundo(UDELETE, NULL, chars, bytes, cp1);
      memmove (cp1, cp2, end - cp2);
      dot.p->l_used -= bytes;
      ALLWIND (wp)
      {				/* Fix windows          */
	adjustfordelete (&dot, chars, wp);
      }
      n -= chars;
    }
  return (TRUE);
}

/*
 * Adjust line position *pos to account for the deletion of the newline between
 * old lines lp1 and lp2, forming a new line newlp.
 */
static void
adjustfordelnewline (LINE *lp1, LINE *lp2, LINE *newlp, EWINDOW *wp)
{
  int i;

  for (i = 0; i <= wp->w_ring.m_count; i++)
    {
      POS *pos;

      if (i == wp->w_ring.m_count)
	pos = &wp->w_dot;
      else
	pos = &wp->w_ring.m_ring[i];

      if (pos->p == lp1)
	pos->p = newlp;
      else if (pos->p == lp2)
	{
	  pos->p = newlp;
	  pos->o += lp1->l_used;
	}
    }
}

/*
 * Delete a newline. Join the current line
 * with the next line. If the next line is the magic
 * header line always return TRUE; merging the last line
 * with the header line can be thought of as always being a
 * successful operation, even if nothing is done, and this makes
 * the kill buffer work "right". Easy cases can be done by
 * shuffling data around. Hard cases require that lines be moved
 * about in memory. Return FALSE on error and TRUE if all
 * looks ok. Called by "ldelete" only.
 */
static int
ldelnewline (void)
{
  register LINE *lp1;
  register LINE *lp2;
  register LINE *lp3;
  register EWINDOW *wp;

  lp1 = curwp->w_dot.p;
  lp2 = lp1->l_fp;
  if (lp2 == curbp->b_linep)	/* At the buffer end.   */
    return (TRUE);
  if (lp2->l_used <= lp1->l_size - lp1->l_used)
    {
      int chars = wllength (lp1);
      memcpy (&lp1->l_text[lp1->l_used], &lp2->l_text[0], lp2->l_used);	/* copy bytes up        */
      ALLWIND (wp)
      {
	if (wp->w_linep == lp2)
	  wp->w_linep = lp1;
	if (wp->w_savep == lp2)
	  wp->w_savep = lp1;
	if (wp->w_dot.p == lp2)
	  {
	    wp->w_dot.p = lp1;
	    wp->w_dot.o += chars;
	  }
	if (wp->w_mark.p == lp2)
	  {
	    wp->w_mark.p = lp1;
	    wp->w_mark.o += chars;
	  }
      }
      lp1->l_used += lp2->l_used;
      lp1->l_fp = lp2->l_fp;
      lp2->l_fp->l_bp = lp1;
      free ((char *) lp2);
      return (TRUE);
    }
  if ((lp3 = lalloc (lp1->l_used + lp2->l_used)) == NULL)
    return (FALSE);
  memcpy (&lp3->l_text[0], &lp1->l_text[0], lp1->l_used);
  memcpy (&lp3->l_text[lp1->l_used], &lp2->l_text[0], lp2->l_used);
  lp1->l_bp->l_fp = lp3;
  lp3->l_fp = lp2->l_fp;
  lp2->l_fp->l_bp = lp3;
  lp3->l_bp = lp1->l_bp;
  ALLWIND (wp)
  {
    if (wp->w_linep == lp1 || wp->w_linep == lp2)
      wp->w_linep = lp3;
    if (wp->w_savep == lp1 || wp->w_savep == lp2)
      wp->w_savep = lp3;
    adjustfordelnewline(lp1, lp2, lp3, wp);
  }
  free ((char *) lp1);
  free ((char *) lp2);
  return (TRUE);
}

/*
 * Replace the character at offset n in the line lp with
 * the Unicode character c, which is first converted to UTF-8.
 * This function used to be macro which directly overwrote
 * the byte at lp->l_text[n].  That worked with ASCII, but
 * in UTF-8, characters may be differently lengths.  Simplify
 * the logic by letting ldelete and linsert do the hard work.
 */
void
lputc (POS pos, wchar_t c)
{
  POS dot;
  int move;

  dot.p = curwp->w_dot.p;
  dot.o = curwp->w_dot.o;
  move = dot.p != pos.p || dot.o != pos.o;
  if (move)
    {
      curwp->w_dot.p = pos.p;
      curwp->w_dot.o = pos.o;
    }
  saveundo (UMOVE, &curwp->w_dot);
  ldelete (1, FALSE);
  linsert (1, c, NULLPTR);
  if (move)
    {
      curwp->w_dot.p = dot.p;
      curwp->w_dot.o = dot.o;
    }
  saveundo (UMOVE, &curwp->w_dot);
}

/*
 * Replace plen characters before dot with argument string.
 * Control-J characters in st are interpreted as newlines.
 * There is a casehack disable flag (normally it likes to match
 * case of replacement to what was there).
 */
int
lreplace (
     int plen,			/* length to remove             */
     const char *st,		/* replacement string           */
     int f)			/* case hack disable            */
{
  register int rlen;		/* replacement length           */
  register int rtype;		/* capitalization               */
  register int c;		/* used for random characters   */
  register int doto;		/* offset into line             */
  int clen;			/* length of UTF-8 char		*/
  const char *end;		/* end of replacement string	*/

  if (checkreadonly () == FALSE)
    return FALSE;
  if (casefold == FALSE)	/* is case folding turned off?  */
    f = TRUE;			/* disable case hack            */

  /*
   * Find the capitalization of the word that was found.
   * f says use exact case of replacement string (same thing that
   * happens with lowercase found), so bypass check.
   */
  backchar (TRUE, plen, KRANDOM);
  rtype = LOWER;
  c = wlgetc (curwp->w_dot.p, curwp->w_dot.o);
  if (CISUPPER (c) != FALSE && f == FALSE)
    {
      rtype = UPPER | LOWER;
      if (curwp->w_dot.o + 1 < wllength (curwp->w_dot.p))
	{
	  c = wlgetc (curwp->w_dot.p, curwp->w_dot.o + 1);
	  if (CISUPPER (c) != FALSE)
	    {
	      rtype = UPPER;
	    }
	}
    }

  /*
   * make the string lengths match (either pad the line
   * so that it will fit, or scrunch out the excess).
   * be careful with dot's offset.
   */
  saveundo(UMOVE, &curwp->w_dot);
  rlen = uslen ((const uchar *)st);
  doto = curwp->w_dot.o;
  if (plen > rlen)
    ldelete (plen - rlen, FALSE);
  else if (plen < rlen)
    {
      if (linsert (rlen - plen, ' ', NULLPTR) == FALSE)
	return (FALSE);
    }
  curwp->w_dot.o = doto;
  saveundo (UMOVE, &curwp->w_dot);

  /*
   * do the replacement:  If was capital, then place first
   * char as if upper, and subsequent chars as if lower.
   * If inserting upper, check replacement for case.
   */
  end = st + strlen (st);
  while (st < end)
    {
      c = ugetc ((const uchar *) st, 0, &clen);
      st += clen;
      if ((rtype & UPPER) != 0 && CISLOWER (c) != 0)
	c = CTOUPPER (c);
      if (rtype == (UPPER | LOWER))
	rtype = LOWER;
      if (c == '\n')
	{
	  if (curwp->w_dot.o == wllength (curwp->w_dot.p))
	    forwchar (FALSE, 1, KRANDOM);
	  else
	    {
	      ldelete (1, FALSE);
	      lnewline ();
	    }
	}
      else if (curwp->w_dot.p == curbp->b_linep)
	{
	  linsert (1, c, NULLPTR);
	}
      else if (curwp->w_dot.o == wllength (curwp->w_dot.p))
	{
	  ldelete (1, FALSE);
	  linsert (1, c, NULLPTR);
	}
      else
	{
	  lputc (curwp->w_dot, c);
	}
    }

  lchange (WFHARD);
  return (TRUE);
}

/*
 * Delete all of the text
 * saved in the kill buffer. Called by commands
 * when a new kill context is being created. The kill
 * buffer array is released, just in case the buffer has
 * grown to immense size. No errors.  The kill buffer is
 * not released if the last command was a killer.  In
 * any case, the kill bit is set in "thisflag".
 */
void
kdelete (void)
{
  thisflag |= CFKILL;		/* This is a kill cmd   */
  if ((lastflag & CFKILL) != 0)	/* Last cmd was kill?   */
    return;			/* Don't purge yet?     */
  if (kbufp != NULL)
    {
      free ((char *) kbufp);
      kbufp = NULL;
      kused = 0;
      ksize = 0;
      kchars = 0;
      kptr = NULL;
      kindex = -1;
    }
}

/*
 * Insert a string of character to the kill buffer,
 * enlarging the buffer if there isn't any room. Always
 * grow the buffer in chunks, on the assumption that if you
 * put something in the kill buffer you are going to put
 * more stuff there too later. Return TRUE if all is
 * well, and FALSE on errors. Print a message on
 * errors.
 */

#define REALLOC	1		/* set to 0 if realloc() not available  */

int
kinsert (const char *s, int n)
{
  register char *nbufp;

  if (kused + n > ksize)
    {
#if REALLOC
      if (kbufp == NULL)
	nbufp = malloc (ksize + n + KBLOCK);
      else
	nbufp = realloc (kbufp, ksize + n + KBLOCK);
      if (nbufp == NULL)
	{
#else
      if ((nbufp = realloc (kbufp, ksize + n + KBLOCK)) == NULL)
	{
#endif
	  eprintf ("Not enough memory for kill buffer");
	  return (FALSE);
	}
#if !REALLOC
      if (kbufp != NULL)
	{
	  memcpy (nbufp, kbufp, ksize);
	  free ((char *) kbufp);
	}
#endif
      kbufp = nbufp;
      ksize += n + KBLOCK;
    }
  memcpy (&kbufp[kused], s, n);
  kused += n;
  kchars += unslen ((const uchar *)s, n);
  return (TRUE);
}

/*
 * This function gets the n'th UTF-8 character from
 * the kill buffer, stores it in buf, which must be
 * at least 6 bytes long, and returns the number of
 * bytes stored.  If the character index "n" is
 * off the end, it returns "-1". This lets the caller
 * just scan along until it gets a "-1" back.
 *
 * To avoid the order-n-squared problem, we use
 * the kptr and kindex variables to iterate through
 * the kill buffer, without starting over from
 * the beginning of the buffer each time kremove is called.
 */
int
kremove (int n, uchar *buf)
{
  int len;

  if (n >= kchars)
    return -1;
  if (n != kindex)
    {
      kptr = ugetcptr ((const uchar *) kbufp, n);
      kindex = n;
    }
  len = uclen (kptr);
  memcpy (buf, kptr, len);
  kptr += len;
  ++kindex;
  return len;
}
