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

/* $Header: /home/bloovis/cvsroot/pe/search.c,v 1.2 2005-10-18 02:18:06 bloovis Exp $
 *
 * Name:	MicroEMACS
 * 		Search commands.
 * Version:	30
 * By:		rex::conroy
 *		decvax!decwrl!dec-rhea!dec-rex!conroy
 * Modified by:	Mark Alexander (amdahl!drivax!alexande)
 *
 *
 * The functions in this file implement the
 * search commands (both plain and incremental searches
 * are supported) and the query-replace command.
 *
 * The plain old search code is part of the original
 * MicroEMACS "distribution". The incremental search code,
 * and the query-replace code, is by Rich Ellison.
 *
 * $Log: search.c,v $
 * Revision 1.2  2005-10-18 02:18:06  bloovis
 * Rename some things to avoid conflict with ncurses.
 *
 * Revision 1.1.1.1  2003/11/06 02:51:52  bloovis
 * Imported sources
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
 * Revision 1.1.1.1  2000/07/14 19:23:11  malexander
 * Imported sources
 *
 * Revision 1.5  1996/10/22 16:01:43  marka
 * Merge forward and backward regexp searches into common code.
 *
 * Revision 1.4  91/04/19  23:26:10  alexande
 * Case-fold command is now a toggle if no argument supplied.
 * 
 * Revision 1.3  91/01/07  10:29:45  alexande
 * Remove C++ warnings.
 * 
 * Revision 1.2  89/01/13  12:59:49  MGA
 * Changed incremental search to be more like Epsilon.
 * 
 * Revision 1.1  89/01/13  09:42:09  MGA
 * Initial revision
 * 
 */
#include	"def.h"
#include	"regexp.h"

#define CCHR(x)		((x)-'@')

#define SRCH_BEGIN	(0)	/* Search sub-codes.    */
#define	SRCH_FORW	(-1)
#define SRCH_BACK	(-2)
#define SRCH_PREV	(-3)
#define SRCH_NEXT	(-4)
#define SRCH_NOPR	(-5)
#define SRCH_ACCM	(-6)
#define SRCH_REGFORW	(-7)
#define SRCH_REGBACK	(-8)

typedef struct
{
  int s_code;
  LINE *s_dotp;
  int s_doto;
}
SRCHCOM;

static SRCHCOM cmds[NSRCH];
static int cip;
static regexp *regpat;
static wchar_t upat[NPAT];	/* Unicode copy of pat	*/
int patlen;			/* # of Unicode characters in upat */

int srch_lastdir = SRCH_NOPR;	/* Last search flags.   */

/*
 * Set the casefold flag according to the numeric argument.
 * If zero, searches do not fold case (i.e. searches
 * will be exact).  If non-zero, searches will fold case (i.e.
 * upper case letters match their corresponding lower case letters).
 */
int
foldcase (int f, int n, int k)
{
  casefold = f ? n : !casefold;
  upmapinit ();			/* reset the translation table  */
  if (casefold)
    eprintf ("[Case folding now ON]");
  else
    eprintf ("[Case folding now OFF]");
  return (TRUE);
}

/*
 * Convert pat to Unicode, store the result in upat,
 * and set patlen to the number of Unicode characters stored.
 */
static void
unicodepat (void)
{
  uchar *s;
  wchar_t *d;
  int clen;

  s = pat;
  d = upat;
  while (*s != 0)
    {
      *d++ = ugetc (s, 0, &clen);
      s += clen;
    }
  *d = 0;
  patlen = d - upat;
}

/*
 * Read a pattern.
 * Stash it in the external variable "pat". The "pat" is
 * not updated if the user types in an empty line. If the user typed
 * an empty line, and there is no old pattern, it is an error.
 * Display the old pattern, in the style of Jeff Lomicka. There is
 * some do-it-yourself control expansion.
 */
static int
readpattern (const char *prompt)
{
  register int s;
  char tpat[NPAT];

  s = ereplyf ("%s [%s]: ", tpat, NPAT, EFNEW | EFCR | EFPAT, prompt, pat);
  if (s == TRUE)		/* Specified            */
    {
      strcpy ((char *) pat, tpat);
      unicodepat ();		/* Make Unicode version of pat */
    }
  else if (s == FALSE && pat[0] != 0)	/* CR, but old one      */
    s = TRUE;
  return (s);
}

/*
 * This routine does the real work of a regular expression
 * forward search. The pattern is sitting in the static
 * variable "pat", and the compiled pattern is in "regpat".
 * If found, dot is updated, the window system
 * is notified of the change, and TRUE is returned. If the
 * string isn't found, FALSE is returned.
 *
 * A copy of the line where the pattern was found is kept
 * around until the next search is performed, so that that
 * pointers to the pattern in regpat will still be valid
 * if regsub is called.  The copy is freed on the next search.
 */
