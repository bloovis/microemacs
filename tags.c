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

/* $Header: /exit14/home/marka/tools/pe/RCS/tags.c,v 1.1 2007/01/17 20:51:17 marka Exp marka $
 *
 * Name:	MicroEMACS
 * 		Tagscommands.
 * Version:	1
 * By:		marka@pepper.com (Mark Alexander)
 *
 * $Log: tags.c,v $
 * Revision 1.7  2001/02/28 21:07:40  malexander
 * * def.h (POS): New structure for holding line position, which replaces
 * dot and mark variable pairs everywhere.
 *
 * Revision 1.6  2001/01/09 01:26:10  malexander
 * (inwordpos): Don't treat single quote as a word character.
 *
 * Revision 1.5  2000/09/29 00:19:38  malexander
 * Numerous changes to eliminate warnings and add prototypes.
 *
 * Revision 1.4  2000/07/26 01:50:00  malexander
 * (in_word, getcursorword): New functions for getting the word
 * under the cursor.
 * (searchtag): Use the word under the cursor as the default search string.
 *
 * Revision 1.3  2000/07/25 20:02:20  malexander
 * Restructure things to allow code sharing with cscope.c.
 *
 * Revision 1.2  2000/07/21 16:20:32  malexander
 * Reformatted with GNU indent.
 *
 * Revision 1.1.1.1  2000/07/14 19:23:11  malexander
 * Imported sources
 *
 * Revision 1.2  1996/10/22 15:39:40  marka
 * Handle macro definitions correctly.  Improve error checking.
 *
 * Revision 1.1  1996/04/09 00:28:12  marka
 * Initial revision
 *
 */
#include	"def.h"
#include	<string.h>
#include	<unistd.h>

#define DEBUG 0

#if DEBUG
FILE *report;
#define dprintf(x) fprintf x
#else
#define dprintf(x)
#endif

/*
 * Global variables
 */
tagfile tagfilelist = {		/* list of tag files            */
  "",				/* dummy filename               */
  &tagfilelist,			/* head pointer                 */
  &tagfilelist			/* tail pointer                 */
};

tagref tagreflist = {		/* list of all tags             */
  "",				/* dummy tag string             */
  0,				/* line number                  */
  0,				/* offset                       */
  0,				/* exact			*/
  &tagreflist,			/* head pointer                 */
  &tagreflist,			/* tail pointer                 */
  NULL				/* file pointer                 */
};

char tagpat[NPAT];		/* tag pattern to search        */

static int havetags;		/* true if tags file was read	*/

/*
 * Free up the memory associated with the current
 * tag list.
 */
int
freetags (int f, int n, int k)
{
  tagfile *curfile, *nextfile;
  tagref *curref, *nextref;

  /* Free the tag file list. */
  for (curfile = tagfilelist.next; curfile != &tagfilelist; curfile = nextfile)
    {
      nextfile = curfile->next;
      free (curfile->fname);
      free (curfile);
    }
  tagfilelist.next = tagfilelist.prev = &tagfilelist;

  /* Free the tag reference list. */
  for (curref = tagreflist.next; curref != &tagreflist; curref = nextref)
    {
      nextref = curref->next;
      free (curref->string);
      free (curref);
    }
  tagreflist.next = tagreflist.prev = &tagreflist;
  havetags = FALSE;
  return TRUE;
}

/*
 * Add a filename to the end of the filename list, return a
 * pointer to the next file structure, or NULL if out of memory.
 */
tagfile *
addtagfile (const char *name)
{
  tagfile *newfile;
  int len = strlen (name) + 1;

  if ((newfile = malloc (sizeof (tagfile))) == NULL)
    return NULL;

  if ((newfile->fname = malloc (len)) == NULL)
    return NULL;
  strcpy (newfile->fname, name);

  /* Add this file to the end of the file list.
   */
  newfile->next = &tagfilelist;
  newfile->prev = tagfilelist.prev;
  tagfilelist.prev = newfile->prev->next = newfile;

  return newfile;
}

/*
 * Search the filename list for the specified filename, and add it
 * to the list if not found.  Return a pointer to the filename structure,
 * or NULL if out of memory.
 */
