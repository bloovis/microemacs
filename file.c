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

/* $Header: /home/bloovis/cvsroot/pe/file.c,v 1.4 2005-11-11 17:30:26 bloovis Exp $
 *
 * Name:	MicroEMACS
 * 		File commands.
 * Version:	29
 * By:		rex::conroy
 *		decvax!decwrl!dec-rhea!dec-rex!conroy
 *
 * $Log: file.c,v $
 * Revision 1.4  2005-11-11 17:30:26  bloovis
 * (filesave): Improve the detection of the elusive bug that
 * corrupts the casefold variable on file saves.
 *
 * Revision 1.3  2005/10/18 02:18:12  bloovis
 * Rename some things to avoid conflict with ncurses.
 *
 * Revision 1.2  2005/05/31 18:18:20  bloovis
 * (filesave): Add debug output to try to catch weird bug
 * where casefold gets zapped sometimes.
 *
 * Revision 1.1.1.1  2003/11/06 02:51:52  bloovis
 * Imported sources
 *
 * Revision 1.5  2001/03/05 16:04:05  malexander
 * (visit_file): Use structure assignments for brevity.
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
 * Revision 1.2  1996/10/22 15:34:52  marka
 * Expand filenames with leading tilde.
 *
 * Revision 1.1  1996/10/22 09:06:00  marka
 * Initial revision
 *
 * Revision 1.4  91/04/19  23:18:14  alexande
 * Added support for a third path delimiter character for buffer names.
 * Changed save-tabs command to a toggle if no argument supplied.
 * 
 * Revision 1.3  91/02/06  09:11:37  alexande
 * Call egetfname() instead of ereply() to allow filename autocompletion.
 * 
 * Revision 1.2  90/10/23  17:13:02  alexande
 * Changes to allow arbitrary-length lines.
 * 
 *
 */
#include	"def.h"

int savetabs = 1;		/* TRUE if tabs are preserved when saving files */

/*
 * External declarations.
 */
extern char *fftilde (char *filename);	/* fileio.c */

/*
 * Read a file into the current
 * buffer. This is really easy; all you do is
 * find the name of the file, and call the standard
 * "read a file into the current buffer" code.
 */
int
fileread (int f, int n, int k)
{
  register int s;
  char fname[NFILEN];
  char *expanded_fname;

  if ((s = egetfname ("Read file: ", fname, NFILEN)) != TRUE)
    return (s);
  adjustcase (fname);
  expanded_fname = fftilde (fname);
  return (readin (expanded_fname));
}

/*
 * Insert a file into the current window.
 */
static int
insertf (const char *fname)
{
  register EWINDOW *wp;
  register BUFFER *bp;
  register LINE *lp1, *lp2;
  register int doto;
  register LINE *dotp;
  int s, hadnl;

  bp = curbp;			/* Make local copy      */
  wp = curwp;			/* Make local copy      */
  if ((s = ffropen (fname)) != FIOSUC)
    {				/* Hard file open.      */
      if (kbdmop == NULL)
	{
	  if (s == FIOFNF)
	    eprintf ("File not found");
	  else
	    eprintf ("File open error");
	}
      return (FALSE);
    }

  doto = wp->w_dot.o;		/* Save dot offset      */
  if (doto != 0)		/* In middle of line?   */
    lnewline ();		/* Insert dummy line    */
  lp2 = wp->w_dot.p;		/* Insert lines between */
  lp1 = lback (lp2);		/*  lp1 and lp2         */
  hadnl = readlines (lp2, &s);	/* Read the lines       */

  wp->w_dot.p = lp2;		/* Move dot after lines */
  wp->w_dot.o = 0;		/*  just inserted       */
  if (hadnl)
    {				/* Last line had \n?    */
      if (lp2 == bp->b_linep)	/* Last line in buffer? */
	lnewline ();		/* Insert extra newline */
    }
  else
    {				/* Last line had no \n  */
      if (lp2 != bp->b_linep)	/* Not end of buffer?   */
	backdel (FALSE, 1, KRANDOM);
    }

  wp->w_dot.p = lforw (lp1);	/* Move dot to first    */
  wp->w_dot.o = 0;		/*  of the new lines    */
  if (doto != 0)		/* Was a line split?    */
    backdel (FALSE, 1, KRANDOM);	/* Un-split the line    */
  dotp = wp->w_dot.p;		/* Save new dot value   */
  doto = wp->w_dot.o;

#if	BACKUP
  bp->b_flag |= BFBAK | BFCHG;	/* Need a backup.       */
#else
  bp->b_flag |= BFCHG;		/* Need a backup.       */
#endif

  ALLWIND (wp)
  {
    if (wp->w_bufp == bp)
      {
	wp->w_dot.p = dotp;
	wp->w_dot.o = doto;
	wp->w_mark.o = 0;
	wp->w_flag |= WFMODE | WFHARD;
      }
  }

  return (s != FIOERR);		/* False if error.      */
}

