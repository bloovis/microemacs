/*
 * Name:	MicroEMACS
 *		MS-DOS file I/O.
 * Version:	29
 * Last edit:	5-Feb-88
 * By:		Mark Alexander
 *		drivax!alexande
 */
#include	"def.h"
#include	<fcntl.h>

char *getenv ();

#define	CTRLZ	0x1a		/* DOS end of file      */

static int ffp;			/* text file pointer    */
static char cbuf[1024];		/* buffer for getbyte   */
static int cindex;		/* index into cbuf      */
static int csize;		/* no. of bytes in cbuf */
static int status;		/* read(),write() status */
static int writing;		/* open for write?      */
static int longline;		/* file had long line   */

#define	getbyte() cindex == csize ? fillbuf() : cbuf[cindex++] & 0xff

/*
 * Macro for putting one byte back into the input file.  This only
 * works after getbyte(), and only one character can be put back.
 */
#define ungetbyte(c) cbuf[--cindex] = (c)

static int pfp;			/* profile descriptor   */
static char pbuf[1024];		/* buffer for profile   */
static int pindex;		/* index into pbuf      */
static int psize;		/* no. of bytes in pbuf */

/*
 * Open a file for reading.  Used for text files, not profile files.
 */
int
ffropen (const char *fn)
{
  longline = writing = FALSE;
  if ((ffp = open (fn, O_RAW)) < 0)
    return (FIOFNF);
  cindex = csize = 0;		/* set up for getbyte() */
  return (FIOSUC);
}

/*
 * Open a file for writing.
 * Return TRUE if all is well, and
 * FALSE on error (cannot create).
 */
int
ffwopen (const char *fn)
{
  if ((ffp = creat (fn, O_CREAT | O_TRUNC | O_RAW)) < 0)
    {
      eprintf ("Cannot open file for writing");
      return (FIOERR);
    }
  writing = TRUE;
  cindex = 0;
  return (FIOSUC);
}

/*
 * Close a file.  Append a ctrl-Z if file was open for write.
 * Should look at the status.
 */
ffclose ()
{
  char c;

  if (writing)
    {				/* open for write?      */
      if (zflag)
	putbyte ("\32", 1);	/* write a CTRLZ        */
      if (status >= 0)		/* write OK?            */
	status = write (ffp, cbuf, cindex);	/* flush output */
      close (ffp);
      if (status < cindex)
	{
	  eprintf ("File write error");
	  return (FIOERR);
	}
      else
	return (FIOSUC);
    }
  else
    {				/* open for read        */
      close (ffp);
      return (FIOSUC);		/* ignore errors        */
    }
}

/*
 * Write a line to the already
 * opened file. The "buf" points to the
 * buffer, and the "nbuf" is its length, less
 * the free newline. Return the status.
 * Check only at the newline.  The 'nl' parameter
 * says whether to terminate the line with a newline.
 */
int
ffputline (const char *buf, int nbuf, int nl)
{
  putbyte (buf, nbuf);
  if ((status >= 0) && nl)
    putbyte ("\r\n", 2);
  if (status < 0)
    {
      eprintf ("File write error");
      return (FIOERR);
    }
  return (FIOSUC);
}

/*
 * Read a line from a file, and store the bytes
 * in the supplied buffer. Stop on end of file or end of
 * line. Don't get upset by files that don't have an end of
 * line on the last line; this seem to be common on CP/M-86 and
 * MS-DOS (the suspected culprit is VAX/VMS kermit, but this
 * has not been confirmed. If this is sufficiently researched
 * it may be possible to pull this kludge). Delete any CR
 * followed by an LF. This is mainly for runoff documents,
 * both on VMS and on Ultrix (they get copied over from
 * VMS systems with DECnet).
 */
ffgetline (buf, nbuf)
     register char buf[];
{
  register int c;
  register int i;

  i = 0;
  for (;;)
    {
      c = getbyte ();

      /*  Delete carriage return if followed by line feed
       */
      if (c == '\r')
	{			/* carriage return?     */
	  c = getbyte ();	/* check next byte      */
	  if (c != '\n')
	    {			/* next not line feed?  */
	      ungetbyte (c);	/* put it back          */
	      c = '\r';		/* put cr into line     */
	    }
	}

      /*  Check for terminating character or end of file
       */
      if (c == CTRLZ && zflag)
	{			/* old eof character?   */
	  c = EOF;
	  break;
	}
      if (c == EOF || c == '\n')	/* end of line/file?    */
	break;

      /*  Chop line in pieces if it is too long, otherwise
       *  store the byte into the line and continue looping.
       */
      if (i >= nbuf - 1)
	{			/* line full?           */
	  ungetbyte (c);	/* put the byte back    */
	  eprintf ("File has long line");
	  longline = TRUE;
	  c = '\n';		/* fake a new line      */
	  break;
	}

      buf[i++] = c;		/* store byte in line   */
    }
  buf[i] = 0;
  if (c != '\n')
    {				/* End of file.         */
      if (longline)		/* file had long line?  */
	return (FIOERR);	/* report error         */
      else
	return (FIOEOF);	/* normal end of file   */
    }
  return (FIOSUC);
}

