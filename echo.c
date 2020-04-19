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

/* $Header: /home/bloovis/cvsroot/pe/echo.c,v 1.3 2005-10-18 02:18:01 bloovis Exp $
 *
 * Name:	MicroEMACS
 *		Echo line reading and writing.
 * Version:	29
 *
 * Common routines for reading
 * and writing characters in the echo line area
 * of the display screen. Used by the entire
 * known universe.
 *
 * $Log: echo.c,v $
 * Revision 1.3  2005-10-18 02:18:01  bloovis
 * Rename some things to avoid conflict with ncurses.
 *
 * Revision 1.2  2004/04/20 15:18:19  bloovis
 * (egetfname): Use ereadv instead of passing NULL arg list
 * pointer to eread; this is necessary for compiling on x86_64.
 * (ereadv): New wrapper for eread that takes a variable number of args.
 *
 * Revision 1.1.1.1  2003/11/06 02:51:52  bloovis
 * Imported sources
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
 * Revision 1.8  1996/10/22 15:37:15  marka
 * Allow tab and Control-D to be used in autocompletion.
 * Autocomplete a directory name with '/', not space.
 *
 * Revision 1.7  1996/03/14 03:00:16  marka
 * Fix problem with mode line not being updated properly on Linux.
 *
 * Revision 1.6  91/06/29  08:18:26  alexande
 * Minor improvements to autocompletion.
 * Add feature from Epsilon: if control-S is entered when prompted for
 * a filename, the path of the current buffer is inserted.
 * 
 * Revision 1.5  91/04/19  23:17:03  alexande
 * Added support for autocompletion of buffer names, and popup
 * window showing choices for autocompletion.
 * 
 * Revision 1.4  91/02/14  16:44:38  alexande
 * Convert all VARARGS routines to use <stdarg.h> or <varargs.h>.
 * Fixed bug in nextname() that caused crash on SunOS.
 * 
 * Revision 1.3  91/02/06  09:13:08  alexande
 * Add egetfname() function, which allows automatic filename completion.
 * 
 * Revision 1.1  90/05/30  15:17:40  alexande
 * Initial revision
 * 
 *
 */
#include	"def.h"

int epresf = FALSE;		/* Stuff in echo line flag.     */
int enoecho = FALSE;		/* True if echo line disabled   */
int nmsg = 0;			/* Size of occupied msg. area.  */
int curmsgf = FALSE;		/* Current alert state.         */
int newmsgf = FALSE;		/* New alert state.             */

char msg[NMSG];			/* Random message storage.      */
char choicebuf[NCOL + 1];	/* Line buffer for displaying   */
				/*  autocompletion choices      */

/*
 * The reply queue contains strings to be returned by eread
 * instead of reading a string from the console.  The Ruby
 * extension places strings here so that commands can be
 * invoked with string parameters without prompting the user.
 */
#define NQUEUE 10
static char *replyq[NQUEUE];	/* queue of 10 strings		*/
static int replyq_index;	/* index of next string to get	*/
static int replyq_size;		/* index of next string to put	*/

/*
 * Special versions of the "tt" routines in ttyio.c that don't don't
 * do anything if the "noecho" variable is set.  This prevents echo
 * line activity from showing on the screen while we are
 * processing a profile.  "Noecho" is set to TRUE when a profile is
 * executed, and turned off temporarily by eprintf() to print error
 * messages (i.e. messages that don't start with '[').
 */
static void
ettmove (int row, int col)
{
  if (!enoecho)
    ttmove (row, col);
}

static void
ettputc (int c)
{
  if (!enoecho)
    ttputc (c);
}

static void
ettinsertc (int c)
{
  if (!enoecho)
    ttinsertc (c);
}

static void
ettdelc (void)
{
  if (!enoecho)
    ttdelc ();
}

static void
etteeol (void)
{
  if (!enoecho)
    tteeol ();
}

static void
ettflush (void)
{
  if (!enoecho)
    ttflush ();
}

static void
ettcolor (int color)
{
  if (!enoecho)
    ttcolor (color);
}