/*
 * Read a file into the current
 * buffer. This is really easy; all you do is
 * find the name of the file, and call the standard
 * "read a file into the current buffer" code.
 */
int
fileinsert (int f, int n, int k)
{
  register int s;
  char fname[NFILEN];
  char *expanded_fname;

  if ((s = egetfname ("Insert file: ", fname, NFILEN)) != TRUE)
    return (s);
  adjustcase (fname);
  expanded_fname = fftilde (fname);
  return (insertf (expanded_fname));
}

/*
 * Select a file for editing.
 * Look around to see if you can find the
 * file in another buffer; if you can find it
 * just switch to the buffer. If you cannot find
 * the file, create a new buffer, read in the
 * text, and switch to the new buffer.
 *
 * filevisit is the function bound to 'file-visit'.
 * It prompts for the filename.
 *
 * visit_file is a helper function for filevisit.
 * It takes the filename to read as a parameter.
 * It's also used by tags.c.
 */
int
visit_file (char *fname)
{
  register BUFFER *bp;
  register EWINDOW *wp;
  register LINE *lp;
  register int i;
  register int s;
  char bname[NBUFN];
  char *expanded_fname;

  adjustcase (fname);
  expanded_fname = fftilde (fname);
  ALLBUF (bp)
  {
    if (strcmp (bp->b_fname, expanded_fname) == 0)
      {
	addwind (curwp, -1);
	strcpy (oldbufn, curbp->b_bname);	/* save name */
	curbp = bp;
	curwp->w_bufp = bp;
	addwind (curwp, 1);
	if (bp->b_nwnd != 1)
	  ALLWIND (wp)
	  {
	    if (wp != curwp && wp->w_bufp == bp)
	      {
		curwp->w_dot = wp->w_dot;
		curwp->w_mark = wp->w_mark;
		break;
	      }
	  }
	lp = curwp->w_dot.p;
	i = curwp->w_ntrows / 2;
	while (i-- && lp != firstline (curbp))
	  lp = lback (lp);
	curwp->w_linep = lp;
	curwp->w_flag |= WFMODE | WFHARD;
	if (kbdmop == NULL)
	  eprintf ("[Old buffer]");
	return (TRUE);
      }
  }
  makename (bname, expanded_fname);	/* New buffer name.     */
  while ((bp = bfind (bname, FALSE)) != NULL)
    {
      s = ereply ("Buffer name: ", bname, NBUFN);
      if (s == ABORT)		/* ^G to just quit      */
	return (s);
      if (s == FALSE)
	{			/* CR to clobber it     */
	  makename (bname, expanded_fname);
	  break;
	}
    }
  if (bp == NULL && (bp = bfind (bname, TRUE)) == NULL)
    {
      eprintf ("Cannot create buffer");
      return (FALSE);
    }
  addwind (curwp, -1);		/* Undisplay.           */
  strcpy (oldbufn, curbp->b_bname);	/* save current name    */
  curbp = bp;			/* Switch to it.        */
  curwp->w_bufp = bp;
  curbp->b_nwnd++;
  return (readin (expanded_fname));	/* Read it in.          */
}

int
filevisit (int f, int n, int k)
{
  register int s;
  char fname[NFILEN];

  if ((s = egetfname ("Visit file: ", fname, NFILEN)) != TRUE)
    return (s);
  return visit_file (fname);
}


/*
 * Visit a file but mark the buffer as read-only to prevent
 * accidental changes.
 */
int
filevisitreadonly (int f, int n, int k)
{
  register int s;
  char fname[NFILEN];

  if ((s = egetfname ("Visit file: ", fname, NFILEN)) != TRUE)
    return (s);
  if (visit_file (fname) == FALSE)
    return FALSE;
  curbp->b_flag |= BFRO;
  return TRUE;
}

/*
 * Toggle the read-only state of the current buffer.
 */
int
togglereadonly (int f, int n, int k)
{
  curbp->b_flag ^= BFRO;
  if ((curbp->b_flag & BFRO) != 0)
    eprintf ("Buffer is now read-only");
  else
    eprintf ("Buffer is now read-write");
  return TRUE;
}

/*
 * Check whether the current buffer is read-only.  It if is,
 * display an error message and return FALSE; otherwise return TRUE.
 */