static int
doregsrch (int dir)
{
  register LINE *clp;
  register int cbo;
  register LINE *lastline;
  uchar *line;
  int linelen;
  static uchar *buf = NULL;
  static int buflen = 0;

  clp = curwp->w_dot.p;
  cbo = curwp->w_dot.o;
  lastline = curbp->b_linep;
  if (clp == lastline)
    return (FALSE);
  for (;;)
    {
      /* Get byte offset of the UTF-8 character at cbo.
       */
      int offset = wloffset (clp, cbo);

      /* Make a copy of the line in the temporary buffer.
       */
      if (dir == SRCH_REGFORW)
	{
	  line = lgets (clp) + offset;
	  linelen = llength (clp) - offset;
	}
      else
	{
	  line = lgets (clp);
	  linelen = offset;
	}
      if (linelen + 1 > buflen)
	{
	  uchar *newbuf;

	  buflen = linelen + 1;
	  newbuf = realloc (buf, buflen);
	  if (newbuf == NULL)
	    {
	      eprintf ("Can't allocate %d bytes for searching", buflen);
	      return (FALSE);
	    }
	  buf = newbuf;
	}
      memcpy (buf, line, linelen);
      buf[linelen] = '\0';

      /* Search for the pattern in the temporary buffer.
       * If found, calculate the actual ending offset
       * of the found string in the current line,
       * and set the dot to that location.
       */
      if (regexec (regpat, (const char *) buf))
	{
	  curwp->w_dot.p = clp;
	  if (dir == SRCH_REGFORW)
	    curwp->w_dot.o = unslen (buf, regpat->endp[0] - (char *) buf) + cbo;
	  else
	    curwp->w_dot.o = unslen (buf, regpat->startp[0] - (char *) buf);
	  curwp->w_flag |= WFMOVE;
	  return (TRUE);
	}

      /* Try again in the next line.
       */
      if (dir == SRCH_REGFORW)
	{
	  clp = lforw (clp);
	  cbo = 0;
	}
      else
	{
	  clp = lback (clp);
	  cbo = wllength (clp);
	}
      if (clp == lastline)
	{
	  return (FALSE);
	}
    }
}


/*
 * Search forward using regular expression.
 * Get a search string, which must be a regular expression, from the user,
 * and search for it, starting at ".". If found, "." gets moved to just
 * after the matched characters, and display does all the hard stuff.
 * If not found, it just prints a message.
 */
int
regsearch (char *prompt, int dir)
{
  register int s;

  /* Prompt for a regular expression pattern.
   */
  if ((s = readpattern (prompt)) != TRUE)
    return (s);
  srch_lastdir = dir;

  /* Compile the pattern into a regexp program.
   */
  if (regpat != NULL)
    free (regpat);
  if ((regpat = regcomp ((const char *) pat)) == NULL)	/* regerror shows message */
    return (FALSE);

  /* Search the current buffer for the pattern.
   */
  if (doregsrch (dir) == FALSE)
    {
      eprintf ("Not found");
      return (FALSE);
    }
  return (TRUE);
}

int
forwregsearch (int f, int n, int k)
{
  return regsearch ("Regexp-search", SRCH_REGFORW);
}

/*
 * Reverse search using regular expression.
 * Get a search string, which must be a regular expression, from the user,
 * and search for it, starting at ".". If found, "." is left pointing
 * at the first character of the pattern [the last character that
 * was matched].
 */
int
backregsearch (int f, int n, int k)
{
  return regsearch ("Reverse regexp-search", SRCH_REGBACK);
}


/*
 * This routine does the real work of a
 * forward search. The pattern is sitting in the external
 * variable "pat". If found, dot is updated, the window system
 * is notified of the change, and TRUE is returned. If the
 * string isn't found, FALSE is returned.
 */

#ifndef SRCHASM