static void
ettbeep (void)
{
  if (!enoecho)
    ttbeep ();
}

/*
 * Send a string to the message system.
 * Add a free newline to the end of the message string.
 * Return TRUE if it fits, and FALSE if it does not.
 * Perhaps the message buffer should know how to get
 * larger, just like the kill buffer?
 */
int
writemsg (const char *sp)
{
  register int c;

  if (nmsg + strlen (sp) + 1 > NMSG)	/* "+1" for the "\n".   */
    return (FALSE);
  while ((c = *sp++) != '\0')
    msg[nmsg++] = c;
  msg[nmsg++] = '\n';
  newmsgf = TRUE;		/* Update mode line.    */
  return (TRUE);
}

/*
 * Read messages. The message lines are
 * displayed, one line at a time, in the message line.
 * A special sub-mode is entered, in which the keys have
 * the following meanings:
 *	^P	Go backward 1 line.
 *	BS	Go backward 1 line.
 *	^N	Go forward 1 line. Quit if at the end.
 *	SP	Go forward 1 line. Quit if at the end.
 *	CR	Go forward 1 line. Quit if at the end.
 *	^G	Abort, leave old text.
 *	^C	Quit, delete anything already read.
 * Return TRUE if you left this mode in a reasonable
 * way (not ^G), and ABORT if you quit the mode with a
 * ^G.
 */
int
readmsg (void)
{
  register int c;
  register int i;
  register int j;

  if (nmsg == 0)		/* Duck out if none.    */
    return (TRUE);
  newmsgf = FALSE;		/* Kill alert, and do   */
  update ();			/* a redisplay.         */
  ettcolor (CTEXT);
  i = 0;
  while (i < nmsg)
    {
      ettmove (nrow - 1, 0);	/* Display 1 line.      */
      while (i < nmsg && (c = msg[i++]) != '\n')
	eputc (c);
      etteeol ();
      ettmove (nrow - 1, 0);	/* Looks nice.          */
      ettflush ();
      for (;;)
	{			/* Editing loop.        */
	  c = getinp ();
	  switch (c)
	    {
	    case 0x0E:		/* ^N                   */
	    case 0x20:		/* SP                   */
	    case 0x0D:		/* CR                   */
	      break;

	    case 0x10:		/* ^P                   */
	    case 0x08:		/* BS                   */
	      do
		{
		  --i;
		}
	      while (i != 0 && msg[i - 1] != '\n');
	      if (i != 0)
		{
		  do
		    {		/* Back up 1 line.      */
		      --i;
		    }
		  while (i != 0 && msg[i - 1] != '\n');
		}
	      break;

	    case 0x03:		/* ^C                   */
	      j = 0;		/* Eat what we read.    */
	      while (i < nmsg)
		msg[j++] = msg[i++];
	      nmsg = j;
	      eerase ();
	      return (TRUE);

	    case 0x07:		/* ^G                   */
	      ettbeep ();
	      eerase ();
	      return (ABORT);

	    default:		/* Loop on the rest.    */
	      continue;
	    }
	  break;
	}
    }
  nmsg = 0;			/* Flow off the end.    */
  eerase ();
  return (TRUE);
}

/*
 * Prompt for a string, then echo it.  This is useful in
 * long profiles for showing progress.
 */

int
eecho (void)
{
  register int s;
  char echoline[NCOL];
  char oldecho;

  if ((s = ereply ("Echo: ", echoline, NCOL)) != TRUE)
    return (s);
  oldecho = enoecho;		/* save noecho flag     */
  enoecho = FALSE;		/* enable echoing       */
  ettcolor (CTEXT);
  ettmove (nrow - 1, 0);
  eputs (echoline);
  etteeol ();
  ettflush ();
  epresf = TRUE;
  enoecho = oldecho;		/* restore noecho flag  */
  return (TRUE);
}

/*
 * Erase the echo line.
 */