tagfile *
findtagfile (const char *name)
{
  tagfile *f;

  for (f = tagfilelist.next; f != &tagfilelist; f = f->next)
    if (strcmp (name, f->fname) == 0)
      return f;
  return addtagfile (name);
}

/*
 * Add a new tag reference to the list, return a pointer to the
 * newly allocated tag structure.
 */
tagref *
addtagref (const char *string, tagfile *file, int line, long offset,
	   int exact)
{
  int len = strlen (string) + 1;
  tagref *newref, *prev, *next;

  /* Make sure there aren't any duplicates in the list.
   */
  for (next = tagreflist.next;
       next != &tagreflist;
       next = next->next)
    {
      if (file == next->file &&
	  line == next->line)
	{
	  eprintf ("Duplicated tag %s:%d", file->fname, line);
	  return next;
	}
    }

  /* Allocate space for the tag reference and
   * the string, and make a copy of the string.
   */
  if ((newref = malloc (sizeof (tagref))) == NULL)
    return NULL;
  if ((newref->string = malloc (len + 1)) == NULL)
    {
      free (newref);
      return NULL;
    }
  strcpy (newref->string, string);

  /* Fill in the rest of the fields.
   */
  newref->line   = line;
  newref->offset = offset;
  newref->file   = file;
  newref->exact  = exact;

  /* If exact is true, add it to the beginning of the list after any other
   * exact matches; otherwise append to the tail of the list.
   */
  if (exact)
    {
      for (prev = &tagreflist, next = tagreflist.next;
	   next->exact != 0;
	   prev = next, next = next->next)
	;
      newref->next = next;
      newref->prev = prev;
      prev->next = next->prev = newref;
    }
  else      
    {
      newref->next = &tagreflist;
      newref->prev = tagreflist.prev;
      tagreflist.prev = newref->prev->next = newref;
    }

  return newref;
}


/*
 * Read a tag file, parse into our internal format,
 * store in taglist.  Return false if error,
 * or true if successful.
 */
static int
readtagfile (char *fname)
{
  int		s;
  char *	line;
  int		nbytes;
  int		istagline;
  char *	eol;
  tagfile *	curfile = NULL;
  tagref *	newref;
  char *	tmp;
  int		lineno;
  long		offset;

  /* Open the TAGS file.
   */
  if ((s = ffropen (fname)) != FIOSUC)
    {				/* can't open tag file? */
      eprintf ("Unable to open tag file %s", fname);
      return FALSE;
    }

  /* Get rid of existing tags.
   */
  freetags (FALSE, 1, KRANDOM);

  /* Parse the tags file, store it in internal format, in
   * lists of files and references.
   */
  istagline = TRUE;
  do
    {
      s = ffgetline (&line, &nbytes);	/* read next line       */
      if (s != FIOSUC && nbytes == 0)	/* True end-of-file?    */
	break;
      line[nbytes] = '\0';	/* terminate the line   */

      /* A line consisting of a form feed indicates
       * the start of the tags for a new file.  The following
       * line will give the filename.
       */
      if (line[0] == '\f')
	{			/* form feed?           */
	  istagline = FALSE;	/* next line is filename */
	  continue;
	}
      if (istagline)
	{			/* this is a tag line   */

	  /* There must have been a previous file line.
	   */
	  if (curfile == NULL)
	    {
	      eprintf ("Reference without file");
	      break;
	    }

	  /* The tag string is terminated by a rubout.
	   */
	  if ((eol = strchr (line, 0x7f)) == NULL)
	    {
	      eprintf ("Tag line missing 0x7f");
	      s = FIOERR;
	      break;
	    }
	  *eol++ = '\0';	/* skip over rubout, terminate string */

	  /* The tag string is followed by the line number
	   * and file offset.  However, definitions of macros
	   * that take arguments are followed by the non-
	   * parenthesized macro name terminated by a Control-A.
	   * For now, ignore this and skip past the Control-A.
	   */
	  if ((tmp = strchr (eol, '\001')) != NULL)
	    eol = tmp + 1;	/* skip past control-A */
	  lineno = strtol (eol, &eol, 10);
	  if (lineno == 0 || *eol != ',')
	    {
	      eprintf ("Badly formed line number for %s", line);
	      /* s = FIOERR; */
	      /* break; */
	      continue;
	    }
	  eol++;		/* skip over comma */
	  offset = strtol (eol, NULL, 10);

	  /* Add this to the end of the list of references.
	   */
	  if ((newref = addtagref (line, curfile, lineno, offset, 0)) == NULL)
	    {
	      eprintf ("Unable to allocate tagref");
	      s = FIOERR;
	      break;
	    }
	}
      else
	{
	  /* This is a file line, not a tag line, so must create new
	  * file record. Filename is followed by a comma and the
	   * number of characters to skip to get
	   * to the next control-L.  Make a copy of the
	   * filename but ignore the number of characters.
	   */
	  if ((eol = strrchr (line, ',')) == NULL)
	    {
	      eprintf ("Filename not followed by comma");
	      s = FIOERR;
	      break;
	    }
	  *eol = '\0';		/* null-terminate the filename */

	  if ((curfile = addtagfile (line)) == NULL)
	    {
	      eprintf ("Unable to allocate file struct");
	      s = FIOERR;
	      break;
	    }

	  istagline = TRUE;	/* next line is a tag line */
	}

    }
  while (s == FIOSUC);		/* until error or EOF   */
  ffclose ();			/* Ignore errors.       */
  havetags = s != FIOERR;	/* False if error.      */
  return havetags;
}