static int
forwsrch (void)
{
  register LINE *clp;
  const uchar *cend;
  register int cbo;
  const uchar *cptr;
  register LINE *tlp;
  const uchar *tend;
  register int tbo;
  const uchar *tptr;
  register int pp;
  register LINE *lastline;
  wchar_t uc;
  int ulen;

  clp = curwp->w_dot.p;
  cbo = curwp->w_dot.o;
  cend = lend (clp);
  cptr = wlgetcptr (clp, cbo);
  lastline = curbp->b_linep;
  if (clp == lastline)
    return (FALSE);
  for (;;)
    {
    fail:
      if (cptr >= cend)
	{
	  if ((clp = lforw (clp)) == lastline)
	    return (FALSE);
	  cend = lend (clp);
	  cbo = 0;
	  cptr = wlgetcptr (clp, cbo);
	  if (upat[0] != '\n')
	    goto fail;
	}
      else
	{
	  uc = ugetc (cptr, 0, &ulen);
	  cptr += ulen;
	  ++cbo;
	  if (!CEQ (uc, upat[0]))
	    goto fail;
	}
      tlp = clp;
      tbo = cbo;
      tptr = cptr;
      tend = cend;
      for (pp = 1; pp < patlen; pp++)
	{
	  if (tptr >= tend)
	    {
	      if ((tlp = lforw (tlp)) == lastline)
		return (FALSE);
	      if (upat[pp] != '\n')
		goto fail;
	      tend = lend (tlp);
	      tbo = 0;
	      tptr = lgets (tlp);
	    }
	  else
	    {
	      uc = ugetc (tptr, 0, &ulen);
	      if (!CEQ (uc, upat[pp]))
		goto fail;
	      tptr += ulen;
	      ++tbo;
	    }
	}
      curwp->w_dot.p = tlp;
      curwp->w_dot.o = tbo;
      curwp->w_flag |= WFMOVE;
      return (TRUE);
    }
  return (FALSE);
}

/*
 * This routine does the real work of a
 * backward search. The pattern is sitting in the external
 * variable "pat". If found, dot is updated, the window system
 * is notified of the change, and TRUE is returned. If the
 * string isn't found, FALSE is returned.
 */
static int
backsrch (void)
{
  register LINE *clp;
  register int cbo;
  const uchar *cptr;
  register LINE *tlp;
  register int tbo;
  const uchar *tptr;
  register LINE *lastline;
  register int epp;
  register int pp;
  wchar_t uc;
  int ulen;

  lastline = curbp->b_linep;
  epp = patlen - 1;
  clp = curwp->w_dot.p;
  cbo = curwp->w_dot.o;
  cptr = wlgetcptr (clp, cbo);
  for (;;)
    {
    fail:
      if (cbo == 0)
	{
	  if ((clp = lback (clp)) == lastline)
	    return (FALSE);
	  cbo = wllength (clp);
	  cptr = wlgetcptr (clp, cbo);
	  if (upat[epp] != '\n')
	    goto fail;
	}
      else
	{
	  uc = ugetprevc (cptr, &ulen);
	  cptr -= ulen;
	  --cbo;
	  if (!CEQ (uc, upat[epp]))
	    goto fail;
	}
      tlp = clp;
      tbo = cbo;
      tptr = cptr;
      pp = epp;
      while (pp != 0)
	{
	  if (tbo == 0)
	    {
	      if ((tlp = lback (tlp)) == lastline)
		return (FALSE);
	      tbo = wllength (tlp);
	      tptr = lend (tlp);
	      if (upat[--pp] != '\n')
		goto fail;
	    }
	  else
	    {
	      uc = ugetprevc (tptr, &ulen);
	      if (!CEQ (uc, upat[--pp]))
		goto fail;
	      tptr -= ulen;
	      --tbo;
	    }
	}
      curwp->w_dot.p = tlp;
      curwp->w_dot.o = tbo;
      curwp->w_flag |= WFMOVE;
      return (TRUE);
    }
}

#endif


/*
 * Do a search of the type specified by dir.
 * If the search succeeds, return TRUE.
 * If the search fails, return FALSE.
 * If there is an error that prevents the search from being
 * performed (e.g., a regexp search when no regular expression
 * is available in regpat), return ABORT.
 */
int
dosearch (int dir)
{
  int status;

  switch (dir)
    {
    case SRCH_FORW:
      status = forwsrch ();
      break;
    case SRCH_BACK:
      status = backsrch ();
      break;
    case SRCH_REGFORW:
    case SRCH_REGBACK:
      if (regpat != NULL)
	status = doregsrch (dir);
      else
	status = ABORT;
      break;
    default:
      status = ABORT;
      break;
    }
  return status;
}

/* 
 * Search again, using the same search string
 * and direction as the last search command. The direction
 * has been saved in "srch_lastdir", so you know which way
 * to go.
 */
int
searchagain (int f, int n, int k)
{
  int status = dosearch (srch_lastdir);

  if (status == FALSE)
    eprintf ("Not found");
  else if (status == ABORT)
    {
      eprintf ("No last search");
      status = FALSE;
    }
  return status;
}

/*
 * Display regular expression error.
 * The regular expression compiler regcomp() calls this function when
 * an error is detected.  This function displays the error message
 * on the echo line.
 */
void
regerror (const char *s)
{
  eprintf ("regular expression error: %s", s);
}

/*
 * Find the first key bound to a particular command, and convert it
 * from the 32-bit internal code to ASCII.  If no key is bound to
 * the command, or the key is a META- or CTLX- key, return the default
 * key value parameter. Otherwise return the key converted to ASCII.
 */