int
checkreadonly (void)
{
  if ((curbp->b_flag & BFRO) != 0)
    {
      eprintf ("Buffer is read-only");
      return FALSE;
    }
  else
    return TRUE;
}


/*
 * Read the file "fname" into the current buffer.
 * Make all of the text in the buffer go away, after checking
 * for unsaved changes. This is called by the "read" command, the
 * "visit" command, and the mainline (for "uemacs file"). If the
 * BACKUP conditional is set, then this routine also does the read
 * end of backup processing. The BFBAK flag, if set in a buffer,
 * says that a backup should be taken. It is set when a file is
 * read in, but not on a new file (you don't need to make a backup
 * copy of nothing). Return a standard status. Print a summary
 * (lines read, error message) out as well.
 */
int
readin (const char *fname)
{
  register EWINDOW *wp;
  register BUFFER *bp;
  register LINE *lp1;
  register LINE *lp2;
  int s;

  bp = curbp;				/* Cheap.               */
  if ((s = bclear (bp)) != TRUE)	/* Might be old.        */
    return (s);
  lp2 = firstline (bp);			/* Header line          */
#if	BACKUP
  bp->b_flag &= ~(BFCHG | BFBAK);	/* No change, backup.   */
#else
  bp->b_flag &= ~BFCHG;			/* No change.           */
#endif
  strcpy (bp->b_fname, fname);
  if ((s = ffropen (fname)) == FIOERR)	/* Hard file open.      */
    goto out;
  if (s == FIOFNF)
    {				/* File not found.      */
      if (kbdmop == NULL)
	eprintf ("[New file]");
      goto out;
    }
  if (!readlines (lp2, &s))
    {				/* Last line didn't have \n?    */
      lp1 = lastline(bp);	/* Delete empty last line	*/
      lp2 = lback(lp1);
      lp2->l_fp = lp1->l_fp;
      lp1->l_fp->l_bp = lp2;
      free (lp1);
    }
#if	BACKUP
  bp->b_flag |= BFBAK;		/* Need a backup.       */
#endif
out:
  lp1 = firstline (bp);		/* First line in buffer */
  ALLWIND (wp)
  {
    if (wp->w_bufp == bp)
      {
	wp->w_linep = lp1;
	wp->w_dot.p = lp1;
	wp->w_dot.o = 0;
	wp->w_mark.p = NULL;
	wp->w_mark.o = 0;
	wp->w_flag |= WFMODE | WFHARD;
      }
  }

  /* In case buffer isn't in any window, point dot to first line.
   */
  bp->b_dot.p = lp1;

  /* Set up the mode for this file.
   */
#if USE_RUBY
  /* BUG - if the current buffer is not in the current window,
   * which can happen when reading in files at startup,
   * we can't run the Ruby mode code, because it will try to manipulate
   * the current buffer, and that will cause window data structure
   * corruption.
   */
  if (curwp->w_bufp == curbp)
    {
      rubymode();
    }
#endif

  return (s != FIOERR);		/* False if error.      */
}

/*
 * Read lines from the open file, inserting them before lp2.  Return
 * TRUE if the last line read was terminated by a newline; return FALSE
 * otherwise.  The status returned from the file I/O routines (FIOSUC,
 * FIOERR, or FIOEOF) is returned to *statptr.
 */
int
readlines (
     LINE *lp2,		/* insert lines before this one */
     int *statptr)	/* return status                */
{
  register LINE *lp1;
  register int s;
  register int nline;
  int nbytes;
  char *line;

  nline = 0;
  eprintf ("[Reading...]");
  do
    {
      s = ffgetline (&line, &nbytes);	/* read next line       */
      if (s != FIOSUC && nbytes == 0)	/* True end-of-file?    */
	break;
      if ((lp1 = lallocx (nbytes)) == NULL)
	{
	  s = FIOERR;		/* Keep message on the  */
	  break;		/* display.             */
	}
      lp1->l_bp = lp2->l_bp;	/* Insert lp1           */
      lp1->l_fp = lp2;		/*  before lp2          */
      lp2->l_bp->l_fp = lp1;
      lp2->l_bp = lp1;
      lputs (lp1, line, nbytes);
      ++nline;
    }
  while (s == FIOSUC);		/* until error or EOF   */
  ffclose ();			/* Ignore errors.       */
  if (s == FIOEOF && kbdmop == NULL)
    {				/* Don't zap an error.  */
      if (nline == 1)
	eprintf ("[Read 1 line]");
      else
	eprintf ("[Read %d lines]", nline);
    }
  *statptr = s;			/* Return file I/O stat */
  return (nbytes == 0);		/* Last line had \n?    */
}