/*
 * Read the tags file if the tag list is empty.
 */
static int
preptag (const char *string)
{
  if (!havetags)
    return readtagfile ("TAGS");
  else
    return TRUE;
}

/*
 * Return TRUE if the character at the specified offset and line
 * is a character that is considered to be part of a word.
 * Similar to inword () except that the latter uses the current line
 * and dot offset.
 */
static int
inwordpos (struct LINE *linep, int doto)
{
  return (doto < wllength (linep)) && ISWORD (lgetc (linep, doto)) &&
	  lgetc (linep, doto) != '\'';
}


/*
 * Get the word under the current cursor position and copy it to 'buffer',
 * up to a maximum of 'size' characters.
 */
static void
getcursorword (char *buffer, int size)
{
  int		n;
  int		doto;
  struct LINE	*linep;

  /* Scan back to the beginning of this word.
   */
  doto    = curwp->w_dot.o;
  linep   = curwp->w_dot.p;
  if (!inwordpos (linep, doto))
    return;
  while (doto >= 0 && inwordpos (linep, doto-1))
    doto--;

  /* Copy the word characters to the buffer.
   */
  for (n = 0; inwordpos (linep, doto) == TRUE && n < size - 1; n++)
    {
      buffer[n] = lgetc (linep, doto);
      doto++;
    }

  /* Null-terminate the string, if it is non-empty.
   */
  if (n > 0)
    buffer[n] = '\0';
}

/*
 * Prompt the user for a tag string to search for, and call the
 * caller-specified 'prep' function to prime the tag list for
 * an initial search.  Then search the tag list for the string,
 * and if found, read in the file given in the tag, and position
 * the cursor at the tag's line number.  If an argument is
 * given (f is TRUE), search for the next occurrence of the most recent tag
 * string specified.  If the argument (n) is negative, search
 * for the previous occurrence of the tag string.
 */

typedef int (*prepfunc)(const char *string);