static int
cvtbind (const char *command, int defkey)
{
  int key;

  key = getbindingforcmd (command);	/* look up key  */
  if (key == -1 || (key & (KCTLX | KMETA)) != 0)	/* META or C-X  */
    return (defkey);
  else if ((key & KCTRL) != 0)	/* CTRL- key?   */
    return (CCHR (key & KCHAR));	/* convert it   */
  else
    return (key & KCHAR);	/* normal key   */
}

static void
is_cpush (cmd)
     register int cmd;
{
  if (++cip >= NSRCH)
    cip = 0;
  cmds[cip].s_code = cmd;
}

static void
is_lpush (void)
{
  register int ctp;

  ctp = cip + 1;
  if (ctp >= NSRCH)
    ctp = 0;
  cmds[ctp].s_code = SRCH_NOPR;
  cmds[ctp].s_doto = curwp->w_dot.o;
  cmds[ctp].s_dotp = curwp->w_dot.p;
}

static void
is_pop (void)
{
  if (cmds[cip].s_code != SRCH_NOPR)
    {
      curwp->w_dot.o = cmds[cip].s_doto;
      curwp->w_dot.p = cmds[cip].s_dotp;
      curwp->w_flag |= WFMOVE;
      cmds[cip].s_code = SRCH_NOPR;
    }
  if (--cip <= 0)
    cip = NSRCH - 1;
}

static int
is_peek (void)
{
  if (cip == 0)
    return (cmds[NSRCH - 1].s_code);
  else
    return (cmds[cip - 1].s_code);
}

static int
is_undo (int *pptr, int *dir)
{
  switch (cmds[cip].s_code)
    {
    case SRCH_NOPR:
    case SRCH_BEGIN:
    case SRCH_NEXT:
    case SRCH_PREV:
      break;

    case SRCH_FORW:
      *dir = SRCH_BACK;
      break;

    case SRCH_BACK:
      *dir = SRCH_FORW;
      break;

    case SRCH_ACCM:
    default:
      *pptr -= 1;
      if (*pptr < 0)
	*pptr = 0;
      pat[*pptr] = '\0';
      break;
    }
  is_pop ();
  return (TRUE);
}

static int
is_find (int dir)
{
  register int plen;

  plen = strlen ((const char *) pat);
  if (plen != 0)
    {
      if (dir == SRCH_FORW || dir == SRCH_NEXT)
	{
	  backchar (FALSE, plen, KRANDOM);
	  if (forwsrch () == FALSE)
	    {
	      forwchar (FALSE, plen, KRANDOM);
	      return (FALSE);
	    }
	  return (TRUE);
	}
      if (dir == SRCH_BACK || dir == SRCH_PREV)
	{
	  forwchar (FALSE, plen, KRANDOM);
	  if (backsrch () == FALSE)
	    {
	      backchar (FALSE, plen, KRANDOM);
	      return (FALSE);
	    }
	  return (TRUE);
	}
      eprintf ("bad call to is_find");
      ctrlg (FALSE, 0, KRANDOM);
      return (FALSE);
    }
  return (FALSE);
}

/*
 * Prompt writing routine for the incremental search. 
 * The "prompt" is just a string. The "flag" determines
 * if a "[ ]" or ":" embelishment is used.
 */
static void
is_dspl (const char *prompt, int flag)
{
  if (flag != FALSE)
    eprintf ("%s [%s]", prompt, pat);
  else
    eprintf ("%s: %s", prompt, pat);
}

/*
 * If called with "dir" not one of SRCH_FORW
 * or SRCH_BACK, this routine used to print an error
 * message. It also used to return TRUE or FALSE,
 * depending on if it liked the "dir". However, none
 * of the callers looked at the status, so I just
 * made the checking vanish.
 */
static void
is_prompt (int dir, int flag, int success)
{
  if (dir == SRCH_FORW)
    {
      if (success != FALSE)
	is_dspl ("i-search forward", flag);
      else
	is_dspl ("failing i-search forward", flag);
    }
  else if (dir == SRCH_BACK)
    {
      if (success != FALSE)
	is_dspl ("i-search backward", flag);
      else
	is_dspl ("failing i-search backward", flag);
    }
}

/*
 * Incremental Search.
 *	dir is used as the initial direction to search.
 *	^S	switch direction to forward, find next
 *	^R	switch direction to reverse, find prev
 *	^Q	quote next character (allows searching for ^N etc.)
 *	<CR>	exit from Isearch. (<ESC> also does it).
 *	<DEL>	undoes last character typed. (tricky job to do this correctly).
 *	<BKSP>	same as <DEL>
 *	else	accumulate into search string
 */