void
eerase (void)
{
  if (enoecho == FALSE)
    {
      ettcolor (CTEXT);
      ettmove (nrow - 1, 0);
      etteeol ();
      ettflush ();
      epresf = FALSE;
    }
}

/*
 * Ask "yes" or "no" question.
 * Return ABORT if the user answers the question
 * with the abort ("^G") character. Return FALSE
 * for "no" and TRUE for "yes". No formatting
 * services are available.
 */
int
eyesno (const char *sp)
{
  register int s;
  char buf[64];

  for (;;)
    {
      s = ereply ("%s [y/n]? ", buf, sizeof (buf), sp);
      if (s == ABORT)
	return (ABORT);
      if (s != FALSE)
	{
	  if (buf[0] == 'y' || buf[0] == 'Y')
	    return (TRUE);
	  if (buf[0] == 'n' || buf[0] == 'N')
	    return (FALSE);
	}
    }
}

/*
 * Write out a prompt, and read back a
 * reply. The prompt is now written out with full "eprintf"
 * formatting, although the arguments are in a rather strange
 * place. This is always a new message, there is no auto
 * completion, and the return is echoed as such.
 *
 * The arguments are as follows:
 *	format string
 *	buffer pointer
 *	size of buffer
 *	[optional arguments]...
 */
int
ereply (const char *fp, char *buf, int nbuf, ...)
{
  va_list ap;
  int ret;

  va_start (ap, nbuf);
  ret = eread (fp, buf, nbuf, EFNEW | EFCR, ap);
  va_end (ap);
  return (ret);
}

/*
 * Write out a prompt, and read back a
 * reply. The prompt is now written out with full "eprintf"
 * formatting, although the arguments are in a rather strange
 * place. This is always a new message, there is no auto
 * completion, and the return is echoed as such.
 *
 * The arguments are as follows:
 *	format string
 *	buffer pointer
 *	size of buffer
 *	flags
 *	[optional arguments]...
 */
int
ereplyf (const char *fp, char *buf, int nbuf, int flag, ...)
{
  va_list ap;
  int ret;

  va_start (ap, flag);
  ret = eread (fp, buf, nbuf, flag, ap);
  va_end (ap);
  return (ret);
}

/*
 * Write out a prompt, and read back a filename.  Autocompletion
 * of the filename is requested by hitting a space.
 */
int
egetfname (const char *fp, char *buf, int nbuf)
{
  return ereadv (fp, buf, nbuf, EFNEW | EFCR | EFFILE);
}

/*
 * Start creating a visible list of choices for autocompletion,
 * using the pop-up buffer that's also used for displaying
 * the buffer list and the key bindings.  Ignore errors,
 * since we want the user to be able to autocomplete a command
 * even if there isn't enough room to create the pop-up buffer.
 */
void
startchoices ()
{
  if (bclear (blistp) != TRUE)	/* Clear it out.        */
    return;
  strcpy (blistp->b_fname, "");
  choicebuf[0] = '\0';
}

/*
 * Add an item to the choice list pop-up buffer.  Multiple items
 * are placed on a line, separated into columns with 16-character
 * spacing.
 */
void
addchoice (const
     char *name,
     int flag)
{
  int len1, len2, pad;
  char bname[NBUFN];

  if (choicebuf[0] == '\0')
    {				/* This is the first?   */
      addline ("List of choices");	/* Put a pretty header  */
      addline ("---- -- -------");	/*  at the start        */
    }
  if (flag & EFFILE)
    {				/* Filename?            */
      makename (bname, name);	/* Strip out the path   */
      if (flag & EFDIR)
	strcat (bname, "/");
      name = bname;
    }
  len1 = strlen (choicebuf);
  len2 = strlen (name);
  if (flag & EFAUTO)		/* If command name      */
    pad = 1;			/* Pad only one space   */
  else
    pad = 16 - ((len1 + len2) & 15);	/* Pad up to 16 spaces  */
  if (len1 + len2 + pad > ncol)
    {				/* Line too long?       */
      addline (choicebuf);	/* Add it to buffer     */
      choicebuf[0] = '\0';	/* Clear the buffer     */
      len1 = 0;
    }
  strcat (choicebuf, name);	/* Add name to line     */
  strcat (choicebuf, "                " + 16 - pad);	/* Pad it out   */
}

