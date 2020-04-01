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
 * 		Ultrix-32 file I/O.
 * Version:	29
 * Last edit:	09-Feb-88
 * By:		Mark Alexander
 *		drivax!alexande
 */
#include	"def.h"

#ifdef __hpux
/* Need this kludge to get dirent.h to define DIR structure */
#define _INCLUDE_POSIX_SOURCE
#endif

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<dirent.h>
#include	<pwd.h>
#include	<unistd.h>

/*
 * External declarations.
 */
char *getenv ();

/*
 * Forward declarations.
 */
char *fftilde (char *filename);

static FILE *ffp, *pfp, *jfp;	/* text, profile, and journal files     */
static int longline;		/* true if file had long line           */

/* Buffer for ffgetline.  Dynamically allocated to handle any line length.
 */
static char *buf;		/* dynamic line buffer */
static int bufsize;		/* size of line buffer */

/*
 * Open a file for reading.
 */
int
ffropen (const char *fn)
{
  if ((ffp = fopen (fn, "r")) == NULL)
    return (FIOFNF);
  longline = FALSE;
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
  if ((ffp = fopen (fn, "w")) == NULL)
    {
      eprintf ("Cannot open file for writing");
      return (FIOERR);
    }
  return (FIOSUC);
}

/*
 * Close a file.
 * Should look at the status.
 */
int
ffclose (void)
{
  fclose (ffp);
  return (FIOSUC);
}

/*
 * Write a line to the already
 * opened file. The "buf" points to the
 * buffer, and the "nbuf" is its length, less
 * the free newline. Return the status.
 * Check only at the newline.
 */
int
ffputline (const char *buf, int nbuf, int nl)
{
  register int i;

  for (i = 0; i < nbuf; ++i)
    putc (buf[i] & 0xFF, ffp);
  if (ferror (ffp) == FALSE && nl)
    putc ('\n', ffp);
  if (ferror (ffp) != FALSE)
    {
      eprintf ("Write I/O error");
      return (FIOERR);
    }
  return (FIOSUC);
}

/*
 * Read a line from a file, and store the bytes
 * in a local buffer. Stop on end of file or end of
 * line. Don't get upset by files that don't have an end of
 * line on the last line; this seem to be common on CP/M-86 and
 * MS-DOS (the suspected culprit is VAX/VMS kermit, but this
 * has not been confirmed. If this is sufficiently researched
 * it may be possible to pull this kludge). Delete any CR
 * followed by an LF. This is mainly for runoff documents,
 * both on VMS and on Ultrix (they get copied over from
 * VMS systems with DECnet).  Return the address of the local
 * buffer to *buf, and the number of bytes read to *nbytes.
 */
int
ffgetline (char **bufp, int *nbytes)
{
  register int c;
  register int i;
  char *newbuf;
  int newbufsize;
  int merror;

  i = 0;
  merror = 0;
  for (;;)
    {
      c = getc (ffp);

      /*  Delete carriage return if followed by line feed
       */
      if (c == '\r')
	{			/* carriage return?     */
	  c = getc (ffp);	/* check next byte      */
	  if (c != '\n')
	    {			/* next not line feed?  */
	      ungetc (c, ffp);	/* put it back          */
	      c = '\r';		/* put cr into line     */
	    }
	}

      if (c == EOF || c == '\n')	/* end of line/file?    */
	break;

      /*  If the buffer is too small, enlarge it.
       */
      if (i >= bufsize)
	{			/* line full?           */
	  newbufsize = bufsize + 256;
	  if (bufsize != 0)
	    newbuf = realloc (buf, newbufsize);
	  else
	    newbuf = malloc (newbufsize);
	  if (!newbuf)
	    {
	      ungetc (c, ffp);	/* put char back        */
	      eprintf ("Out of memory, line split");
	      merror = 1;
	      break;
	    }
	  buf = newbuf;
	  bufsize = newbufsize;
	}
      buf[i++] = c;		/* store byte in line   */
    }
  *nbytes = i;			/* return no. of bytes  */
  *bufp = buf;			/* return buf pointer   */
  if (c != '\n')
    {				/* End of file.         */
      if (merror)		/* out of memory?       */
	return (FIOERR);	/* report error         */
      else
	return (FIOEOF);	/* normal end of file   */
    }
  return (FIOSUC);
}