static int
isearch (int dir)
{
  register int c;
  register LINE *clp;
  register int cbo;
  register int success;
  int pptr;
  int fkey, bkey;		/* keys bound to commands */

  /* Get the bindings for incremental search so user can use those
   * keys instead of C-S or C-R.  We can't allow C-X or M- keys
   * because those are two-key codes. But we can't use getkey()
   * to get internal 32-bit codes because then we'd lose the
   * ability to exit with an ESC key by itself.
   */
  fkey = cvtbind ("forw-i-search", CCHR ('S'));
  bkey = cvtbind ("back-i-search", CCHR ('R'));

  for (cip = 0; cip < NSRCH; cip++)
    cmds[cip].s_code = SRCH_NOPR;
  cip = 0;
  pptr = -1;
  clp = curwp->w_dot.p;
  cbo = curwp->w_dot.o;
  is_lpush ();
  is_cpush (SRCH_BEGIN);
  success = TRUE;
  is_prompt (dir, TRUE, success);
  for (;;)
    {
      update ();
      c = getinp ();
      if (c == fkey)
	c = CCHR ('S');
      else if (c == bkey)
	c = CCHR ('R');
      switch (c)
	{
	case METACH:		/* ESC */
	case CCHR ('M'):	/* carriage return */
	  srch_lastdir = dir;
	  eprintf ("[Done]");
	  return (TRUE);

	case CCHR ('G'):	/* Control-G */
	  curwp->w_dot.p = clp;
	  curwp->w_dot.o = cbo;
	  curwp->w_flag |= WFMOVE;
	  srch_lastdir = dir;
	  ctrlg (FALSE, 0, KRANDOM);
	  return (FALSE);

	case CCHR ('S'):	/* Control-S */
	  if (dir == SRCH_BACK)
	    {
	      dir = SRCH_FORW;
	      is_lpush ();
	      is_cpush (SRCH_FORW);
	      success = TRUE;
	    }
	  if (success == FALSE && dir == SRCH_FORW)
	    break;
	  is_lpush ();
	  forwchar (FALSE, 1, KRANDOM);
	  if (is_find (SRCH_NEXT) != FALSE)
	    {
	      is_cpush (SRCH_NEXT);
	      pptr = strlen ((const char *) pat);
	    }
	  else
	    {
	      backchar (FALSE, 1, KRANDOM);
	      ttbeep ();
	      success = FALSE;
	    }
	  is_prompt (dir, FALSE, success);
	  break;

	case CCHR ('R'):
	  if (dir == SRCH_FORW)
	    {
	      dir = SRCH_BACK;
	      is_lpush ();
	      is_cpush (SRCH_BACK);
	      success = TRUE;
	    }
	  if (success == FALSE && dir == SRCH_BACK)
	    break;
	  is_lpush ();
	  backchar (FALSE, 1, KRANDOM);
	  if (is_find (SRCH_PREV) != FALSE)
	    {
	      is_cpush (SRCH_PREV);
	      pptr = strlen ((const char *) pat);
	    }
	  else
	    {
	      forwchar (FALSE, 1, KRANDOM);
	      ttbeep ();
	      success = FALSE;
	    }
	  is_prompt (dir, FALSE, success);
	  break;

	case CCHR ('H'):
	case 0x7F:
	  if (is_undo (&pptr, &dir) != TRUE)
	    return (ABORT);
	  if (is_peek () != SRCH_ACCM)
	    success = TRUE;
	  is_prompt (dir, FALSE, success);
	  break;

	case CCHR ('^'):
	case CCHR ('Q'):
	  c = getinp ();
	  goto addchar;

	default:
	  if (CISCTRL (c) != FALSE)
	    {
	      ungetinp (c);	/* push back input */
	      curwp->w_flag |= WFMOVE;
	      return (success);
	    }
	addchar:
	  if (pptr == -1)
	    pptr = 0;
	  if (pptr == 0)
	    success = TRUE;
	  pat[pptr++] = c;
	  if (pptr == NPAT)
	    {
	      eprintf ("Pattern too long");
	      ctrlg (FALSE, 0, KRANDOM);
	      return (ABORT);
	    }
	  pat[pptr] = '\0';
	  is_lpush ();
	  if (success != FALSE)
	    {
	      if (is_find (dir) != FALSE)
		is_cpush (c);
	      else
		{
		  success = FALSE;
		  ttbeep ();
		  is_cpush (SRCH_ACCM);
		}
	    }
	  else
	    is_cpush (SRCH_ACCM);
	  is_prompt (dir, FALSE, success);
	}
    }
}

/*
 * Print the number of replacements performed by a query-replace
 * or a replace-string.
 */