/*
 * Fill the input file buffer, return the first character from it.
 */
fillbuf ()
{
  if ((status = read (ffp, cbuf, sizeof (cbuf))) <= 0)
    {
      if (status < 0)
	eprintf ("File read error.");
      return (EOF);
    }
  cindex = 0;
  csize = status;
  return (cbuf[cindex++] & 0xff);
}


/*
 * Put a string of bytes to the output file.  Use our own buffering for speed.
 */
putbyte (s, len)
     char *s;
     int len;
{
  status = 0;
  while (len)
    {
      if (cindex == sizeof (cbuf))
	{			/* buffer full? */
	  if (write (ffp, cbuf, sizeof (cbuf)) < sizeof (cbuf))
	    status = -1;	/* disk full    */
	  cindex = 0;
	}
      cbuf[cindex++] = *s++;
      --len;
    }
}

#if	BACKUP

/*
 * Make a backup of the given file.  There are two strategies.
 * If no XBACKUP environment variable is defined, just change
 * the extension of the original file to .BAK.  Otherwise,
 * use the XBACKUP variable as a directory name, and try to
 * rename the original file into that directory.  We don't
 * handle the case where the backup directory is on a different
 * disk volume; we'd have to copy the whole file.
 */
int
fbackupfile (const char *fname)
{
  char nname[NFILEN];		/* new name                     */
  register char *p;
  register char *bname;		/* XBACKUP directory name       */
  register char lastc;		/* last character in XBACKUP    */

  /* If there is a XBACKUP environment variable defined, use it
   * as a prefix to the original filename.  Otherwise just
   * change the extension of the filename to .BAK
   */
  if ((bname = getenv ("XBACKUP")) == NULL)
    {
      strcpy (nname, fname);
      for (p = nname; *p && *p != '.'; p++)
	;			/* search for extension */
      strcpy (p, ".BAK");	/* use .BAK extension   */
    }
  else
    {
      strcpy (nname, bname);
      lastc = nname[strlen (nname) - 1];	/* get last character   */
      if (lastc != '\\' && lastc != '/')
	strcat (nname, "\\");
      strcat (nname, fname);
    }
  unlink (nname);		/* delete old backup    */
  return (rename (fname, nname) == 0);
}

#endif

/*
 * The string "fn" is a file name.
 * Perform any required case adjustments. All sustems
 * we deal with so far have case insensitive file systems.
 * We zap everything to lower case. The problem we are trying
 * to solve is getting 2 buffers holding the same file if
 * you visit one of them with the "caps lock" key down.
 * On UNIX file names are dual case, so we leave
 * everything alone.
 */
void
adjustcase (fn)
     register char *fn;
{
  register int c;

  while ((c = *fn) != 0)
    {
      if (c >= 'A' && c <= 'Z')
	*fn = c + 'a' - 'A';
      ++fn;
    }
}


/*
 * Open a profile file for reading.  We keep this separate from
 * ffropen() so that we can have a text file and a profile open
 * at the same time.  If the specified filename is NULL, open
 * the default profile.  The open must allow binary raw data
 * to be read from the file because we don't want CR characters
 * to be filtered out even in CR/LF combinations.
 */
ffpopen (fn)
     char *fn;
{
  static char newname[NFILEN];

  if (fn == NULL)
    {
      if ((pfp = open ("pe.pro", O_RAW)) >= 0)
	return (FIOSUC);
      if ((fn = getenv ("HOME")) != NULL)
	strcpy (newname, fn);
      else
	newname[0] = 0;
      strcat (newname, "\\pe.pro");
      fn = newname;
    }
  if ((pfp = open (fn, O_RAW)) < 0)
    return (FIOFNF);
  return (FIOSUC);
}

/*
 * Read the next byte from the profile file.  If end of file,
 * return FIOEOF.  If error, return FIOERR.
 * Otherwise, return the byte to the specified buffer, and
 * return FIOSUC.  Don't need to do buffering here because
 * profiles don't have to be efficient.
 */
ffpread (cp)
     char *cp;
{
  register int c;

  if (pindex == psize)		/* buffer exhausted     */
    {
      if ((status = read (pfp, pbuf, sizeof (pbuf))) <= 0)
	{
	  if (status < 0)
	    return (FIOERR);
	  else
	    return (FIOEOF);
	}
      pindex = 0;
      psize = status;
    }
  *cp = pbuf[pindex++] & 0xff;
  return (FIOSUC);
}


/*
 * Close the profile file.
 */

ffpclose ()
{
  close (pfp);
  return (FIOSUC);
}