#if BACKUP

/*
 * Rename the file "fname" into a backup copy.  There are two strategies.
 * If no XBACKUP environment variable is defined, the backup has the same
 * name as the original file, with a "~" on the end; this seems to be
 * newest of the new-speak.  Otherwise, use the XBACKUP variable as a
 * directory name, and try to rename the original file into that
 * directory.  The error handling is all in "file.c". The "unlink" is
 * perhaps not the right thing here; I don't care that much as I don't
 * enable backups myself.
 */
int
fbackupfile (const char *fname)
{
  char nname[NFILEN];		/* new name                     */
  register char *bname;		/* XBACKUP directory name       */

  /* If there is a XBACKUP environment variable defined, use it
   * as a prefix to the original filename.  Otherwise just
   * append "~" to the filename.
   */
  if ((bname = getenv ("XBACKUP")) == NULL)
    {
      strcpy (nname, fname);
      strcat (nname, "~");
    }
  else
    {
      strcpy (nname, bname);
      if (nname[strlen (nname) - 1] != '/')
	strcat (nname, "/");
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
adjustcase (char *fn)
{
#if	0
  register int c;

  while ((c = *fn) != 0)
    {
      if (c >= 'A' && c <= 'Z')
	*fn = c + 'a' - 'A';
      ++fn;
    }
#endif
}


/*
 * Open a profile file for reading.  We keep this separate from
 * ffropen() so that we can have a text file and a profile open
 * at the same time.  If the specified filename is NULL, open
 * the default profile.  The open must allow binary raw data
 * to be read from the file because we don't want CR characters
 * to be filtered out even in CR/LF combinations.
 */
int
ffpopen (char *fn)
{
  char newname[NFILEN];

  if (fn == NULL)
    {
      if ((pfp = fopen (".pepro", "r")) != NULL)
	return (FIOSUC);
      if ((fn = getenv ("HOME")) == NULL)
	return (FIOFNF);
      strcpy (newname, fn);
      strcat (newname, "/.pepro");
      fn = newname;
    }
  if ((pfp = fopen (fn, "r")) == NULL)
    return (FIOFNF);
  return (FIOSUC);
}

/*
 * Read the next byte from the profile file.  If end of file,
 * or control-Z read, or error, return FIOEOF.
 * Otherwise, return the byte to the specified buffer, and
 * return FIOSUC.  Don't need to do buffering here because
 * profiles don't have to be efficient.
 */
int
ffpread (char *cp)
{
  int c;

  if ((c = getc (pfp)) == EOF)
    return (FIOEOF);
  *cp = c;
  return (FIOSUC);
}


/*
 * Close the profile file.
 */
int
ffpclose (void)
{
  fclose (pfp);
  return (FIOSUC);
}

/*
 * Open a journal file for writing.
 * Return TRUE if all is well, and
 * FALSE on error (cannot create).
 */
int
ffjopen (void)
{
  if ((jfp = fopen (".pejou", "w")) == NULL)
    {
      eprintf ("Cannot open journal file for writing");
      return (FIOERR);
    }
  return (FIOSUC);
}

/*
 * Write a byte to the already opened
 * journal file. The "c" contains the byte to write.
 * Return the status.
 */
int
ffputjou (char c)
{
  putc (c, jfp);
  fflush (jfp);
  if (ferror (jfp) != FALSE)
    {
      eprintf ("Journal write error");
      return (FIOERR);
    }
  return (FIOSUC);
}

/*
 * Find the first or next file that matches the first 'cpos' characters
 * in the filename 'name'.  Returns a pointer to a static buffer
 * containing the matched filename.  This buffer is overwritten
 * on each call.  If 'prev' is NULL, this is the first search for
 * the file.
 */
char *
ffsearch (
     const char *name,		/* filename to search for */
     int cpos,			/* number of characters in name to match */
     const char *prev)		/* previous matching name */
{
  struct dirent *ff;
  static DIR *dirp;
  static char buf[NFILEN];
  static int pathlen;
  int i, c = 0;

  if (prev == NULL)		/* first time through   */
    {
      if (dirp != NULL)
	closedir (dirp);
      strncpy (buf, name, cpos);	/* save the name        */
      for (i = cpos; i > 0; --i)
	{			/* find end of path     */
	  c = buf[i - 1];
	  if (c == '/')
	    break;
	}
      pathlen = i;		/* save length of path  */
      if (pathlen == 0)		/* no path specified?   */
	dirp = opendir (".");	/* open current dir     */
      else
	{
	  buf[pathlen - 1] = '\0';	/* temporarily zap slash */
	  dirp = opendir (fftilde (buf));
	  /* open directory       */
	  buf[pathlen - 1] = c;	/* restore slash */
	}
      if (dirp == NULL)
	return (NULL);
    }
  while ((ff = readdir (dirp)) != NULL)	/* find next file       */
    {
      strcpy (&buf[pathlen], ff->d_name);	/* append filename    */
      if (strncmp (buf, name, cpos) == 0)	/* does it match?       */
	return (buf);		/* return static buffer */
    }
  closedir (dirp);
  dirp = NULL;
  return (NULL);		/* no more files        */
}

/*
 * Determine whether the first 'cpos' characters in the file 'name' refer
 * to a directory or not.  Return TRUE if it is a directory.
 */
int
ffisdir (
     const char *name,		/* filename to check */
     int cpos)			/* number of characters in name to check */
{
  static char fname[NFILEN];	/* temporary buffer */
  struct stat stbuf;
  int ret;

  strncpy (fname, name, sizeof (fname) - 1);
  if (cpos < NFILEN)		/* cpos isn't too big for buffer? */
    fname[cpos] = '\0';		/* null-terminate it    */
  ret = stat (fftilde (fname), &stbuf);	/* get file information */
  if (ret != 0)			/* file doesn't exist?  */
    return FALSE;		/* must not be a dir    */
  return (stbuf.st_mode & S_IFDIR) != 0;	/* check dir bit        */
}

/*
 * Expand a filename that has a leading ~ or ~username.  Return
 * a pointer to a static buffer containing the expanded filename.
 * This buffer will be overwritten by the next call.
 */
char *
fftilde (char *arg)
{
#ifdef MINGW
  return "";
#else
  char *user, *tail;
  struct passwd *pw;
  static char buf[1024];

  /* Isolate the user name (everything between the tilde and the
   * next slash).
   */
  if (arg[0] != '~')
    return arg;
  for (user = tail = &arg[1]; *tail != '\0'
#ifdef	BDC0
       && *tail != BDC0
#endif
#ifdef	BDC1
       && *tail != BDC1
#endif
#ifdef	BDC2
       && *tail != BDC2
#endif
       ; tail++)
    ;

  /* Get the home directory of the specified user.  If the user name
   * is empty, it's the current user.
   */
  if (user == tail)
    pw = getpwuid (getuid ());
  else
    {
      char c = *tail;
      *tail = '\0';
      pw = getpwnam (user);
      *tail = c;
    }
  if (!pw)
    return arg;

  /* Concatenate the home directory and the directory tail.
   */
  strcpy (buf, pw->pw_dir);
  strcat (buf, tail);
  return buf;
#endif
}

/*
 * Get the directory containing the currently running pe executable.
 */
const char *
ffexedir (void)
{
#ifdef MINGW
  return "";
#else
  int len;
  static char path[NFILEN];
  char *p;

  if ((len = readlink ("/proc/self/exe", path, sizeof (path) - 1)) != -1)
    {
      p = path + len;
      *p = '\0';
      while (p >= path)
        {
          --p;
          if (*p == '/')
            {
              *p = '\0';
              break;
            }
        }
    }
  else
    path[0] = '\0';
  return path;
#endif
}