static void
prcnt (int rcnt)
{
  if (rcnt == 0)
    eprintf ("No replacements done");
  else if (rcnt == 1)
    eprintf ("[1 replacement done]");
  else
    eprintf ("[%d replacements done]", rcnt);
}


/*
 * Get the appropriate replacement string for the kind of search
 * and replace operation indicated by dir.  If this is a regular
 * expression operation, use regsub to create the replacement
 * string based on the substitution pattern in news, and store
 * the replacement string in sub (whose maximum size is sublen).
 * Otherwise use news without modification as the replacement string.
 * Store the number of Unicode characters in the found pattern
 * at *plen, and return a pointer to the correct substitution string.
 */
static char *
getrepl (int dir, char *news, char *sub, int sublen, int *plen)
{
  char *str;

  switch (dir)
    {
    case SRCH_FORW:
    case SRCH_BACK:
      str = news;
      break;
    case SRCH_REGFORW:
    case SRCH_REGBACK:
      if (regpat == NULL)
	{
	  eprintf ("No regular expression!");
	  str = news;
	}
      if (regsub (regpat, news, sub, sublen))
	str = sub;
      else
	str = news;
      break;
    default:
      eprintf ("Illegal search direction %d\n", dir);
      str = news;
    }
  if (str == news)
    *plen = patlen;
  else
    *plen = unslen ((const uchar *) regpat->startp[0],
		    regpat->endp[0] - regpat->startp[0]);
  return str;
}

/*
 * Helper function for all search and replace functions:
 *      If query is TRUE, prompt the user for each replacement:
 *	  A space or a comma replaces the string, a period replaces and quits,
 *	  an n doesn't replace, a C-G quits.
 *	If query is FALSE, replace all strings with no prompting.
 *      The f parameter is a case-fold hack flag, passed to lreplace.
 *      The dir parameter indicates the kind of operation (normal
 *        or regular expression).
 */
int
searchandreplace (int f, int query, int dir)
{
  register int s;
  char news[NPAT];		/* replacement string           */
  char sub[NPAT];		/* regsub-modified replacement	*/
  char *repl;			/* correct replacement string	*/
  register LINE *clp;		/* saved line pointer           */
  int cbo;			/* offset into the saved line   */
  int rcnt = 0;			/* Replacements made so far     */
  int plen;			/* length of found string       */
  int c;			/* input character		*/

  if ((s = readpattern ("Old string")) != TRUE)
    return (s);
  if ((s = ereply ("New string: ", news, NPAT)) == ABORT)
    return (s);
  if (s == FALSE)
    news[0] = '\0';

  /* If this a regular expression operation, compile the pattern into a
   * regexp program.
   */
  if (dir == SRCH_REGFORW || dir == SRCH_REGBACK)
    {
      if (regpat != NULL)
	free (regpat);
      if ((regpat = regcomp ((const char *) pat)) == NULL)	/* regerror shows message */
	return (FALSE);
    }

  if (query)
    eprintf ("[Query Replace:  \"%s\" -> \"%s\"]", pat, news);

  plen = patlen;

  /*
   * Search forward repeatedly, checking each time whether to insert
   * or not.  The "!" case makes the check always true, so it gets put
   * into a tighter loop for efficiency.
   *
   * If we change the line that is the remembered value of dot, then
   * it is possible for the remembered value to move.  This causes great
   * pain when trying to return to the non-existant line.
   *
   * possible fixes:
   * 1) put a single, relocated marker in the EWINDOW structure, handled
   *    like mark.  The problem now becomes a what if two are needed...
   * 2) link markers into a list that gets updated (auto structures for
   *    the nodes)
   * 3) Expand the mark into a stack of marks and add pushmark, popmark.
   */

  clp = curwp->w_dot.p;		/* save the return location     */
  cbo = curwp->w_dot.o;
  while (dosearch (dir) == TRUE)
    {
    retry:
      if (!inprof)
	update ();		/* show current position        */
      if (query)
	c = getinp ();
      else
	c = '!';
      switch (c)
	{
	case ' ':
	case ',':
	case 'y':
	case 'Y':
	  curwp->w_savep = clp;
	  repl = getrepl (dir, news, sub, sizeof (sub), &plen);
	  if (lreplace (plen, repl, f) == FALSE)
	    return (FALSE);
	  rcnt++;
	  clp = curwp->w_savep;
	  break;

	case '.':
	  curwp->w_savep = clp;
	  repl = getrepl (dir, news, sub, sizeof (sub), &plen);
	  if (lreplace (plen, repl, f) == FALSE)
	    return (FALSE);
	  rcnt++;
	  clp = curwp->w_savep;
	  goto stopsearch;

	case CCHR ('G'):
	  ctrlg (FALSE, 0, KRANDOM);
	  goto stopsearch;

	case '!':
	  do
	    {
	      curwp->w_savep = clp;
	      repl = getrepl (dir, news, sub, sizeof (sub), &plen);
	      if (lreplace (plen, repl, f) == FALSE)
		return (FALSE);
	      rcnt++;
	      clp = curwp->w_savep;
	    }
	  while (dosearch (dir) == TRUE);
	  goto stopsearch;

	case 'n':
	  break;

	default:
	  eprintf
	    ("<SP>[,Yy] replace, [.] rep-end, [n] don't, [!] repl rest [C-G] quit");
	  goto retry;
	}
    }
stopsearch:
  curwp->w_dot.p = clp;
  curwp->w_dot.o = cbo;
  curwp->w_flag |= WFHARD;
  if (!inprof)
    update ();
  prcnt (rcnt);
  return (TRUE);
}