/*
 * Pop up the choice list onto the screen.  If nothing was put into
 * the choice list, and the  choice list isn't already being displayed,
 * don't bother displaying it.
 */
void
showchoices ()
{
  int row, col;

  if (choicebuf[0] == '\0')
    {				/* no choices?          */
      if (blistp->b_nwnd == 0)	/* not being displayed? */
	return;			/* don't pop up choices */
      addline ("(no choices)");	/* display empty list   */
    }
  if (addline (choicebuf) == FALSE)
    return;
  popblist ();			/* pop up the buffer    */
  row = ttrow;			/* save cursor position */
  col = ttcol;
  update ();			/* update the screen    */
  ettmove (row, col);		/* restore cursor       */
  ettcolor (CTEXT);
  ettflush ();
}

/*
 * The "sp1" and "sp2" point to two command or file names.
 * The "cpos" is a horizontal position in the name. Return
 * the longest block of characters that can be autocompleted
 * at this point. Sometimes the two symbols are the same, but
 * this is normal.
 */
static int
getxtra (const char *sp1, const char *sp2, int cpos)
{
  register int i;

  i = cpos;
  for (;;)
    {
      if (sp1[i] != sp2[i])
	break;
      if (sp1[i] == '\0')
	break;
      ++i;
    }
  return (i - cpos);
}

/*
 * Find the next matching file or command name.
 */
const static char *
nextname (
     const char *buf,		/* name to match */
     int cpos,			/* no. of characters to match */
     const char *prev,		/* previous match (NULL if first) */
     int flag)			/* EFAUTO, EFFILE, or EFBUF */
{
  if (flag & EFAUTO)		/* autocomplete a command name  */
    return (symsearch (buf, cpos, prev));
  else if (flag & EFFILE)	/* autocomplete a filename      */
    return (ffsearch (buf, cpos, prev));
  else if (flag & EFBUF)	/* autocomplete a buffer name   */
    return (bufsearch (buf, cpos, prev));
  else				/* weird flag value         */
    return (NULL);
}

/*
 * Add another string to the reply queue.  A copy is made of the string
 * so that the caller's string, which is owned by the Ruby extension,
 * can be freed independently.  Return TRUE if success, FALSE otherwise
 * (e.g., if the queue is full).
 */
int
replyq_put (const char *s)
{
  char *copy;

  if (replyq_size == NQUEUE)
    {
      eprintf ("Reply queue is full!");
      return FALSE;
    }
  copy = strdup (s);
  replyq[replyq_size] = copy;
  ++replyq_size;
  return TRUE;
}

/*
 * Clear the reply queue.
 */
void
replyq_clear (void)
{
  int i;

  for (i = replyq_index; i < replyq_size; i++)
    free (replyq[i]);
  replyq_size = replyq_index = 0;
}

/*
 * Get the next string from the reply queue, and store it
 * in the specified buffer.  Return a pointer to the buffer
 * if success, or NULL if the reply queue was empty.
 */
static
char * replyq_get (char *buf, int nbuf)
{
  char *s;

  if (replyq_index == replyq_size)
    return NULL;
  s = replyq[replyq_index];
  ++replyq_index;
  strncpy (buf, s, nbuf);
  free (s);
  return buf;
}

/*
 * Move the physical cursor to a new column in the
 * echo line, based on old and new offsets into the line
 * currently being entered.
 */
static void
setcolumn (const char *buf, int oldcpos, int newcpos)
{
  int i = 0;
  int col = 0;
  int oldcol = 0;
  int newcol = 0;

  while (TRUE)
    {
      if (i == oldcpos)
	oldcol = col;
      if (i == newcpos)
	newcol = col;
      if (i >= oldcpos && i >= newcpos)
	break;
      ++col;
      if (CISCTRL (buf[i]))
	++col;
      ++i;
    }
  ttmove (ttrow, ttcol + newcol - oldcol);
}