/*
 * Take a file name, and from it
 * fabricate a buffer name. This routine knows
 * about the syntax of file names on the target system.
 * BDC0		left scan delimiter.
 * BDC1		optional second left scan delimiter.
 * BDC2		optional third left scan delimiter.
 * BDC3		optional right scan delimiter.
 */
void
makename (char *bname, const char *fname)
{
  const char *cp1;
  char *cp2;

  cp1 = &fname[0];
  while (*cp1 != 0)
    ++cp1;
  while (cp1 != &fname[0]
#ifdef	BDC0
	 && cp1[-1] != BDC0
#endif
#ifdef	BDC1
	 && cp1[-1] != BDC1
#endif
#ifdef	BDC2
	 && cp1[-1] != BDC2
#endif
    )
    --cp1;
  cp2 = &bname[0];
  while (cp2 != &bname[NBUFN - 1] && *cp1 != 0
#ifdef	BDC3
	 && *cp1 != BDC3
#endif
    )
    *cp2++ = *cp1++;
  *cp2 = 0;
}

/*
 * Update the mode lines for all windows viewing the current buffer.
 */
void
updatemode (void)
{
  register EWINDOW *wp;

  ALLWIND (wp)			/* Update mode lines.   */
    if (wp->w_bufp == curbp)
      wp->w_flag |= WFMODE;
}


/*
 * This function expands tabs in a line of text, storing the resulting
 * text in a dynamically allocated buffer.  The address of the buffer
 * is returned.  The buffer is overwritten on each call to this function.
 * The expanded length is returned to the caller in *len.
 */
static char *
expand (const char *text, int *len)
{
  static char *buf;		/* Expansion buffer     */
  static int bufsize = 0;	/* Size of buffer       */
  int i;			/* Index to buffer      */
  char *newbuf;			/* New buffer           */
  int newsize;			/* New size of buffer   */
  int col;			/* Current column       */
  int ncols;			/* Cols used by 1 char  */
  int c;			/* current character    */
  int oldlen;			/* length of original   */

  for (col = 0, i = 0, oldlen = *len; oldlen; oldlen--)
    {
      if ((c = *text++ & 0xff) == '\t')
	ncols = tabsize - (col % tabsize);
      else if (CISCTRL (c) != FALSE)
	ncols = 2;
      else
	ncols = 1;
      if (i + ncols > bufsize)
	{			/* Time to grow buffer? */
	  newsize = bufsize + 80;	/* Grow it by 80 bytes  */
	  if (bufsize == 0)
	    newbuf = malloc (newsize);
	  else
	    newbuf = realloc (buf, newsize);
	  if (newbuf == NULL)
	    {
	      eprintf ("Can't allocate tab expansion buffer of %d bytes",
		       newsize);
	      return (NULL);
	    }
	  buf = newbuf;
	  bufsize = newsize;
	}
      col += ncols;
      if (c == '\t')
	while (ncols--)
	  buf[i++] = ' ';
      else
	buf[i++] = c;
    }
  *len = i;
  return (buf);
}

/*
 * This function performs the details of file
 * writing. Uses the file management routines in the
 * "fileio.c" package. The number of lines written is
 * displayed.  Most of the grief is error checking of some sort.
 */
static int
writeout (const char *fn)
{
  register int s;
  register LINE *lp;
  register LINE *fp;
  register int nline;
  register const char *buf;
  int llen;

  /* Check if the file has no terminating newline.  This is the
   * case if the last line in the file is not empty.
   */
  lp = lastline (curbp);	/* Last line.           */
  if (lp != curbp->b_linep && llength (lp) != 0 && kbdmop == NULL)
    {
      s = eyesno ("File doesn't end with a newline. Should I add one");
      if (s == ABORT)		/* Aborted.             */
	return (FALSE);
      if (s == TRUE)
	{			/* Add the blank line.  */
	  if ((fp = lallocx (0)) == NULL)
	    return (FALSE);
	  fp->l_fp = lp->l_fp;
	  fp->l_bp = lp;
	  lp->l_fp->l_bp = fp;
	  lp->l_fp = fp;
	}
    }

  eprintf ("[Writing...]");
  if ((s = ffwopen (fn)) != FIOSUC)	/* Open writes message. */
    return (FALSE);
  lp = firstline (curbp);		/* First line.          */
  nline = 0;				/* Number of lines.     */
  while (lp != curbp->b_linep)
    {
      llen = llength (lp);
      fp = lforw (lp);
      if (savetabs)			/* Preserving tabs?     */
	buf = (const char *) lgets (lp);	/* Use line as is.      */
      else /* Else expand tabs.        */
      if ((buf = expand ((const char *) lgets (lp), &llen)) == NULL)
	buf = (const char *) lgets (lp);

      if (fp == curbp->b_linep)
	{			/* Last line?           */
	  s = ffputline (buf, llen, FALSE);
	  if (llen != 0)	/* Line isn't blank?    */
	    ++nline;		/* Count it             */
	}
      else
	{			/* Not the last line    */
	  s = ffputline (buf, llen, TRUE);
	  ++nline;
	}
      if (s != FIOSUC)
	break;
      lp = fp;
    }
  if (s == FIOSUC)
    {				/* No write error.      */
      s = ffclose ();
      if (s == FIOSUC && kbdmop == NULL)
	{
	  if (nline == 1)
	    eprintf ("[Wrote 1 line]");
	  else
	    eprintf ("[Wrote %d lines]", nline);
	}
    }
  else				/* Ignore close error       */
    ffclose ();			/* if a write error.    */
  if (s != FIOSUC)		/* Some sort of error.  */
    return (FALSE);
  return (TRUE);
}