/*
 * Query Replace.
 *	Replace strings selectively.  Does a search and replace operation.
 *	A space or a comma replaces the string, a period replaces and quits,
 *	an n doesn't replace, a C-G quits.  If an argument is given,
 *	don't query, just do all replacements.
 */
int
queryrepl (int f, int n, int k)
{
  return searchandreplace (f, TRUE, SRCH_FORW);
}

/*
 * Replace-string function.  This is the same as query-replace,
 * with the difference that it does not prompt for confirmation
 * on each string.
 */
int
replstring (int f, int n, int k)
{
  return searchandreplace (f, FALSE, SRCH_FORW);
}

/*
 * Regexp Query Replace.
 *	Replace strings selectively, using a regular expression as the pattern
 *      and a regular expression subsitution string as the replacement.
 *	Otherwise similar to query-replace.
 */
int
regqueryrepl (int f, int n, int k)
{
  return searchandreplace (f, TRUE, SRCH_REGFORW);
}

/*
 * Regexp Replace.
 *	Replace strings unconditionally, using a regular expression as the pattern
 *      and a regular expression subsitution string as the replacement.
 *	Otherwise similar to replace-string.
 */
int
regrepl (int f, int n, int k)
{
  return searchandreplace (f, FALSE, SRCH_REGFORW);
}

#if 0				/* replaced by EQ macro */
/*
 * Compare two characters.
 * The "bc" comes from the buffer.
 * It has its case folded out. The
 * "pc" is from the pattern.
 */
static int
eq (int bc, int pc)
{
  register int ibc;
  register int ipc;

  ibc = bc & 0xFF;
  ipc = pc & 0xFF;
  if (ISLOWER (ibc) != FALSE)
    ibc = TOUPPER (ibc);
  if (ISLOWER (ipc) != FALSE)
    ipc = TOUPPER (ipc);
  if (ibc == ipc)
    return (TRUE);
  return (FALSE);
}
#endif

/*
 * Search forward.
 * Get a search string from the user, and search for it,
 * starting at ".". If found, "." gets moved to just after the
 * matched characters, and display does all the hard stuff.
 * If not found, it just prints a message.
 */
int
forwsearch (int f, int n, int k)
{
  register int s;

  if ((s = readpattern ("Search")) != TRUE)
    return (s);
  srch_lastdir = SRCH_FORW;
  return searchagain (f, n, k);
}

/*
 * Reverse search.
 * Get a search string from the  user, and search, starting at "."
 * and proceeding toward the front of the buffer. If found "." is left
 * pointing at the first character of the pattern [the last character that
 * was matched].
 */
int
backsearch (int f, int n, int k)
{
  register int s;

  if ((s = readpattern ("Reverse search")) != TRUE)
    return (s);
  srch_lastdir = SRCH_BACK;
  return searchagain (f, n, k);
}

/*
 * Use incremental searching, initially in the forward direction.
 * isearch ignores any explicit arguments.
 */
int
forwisearch (int f, int n, int k)
{
  return (isearch (SRCH_FORW));
}

/*
 * Use incremental searching, initially in the reverse direction.
 * isearch ignores any explicit arguments.
 */
int
backisearch (int f, int n, int k)
{
  return (isearch (SRCH_BACK));
}

/*
 * Search for the next occurence of the character at dot.
 * If character is a (){}[]<>, search for matching bracket, taking
 * proper account of nesting.
 * Written by Walter Bright.
 */
static int srchstate;

/*
 * This routine determines when to ignore characters
 * (ie. those in comments and those in quotes)
 */
struct statetrans
{
  int bslash;
  int fslash;
  int quote;
  int dquote;
  int star;
  int nl;
  int other;
  int ignore;
};