int
searchtag (int f, int n, prepfunc prep, const char * tagtype)
{
  static tagref *lastref;	/* remember last ref    */
  tagref *r;			/* current tag          */
  tagfile *tf;			/* current file         */
  int s;
  char tpat[NPAT];		/* temporary pattern    */

  /* If an argument is specified (whose value is ignored),
   * search for the next tag whose name matches the last
   * pattern used.  Otherwise prompt for a pattern and
   * search from the top of the tags file.
   */
  if (f)
    {				/* argument specified?  */
      /* Start searching where we left off last time.
       */
      r = lastref ? (n >= 0 ? lastref->next : lastref->prev) : &tagreflist;
    }
  else
    {
      /* Get a default string to search for by looking at the word
       * under the cursor (if any).
       */
      getcursorword (tagpat, sizeof (tagpat));

      /* Prompt for the tag to search for if no argument
       * specified.
       */
      s = ereply ("Find %s [%s]: ", tpat, NPAT, tagtype, tagpat);
      if (s == TRUE)		/* Specified            */
	strcpy (tagpat, tpat);
      else if (s == FALSE && tagpat[0] != 0)	/* CR, but old one      */
	s = TRUE;
      if (s != TRUE)
	return s;

      /* Prepare things for an initial search.  This gives the
       * caller an opportunity to prime the tag list based on the
       * string specified by the user.
       */
      if (prep (tagpat) == FALSE)
	return FALSE;

#if DEBUG
      report = fopen ("report", "w");
#endif

      /* Start searching at the top of the tag list.
       */
      r = tagreflist.next;
    }

  /* Search the tags list for the specified tag.
   */
  while (r != &tagreflist)
    {
      dprintf ((report, "Ref %s, line %ld, offset %ld, file %s\n",
		r->string, r->line, r->offset, r->file->tf->fname));
      if (strstr (r->string, tagpat) != NULL)
	break;
      r = n >= 0 ? r->next : r->prev;	/* skip to next tag */
    }
  if (r == &tagreflist)
    {
      eprintf ("No %s%ss for %s", f ? "more " : "", tagtype, tagpat);
      return FALSE;
    }

  /* Save the current position in the tag list so we can
   * later resume the search at this point.
   */
  lastref = r;

  /* Visit the file specified by this tag, then move
   * to the line containing this tag.
   */
  tf = r->file;
  if (visit_file (tf->fname) == FALSE)
    return FALSE;
  if (gotoline (TRUE, r->line, 0) == FALSE)
    return FALSE;
#if DEBUG
  fclose (report);
#endif
  return TRUE;
}

/*
 * Search for a tag.  First read and parse the TAGS file,
 * then search it for the specified string (prompt the user).
 * Then read in the file given in the tag, and position
 * the cursor at the tag's line number.  If an argument is
 * given, search for the next occurrence of the last tag
 * string specified.  If the argument is negative, search
 * for the previous occurrence of the tag string.
 */
int
findtag (int f, int n, int k)
{
  return searchtag (f, n, preptag, "tag");
}

/*
 * Convert a CamelCase name to an underline_name.
 */
static const char *
uncamel(const char *s)
{
  static char buf[256];
  int i, j;

  for (i = 0, j = 0; j < sizeof(buf); i++)
    {
      char c = s[i];
      if (ISUPPER(c))
	{
	  if (j > 0)
	    buf[j++] = '_';
	  buf[j++] = TOLOWER(c);
	}
      else
	buf[j++] = c;
      if (c == '\0')
        break;
     }
   return buf;
}

/*
 * Most of the code for searching for a Rails component: gets
 * the name under the cursor, converts it to a non-camelcase
 * string, and returns the converted name.  Returns NULL on error.
 */

const char *
getrailsname(const char *type)
{
  int s;
  char tpat[NPAT];		/* temporary pattern    */

  /* Get a default string to search for by looking at the word
   * under the cursor (if any).
   */
  getcursorword (tagpat, sizeof (tagpat));

  /* Prompt for the tag to search for if no argument
   * specified.
   */
  s = ereply ("Find %s [%s]: ", tpat, NPAT, type, tagpat);
  if (s == TRUE)		/* Specified            */
    strcpy (tagpat, tpat);
  else if (s == FALSE && tagpat[0] != 0)	/* CR, but old one      */
    s = TRUE;
  if (s != TRUE)
    return NULL;
  return uncamel(tagpat);
}

/*
 * Search for a Rails controller.
 */
int
railscontroller(int f, int n, int k)
{
  char filename[NFILEN];
  const char *uname = getrailsname("controller for DB table");

  if (uname == NULL)
    return FALSE;
  else
    {
      snprintf(filename, NFILEN, "app/controllers/%s_controller.rb", uname);
      if (visit_file (filename) == FALSE)
        return FALSE;
      return TRUE;
    }
}

/*
 * Search for a Rails view
 */