/*
 * This is the general "read input from the
 * echo line" routine. The basic idea is that the prompt
 * string "prompt" is written to the echo line, and a one
 * line reply is read back into the supplied "buf" (with
 * maximum length "len"). The "flag" contains EFNEW (a
 * new prompt), EFAUTO (autocomplete), EFFILE (filename
 * autocomplete), or EFCR (echo the carriage return as CR).
 */
int
eread (const char *fp, char *buf, int nbuf, int flag, va_list ap)
{
  register int cpos;
  int buflen;
  const char *np1;
  register char *np2;
  char *cp0, *cp1;
  register int i;
  register int c;
  register int nhits;
  register int nxtra;
  register int bxtra;
  char savebuf[NFILEN];
  uchar ubuf[6];
  int ulen;

  if (replyq_get (buf, nbuf) != NULL)
    return TRUE;
  cpos = buflen = 0;
  if (kbdmop != NULL)
    {				/* In a macro.          */
      while ((c = *kbdmop++) != '\0' && cpos < nbuf - 1)
	buf[cpos++] = c;
      buf[cpos] = '\0';
      goto done;
    }
  if ((flag & EFNEW) != 0 || ttrow != nrow - 1)
    {
      ettcolor (CTEXT);		/* Normal video         */
      ettmove (nrow - 1, 0);	/* move to echo line    */
      epresf = TRUE;
    }
  else
    eputc (' ');
  eformat (fp, ap);
  etteeol ();
  ettflush ();
  for (;;)
    {
      c = getinp ();
      /* Space or tab means try to complete, ? or ^D means
       * pop up a list of choices.
       */
      if ((c == ' ' || c == '\t' || c == '?' || c == '\004')
	  && (flag & (EFAUTO | EFFILE | EFBUF)) != 0)
	{
	  int popup;

	  if ((popup = (c == '?' || c == '\004')) != FALSE)
	    startchoices ();	/* start choice list    */
	  nhits = 0;
	  nxtra = HUGE;
	  np2 = NULL;
	  while ((np1 = nextname (buf, cpos, np2, flag)) != NULL)
	    {
	      int dirflag = 0;

	      if ((flag & EFFILE) != 0 && ffisdir (np1, strlen (np1)))
		dirflag = EFDIR;
	      if (popup)	/* add to list  */
		addchoice (np1, flag | dirflag);
	      if (nhits++ == 0)
		{
		  strcpy (savebuf, np1);
		  np2 = savebuf;
		}
	      if ((bxtra = getxtra (np2, np1, cpos)) < nxtra)
		nxtra = bxtra;
	    }
	  if (nhits == 0)
	    {			/* No completion.       */
	      if (popup)
		showchoices ();
	      else
		ettbeep ();	/* Ring bell    */
	      continue;
	    }
	  for (i = 0; i < nxtra && cpos < nbuf - 1; ++i)
	    {
	      c = np2[cpos];
	      memmove (&buf[cpos + 1], &buf[cpos], buflen - cpos);
	      buf[cpos++] = c;
	      ++buflen;
	      einsertc (c);
	    }
	  ettflush ();

	  /* Don't fake a carriage return if there's more
	   * than one choice.
	   */
	  if (nhits != 1)
	    {
	      if (popup)	/* Display choices      */
		showchoices ();
	      continue;
	    }
	  if ((flag & EFFILE) != 0)
	    /* Fake a space if ordinary file,
	     * or a slash if directory.
	     */
	    c = ffisdir (buf, cpos) ? '/' : ' ';
	  else
	    c = 0x0D;		/* Fake a CR            */
	}
      switch (c)
	{
        case 0x01:		/* Control-A, go to start */
	  setcolumn (buf, cpos, 0);
	  cpos = 0;
	  ettflush ();
	  break;

	case 0x02:		/* Control-B, move back */
	  if (cpos > 0)
	    {
	      setcolumn (buf, cpos, cpos - 1);
	      --cpos;
	      ettflush ();
	    }
	  break;

	case 0x05:		/* Control-E, go to end	*/
	  setcolumn (buf, cpos, buflen);
	  cpos = buflen;
	  ettflush ();
	  break;

	case 0x06:		/* Control-F, move forward */
	  if (cpos < buflen)
	    {
	      setcolumn (buf, cpos, cpos + 1);
	      ++cpos;
	      ettflush ();
	    }
	  break;

	case 0x0D:		/* Return, done.        */
	  if ((flag & EFFILE) != 0)
	    while (buflen > 0 && buf[buflen - 1] == ' ')
	      buflen--;		/* Zap trailing spaces  */
	  buf[buflen] = '\0';
	  if (kbdmip != NULL)
	    {
	      if (kbdmip + cpos + 1 > &kbdm[NKBDM - 3])
		{
		  (void) ctrlg (FALSE, 0, KRANDOM);
		  ettflush ();
		  return (ABORT);
		}
	      for (i = 0; i < cpos; ++i)
		*kbdmip++ = buf[i];
	      *kbdmip++ = '\0';
	    }
	  if ((flag & EFCR) != 0)
	    {
	      ettputc (0x0D);
	      ettflush ();
	    }
	  goto done;

	case 0x07:		/* Bell, abort.         */
	  eputc (0x07);
	  (void) ctrlg (FALSE, 0, KRANDOM);
	  ettflush ();
	  return (ABORT);

	case 0x7F:		/* Rubout, erase.       */
	case 0x08:		/* Backspace, erase.    */
	  if (cpos == 0)
	    break;
	  setcolumn (buf, cpos, cpos - 1);
	  --cpos;
	  /* drop into Control-D */
	case 0x04:		/* Control-D, delete. */
	  if (cpos == buflen)
	    break;
	  ettdelc ();
	  if (CISCTRL (buf[cpos]))
	    ettdelc ();
	  memmove (&buf[cpos], &buf[cpos + 1], buflen - cpos);
	  --buflen;
	  ettflush ();
	  break;

	case 0x13:		/* Control-S, insert path or pattern */
	  cp0 = NULL;
	  if (flag & EFFILE)
	    {
	      cp0 = cp1 = &curbp->b_fname[0];
	      cp1 += strlen (cp1);
	      while (cp1 != cp0
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
	    }
	  else if (flag & EFPAT)
	    {
	      cp0 = (char *)pat;
	      cp1 = cp0 + strlen (cp0);
	    }
	  if (cp0 != NULL)
	    {
	      while (cp0 != cp1 && cpos < nbuf - 1)
		{
		  memmove (&buf[cpos + 1], &buf[cpos], buflen - cpos);
		  eputc (buf[cpos++] = *cp0++);
		  ++buflen;
		}
	      ettflush ();
	    }
	  break;

	case 0x15:		/* C-U, kill line.      */
	  if (buflen == 0)
	    break;
	  setcolumn (buf, cpos, 0);
	  etteeol ();
	  cpos = buflen = 0;
	  ettflush ();
	  break;

	case 0x0b:		/* C-K, kill to end of line */
	  if (cpos == buflen)
	    break;
	  etteeol ();
	  buflen = cpos;
	  ettflush ();
	  break;

	case 0x11:		/* Control-Q - quote    */
	  c = getinp ();	/* drop into default    */
	default:		/* All the rest.        */
	  ulen = uputc (c, ubuf);
	  if (cpos + ulen < nbuf)
	    {
	      memmove (&buf[cpos + ulen], &buf[cpos], buflen - cpos);
	      memcpy (&buf[cpos], ubuf, ulen);
	      cpos += ulen;
	      buflen += ulen;
	      einsertc (c);
	      ettflush ();
	    }
	}
    }
done:
  if (buf[0] == '\0')
    return (FALSE);
  return (TRUE);
}

/*
 * An interface to eread that allows it to be called with
 * a variable number of arguments after flag (to be formatted using
 * the format string fp)
 */
int
ereadv (const char *fp, char *buf, int nbuf, int flag, ...)
{
  va_list ap;
  int ret;

  va_start (ap, flag);
  ret = eread (fp, buf, nbuf, flag, ap);
  va_end (ap);
  return ret;
}

/*
 * Special "printf" for the echo line.
 * Each call to "eprintf" starts a new line in the
 * echo area, and ends with an erase to end of the
 * echo line. The formatting is done by a call
 * to the standard formatting routine.
 */
void
eprintf (const char *fp, ...)
{
  va_list ap;
  char oldecho;

  va_start (ap, fp);

  if (enoecho && *fp == '[')
    {				/* echo disabled, and   */
      va_end (ap);
      return;			/*  it's not an error   */
    }
  oldecho = enoecho;		/* save noecho flag     */
  enoecho = FALSE;		/* enable echoing       */
  ettcolor (CTEXT);
  ettmove (nrow - 1, 0);
  eformat (fp, ap);
  va_end (ap);
  etteeol ();
  ettflush ();
  epresf = TRUE;
  enoecho = oldecho;		/* restore noecho flag  */
}

static char digit[] = "0123456789ABCDEF";
/*
 * Put integer, in radix "r".
 */
static void
eputi (unsigned int i, unsigned int r)
{
  register unsigned int q, rem;

  if (r == 16)
    {
      q = i >> 4;
      rem = i & 0x0f;
    }
  else
    {
      q = i / r;
      rem = i % r;
    }
  if (q != 0)
    eputi (q, r);
  eputc (digit[rem]);
}

/*
 * Put long integer, in radix "r".
 */
static void
eputl (long i, int r)
{
  register long q, rem;

  if (r == 16)
    {
      q = (i >> 4) & 0x0fffffffL;
      rem = i & 0x0f;
    }
  else
    {
      q = i / r;
      rem = i % r;
    }
  if (q != 0)
    eputl (q, r);
  eputc (digit[rem]);
}

/*
 * Printf style formatting. This is
 * called by both "eprintf" and "ereply", to provide
 * formatting services to their clients. The move to the
 * start of the echo line, and the erase to the end of
 * the echo line, is done by the caller.
 */
void
eformat (const char *fp, va_list ap)
{
  register int c;

  while ((c = *fp++) != '\0')
    {
      if (c != '%')
	eputc (c);
      else
	{
	  c = *fp++;
	  switch (c)
	    {
	    case 'd':
	      eputi (va_arg (ap, int), 10);
	      break;

	    case 'l':
	      eputl (va_arg (ap, long), 10);
	      break;

	    case 'x':
	      eputi (va_arg (ap, int), 16);
	      break;

	    case 'X':
	      eputl (va_arg (ap, long), 16);
	      break;

	    case 'o':
	      eputi (va_arg (ap, int), 8);
	      break;

	    case 's':
	      eputs (va_arg (ap, char *));
	      break;

	    default:
	      eputc (c);
	    }
	}
    }
}

/*
 * Put string.
 */
void
eputs (const char *s)
{
  const char * end;
  int len;
  wchar_t c;

  end = s + strlen (s);
  while (s < end)
    {
      c = ugetc ((const uchar *) s, 0, &len);
      eputc (c);
      s += len;
    }
}

/*
 * Put character. Watch for
 * control characters, and for the line
 * getting too long.
 */
void
eputc (int c)
{
  if (ttcol < ncol)
    {
      if (CISCTRL (c) != FALSE)
	{
	  eputc ('^');
	  c ^= 0x40;
	}
      ettputc (c);
      ++ttcol;
    }
}

/*
 * Insert a character and move the cursor past it. Watch for
 * control characters, and for the line getting too long.
 */
void
einsertc (int c)
{
  if (ttcol < ncol)
    {
      if (CISCTRL (c) != FALSE)
	{
	  einsertc ('^');
	  c ^= 0x40;
	}
      ettinsertc (c);
      ++ttcol;
    }
  ettmove (ttrow, ttcol);
}