static struct statetrans forward_trans[] =
	/* forward state diagram */
{
  /* bsl fsl quo dqu sta  nl oth ign */
  /* 0 normal         */ {0, 1, 4, 6, 0, 0, 0, 0},
  /* 1 normal seen /  */ {0, 8, 4, 6, 2, 0, 0, 0},
  /* 2 comment        */ {2, 2, 2, 2, 3, 2, 2, 1},
  /* 3 comment seen * */ {2, 0, 2, 2, 3, 2, 2, 1},
  /* 4 quote          */ {5, 4, 0, 4, 4, 0, 4, 1},
  /* 5 quote seen \   */ {4, 4, 4, 4, 4, 4, 4, 1},
  /* 6 string         */ {7, 6, 6, 0, 6, 0, 6, 1},
  /* 7 string seen \  */ {6, 6, 6, 6, 6, 6, 6, 1},
  /* 8 C++ comment    */ {8, 8, 8, 8, 8, 0, 8, 1}
};

static struct statetrans backwards_trans[] =
	/* backwards state diagram */
{
  /* bsl fsl quo dqu sta  nl oth ign */
  /* 0 normal         */ {0, 1, 4, 6, 0, 0, 0, 0},
  /* 1 normal seen /  */ {0, 1, 4, 6, 2, 0, 0, 0},
  /* 2 comment        */ {2, 2, 2, 2, 3, 2, 2, 1},
  /* 3 comment seen * */ {2, 0, 2, 2, 3, 2, 2, 1},
  /* 4 quote          */ {4, 4, 5, 4, 4, 5, 4, 1},
  /* 5 quote seen end */ {4, 0, 0, 0, 0, 0, 0, 0},
  /* 6 string         */ {6, 6, 6, 7, 6, 7, 6, 1},
  /* 7 string seen end */ {6, 0, 0, 0, 0, 0, 0, 0}
};

static int
searchignore (int ch, int forward)
{
  int lss = srchstate;		/* local search state */
  struct statetrans *trans;

  /* If not cmode, check all chars
   */
/*	if (!(curbp->b_mode & MDCMOD)) return (0); */

  if (forward)
    trans = forward_trans;
  else
    trans = backwards_trans;

  switch (ch)
    {
    case '\\':
      srchstate = trans[lss].bslash;
      break;
    case '/':
      srchstate = trans[lss].fslash;
      break;
    case '\'':
      srchstate = trans[lss].quote;
      break;
    case '"':
      srchstate = trans[lss].dquote;
      break;
    case '*':
      srchstate = trans[lss].star;
      break;
    case '\n':
      srchstate = trans[lss].nl;
      break;
    default:
      srchstate = trans[lss].other;
      break;
    }
  return (trans[lss].ignore);
}

int
searchparen (int f, int n, int k)
{
  register LINE *clp;
  register int cbo;
  register int len;
  register int i;
  char chinc, chdec, ch;
  int count;
  int forward;
  static char bracket[][2] =
    { {'(', ')'}, {'<', '>'}, {'[', ']'}, {'{', '}'} };

  clp = curwp->w_dot.p;		/* get pointer to current line  */
  cbo = curwp->w_dot.o;		/* and offset into that line    */
  count = 0;

  len = wllength (clp);
  if (cbo >= len)
    chinc = '\n';
  else
    chinc = lgetc (clp, cbo);

  forward = TRUE;		/* assume search forward        */
  chdec = chinc;
  for (i = 0; i < 4; i++)
    if (bracket[i][0] == chinc)
      {
	chdec = bracket[i][1];
	break;
      }
  for (i = 0; i < 4; i++)
    if (bracket[i][1] == chinc)
      {
	chdec = bracket[i][0];
	forward = FALSE;	/* search backwards     */
	break;
      }

  srchstate = 0;		/* start state for ignore */

  while (1)
    {
      if (forward)
	{
	  if (cbo >= len)
	    {			/* proceed to next line */
	      clp = lforw (clp);
	      if (clp == curbp->b_linep)
		break;		/* if end of buffer     */
	      len = wllength (clp);
	      cbo = 0;
	    }
	  else
	    cbo++;
	}
      else
	{			/* backward */
	  if (cbo == 0)
	    {
	      clp = lback (clp);
	      if (clp == curbp->b_linep)
		break;
	      len = wllength (clp);
	      cbo = len;
	    }
	  else
	    --cbo;
	}

      ch = (cbo < len) ? lgetc (clp, cbo) : '\n';
      if (!searchignore (ch, forward))
	{
	  if (CEQ (ch, chdec))
	    {
	      if (count-- == 0)
		{		/* We've found it   */
		  curwp->w_dot.p = clp;
		  curwp->w_dot.o = cbo;
		  curwp->w_flag |= WFMOVE;
		  return (TRUE);
		}
	    }
	  else if (CEQ (ch, chinc))
	    count++;
	}
    }
  eprintf ("Not found");
  return (FALSE);
}