int
railsview(int f, int n, int k)
{
  char filename[NFILEN];
  char method[NPAT];
  const char *classname;
  int s;

  classname = getrailsname("view for DB table");
  if (classname == NULL)
    return FALSE;
  else
    {
      method[0] = '\0';
      s = ereply("view method: ", method, NPAT);
      if (s == FALSE || s == ABORT || method[0] == '\0')
	return FALSE;
      snprintf(filename, NFILEN, "app/views/%s/%s.html.erb", classname, method);
      if (visit_file (filename) == FALSE)
        return FALSE;
      return TRUE;
    }
}

/*
 * Search for a Rails model.
 */
int
railsmodel(int f, int n, int k)
{
  char filename[NFILEN];
  const char *uname = getrailsname("model for class (singular, lower case)");

  if (uname == NULL)
    return FALSE;
  else
    {
      snprintf(filename, NFILEN, "app/models/%s.rb", uname);
      if (visit_file (filename) == FALSE)
        return FALSE;
      return TRUE;
    }
}

/*
 * Visit the file and line mentioned in the next gcc error message in the
 * current buffer.
 */

#define STRINGIZE(x) #x
#define INT2STR(x) STRINGIZE(x)

int
gccerror (int f, int n, int k)
{
  LINE *lp;
  BUFFER *bp;
  EWINDOW *wp;
  static const char *fmt = "%" INT2STR(NFILEN) "[^:]:%d:%d: %n";
  static const char *pfx = "In file included from";
  int pfxlen;
  char filename[NFILEN];
  int line, column;
  const uchar *str;
  int len, chars, wlen;
  char *copy = NULL;

  lp = curwp->w_dot.p;		/* Cursor location.	*/
  if (curwp->w_dot.o != 0)	/* Skip to next line	*/
    lp = lforw (lp);		/*  if not at column 1	*/
  bp = curwp->w_bufp;
  pfxlen = strlen(pfx);
  while (lp != bp->b_linep)
    {
      /* Make a copy of the line with a null termination
       * so that sscanf won't run off the end.
       */
      str = lgets (lp);
      len = llength (lp);
      copy = realloc (copy, len + 1);
      memcpy (copy, str, len);
      copy[len] = '\0';

      /* Test that the line doesn't start with a non-error prefix,
       * and that it starts with the pattern filename:line:column: .
       */
      if (strncmp (copy, pfx, pfxlen) != 0 &&
	  sscanf (copy, fmt, filename, &line, &column, &chars) == 3)
	{
	  /* Don't need the copy of the string any more.
	   */
	  free (copy);

	  /* Move cursor past the filename:line:column.
	   */
	  curwp->w_dot.p = lp;
	  curwp->w_dot.o = unslen (str, chars);
	  curwp->w_flag |= WFMOVE;

	  /* Check if file exists.
	   */
	  if (access (filename, R_OK) != F_OK)
	    {
	      eprintf ("Cannot read '%s'", filename);
	      return FALSE;
	    }

	  /* Pop up a window and read the indicated file into it.
	   */
	  if ((wp = wpopup ()) == NULL)
	    return FALSE;
	  curwp = wp;
	  if (visit_file (filename) == FALSE)
	    return FALSE;

	  /* Move to the indicated line and column.
	   */
	  if (gotoline (TRUE, line, 0) == FALSE)
	    return FALSE;
	  wlen = wllength (curwp->w_dot.p);
	  if (column >= wlen)
	    curwp->w_dot.o = wlen;
	  else
	    curwp->w_dot.o = column - 1;

	  /* Put as much of the error message as will fit
	   * on the echo line.
	   */
	  len = len - chars;
	  str += chars;
	  wlen = uslen (str);
	  if (wlen > ncol)
	    wlen = ncol;
	  len = uoffset (str, wlen);
	  copy = malloc (len + 1);
	  memcpy (copy, str, len);
	  copy[len] = '\0';
	  eprintf ("%s", copy);
	  free (copy);
	  return TRUE;
	}
      lp = lforw (lp);
    }
  eprintf ("gcc error not found");
  if (copy)
    free (copy);
  return FALSE;
}