/*
 * Ask for a file name, and write the
 * contents of the current buffer to that file.
 * Update the remembered file name and clear the
 * buffer changed flag. This handling of file names
 * is different from the earlier versions, and
 * is more compatable with Gosling EMACS than
 * with ITS EMACS.
 */
int
filewrite (int f, int n, int k)
{
  register int s;
  char fname[NFILEN];
  char *expanded_fname;

  if ((s = egetfname ("Write file: ", fname, NFILEN)) != TRUE)
    return (s);
  adjustcase (fname);
  expanded_fname = fftilde (fname);
  if ((s = writeout (expanded_fname)) == TRUE)
    {
      strcpy (curbp->b_fname, expanded_fname);
      curbp->b_flag &= ~BFCHG;
      updatemode ();		/* Update mode lines.   */
    }
#if	BACKUP
  curbp->b_flag &= ~BFBAK;	/* No backup.           */
#endif
  return (s);
}

/*
 * Save the contents of the current buffer back into
 * its associated file. Do nothing if there have been no changes
 * (is this a bug, or a feature). Error if there is no remembered
 * file name. If this is the first write since the read or visit,
 * then a backup copy of the file is made.
 */
int
filesave (int f, int n, int k)
{
  register int s;

  if ((curbp->b_flag & BFCHG) == 0)	/* Return, no changes.  */
    return (TRUE);
  if (curbp->b_fname[0] == 0)
    {				/* Must have a name.    */
      eprintf ("No file name");
      return (FALSE);
    }
#if	BACKUP
  if (bflag == TRUE && (curbp->b_flag & BFBAK) != 0)
    {
      s = fbackupfile (curbp->b_fname);
      if (s == ABORT)		/* Hard error.          */
	return (s);
      if (s == FALSE		/* Softer error.        */
	  && (s = eyesno ("Backup error, save anyway")) != TRUE)
	return (s);
    }
#endif

  if ((s = writeout (curbp->b_fname)) == TRUE)
    {
      curbp->b_flag &= ~BFCHG;
      updatemode ();		/* Update mode lines.   */
    }
#if	BACKUP
  curbp->b_flag &= ~BFBAK;	/* No backup.           */
#endif
  setundochanged ();
  return (s);
}

/*
 * The command allows the user
 * to modify the file name associated with
 * the current buffer. It is like the "f" command
 * in UNIX "ed". The operation is simple; just zap
 * the name in the BUFFER structure, and mark the windows
 * as needing an update. You can type a blank line at the
 * prompt if you wish.
 */
int
filename (int f, int n, int k)
{
  register int s;
  char fname[NFILEN];
  char *expanded_fname;

  if ((s = egetfname ("Name: ", fname, NFILEN)) == ABORT)
    return (s);
  adjustcase (fname);
  expanded_fname = fftilde (fname);
  strcpy (curbp->b_fname, expanded_fname);	/* Fix name.            */
  updatemode ();		/* Update mode lines    */
#if	BACKUP
  curbp->b_flag &= ~BFBAK;	/* No backup.           */
#endif
  lchange (WFEDIT);
  return (TRUE);
}

/*
 * Set the savetabs flag according to the numeric argument if present,
 * or toggle the value if no argument present.  If savetabs is
 * zero, tabs will will be changed to spaces when saving a file, by
 * replacing each tab with the appropriate number of spaces (as
 * determined by tabsize).
 */
int
setsavetabs (int f, int n, int k)
{
  savetabs = f ? (n != 0) : !savetabs;
  eprintf ("[Tabs will %sbe preserved when saving a file]",
	   savetabs ? "" : "not ");
  return (TRUE);
}
