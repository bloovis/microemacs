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

/* $Header: /home/bloovis/cvsroot/pe/kbd.c,v 1.2 2005-10-18 02:18:03 bloovis Exp $
 *
 * Name:	MicroEMACS
 *		Terminal independent keyboard handling.
 * Version:	29
 * Modified by:	Mark Alexander (amdahl!drivax!alexande)
 *
 * $Log: kbd.c,v $
 * Revision 1.2  2005-10-18 02:18:03  bloovis
 * Rename some things to avoid conflict with ncurses.
 *
 * Revision 1.1.1.1  2003/11/06 02:51:52  bloovis
 * Imported sources
 *
 * Revision 1.4  2000/09/29 00:19:38  malexander
 * Numerous changes to eliminate warnings and add prototypes.
 *
 * Revision 1.3  2000/07/26 01:49:49  malexander
 * (readprofile): Expand ~ in filename.
 *
 * Revision 1.2  2000/07/21 16:20:32  malexander
 * Reformatted with GNU indent.
 *
 * Revision 1.1.1.1  2000/07/14 19:23:10  malexander
 * Imported sources
 *
 * Revision 1.4  91/02/06  09:27:26  alexande
 * Use egetfname() instead of ereply() when prompting for profile name,
 * to allow auto completion of filename.
 * 
 * Revision 1.3  91/01/07  10:30:01  alexande
 * Remove C++ warnings.
 * 
 * Revision 1.2  89/01/13  13:01:09  MGA
 * Added ungetinp() routine to push back input.  Used by incremental search.
 * 
 * Revision 1.1  89/01/13  10:01:44  MGA
 * Initial revision
 * 
 */
#include	"def.h"

/*
 * Next token from profile input: a sequence of MicroEMACS 32-bit
 * keyboard values.
 */
#define	PSIZE	128		/* maximum token size   */
static int ptoken[PSIZE];	/* profile token        */
static int plength;		/* length of token      */
static int pindex;		/* current token index  */

/*
 * Table of commonly used non-printable ascii characters, including
 * their names and MicroEMACS internal key values.
 */
#define	SPECSIZE 5		/* no. of table items   */
static struct
{
  char *st_name;		/* its name             */
  int st_value;			/* the uEMACS value     */
}
spectab[SPECSIZE] =
{
  {
  "Tab", KCTRL | 'I'}
  ,
  {
  "Return", KCTRL | 'M'}
  ,
  {
  "Backspace", KCTRL | 'H'}
  ,
  {
  "Space", 0x20}
  ,
  {
  "Rubout", 0x7f}
};

/*
 * One character can be put back in the input stream,
 * using ungetinp().  The flag ungetflag is zero if there is
 * no character put back.
 */
static int ungetflag;		/* is ungetchar valid?          */
static int ungetchar;		/* the character put back       */

/*
 * Helper function for getkey.
 */
static int
getctl (void)
{
  register int c;

  c = getinp ();
  if (CISLOWER (c) != FALSE)
    c = CTOUPPER (c);
  if (c >= 0x00 && c <= 0x1F)	/* Relocate control.    */
    c = KCTRL | (c + '@');
  return (c);
}

/*
 * Read in a key, doing the terminal
 * independent prefix handling. The terminal specific
 * "getkbd" routine gets the first swing, and may return
 * one of the special codes used by the special keys
 * on the keyboard. The "getkbd" routine returns the
 * C0 controls as received; this routine moves them to
 * the right spot in 32 bit code.
 */
int
getkey (void)
{
  register int c;

  if (ungetflag)		/* is key put back?     */
    {
      ungetflag = FALSE;	/* it's no longer valid */
      c = ungetchar;		/* get that key         */
    }
  else
    c = getinp ();		/* read from input	*/
  if (c == METACH)		/* M-                   */
    c = KMETA | getctl ();
  else if (c == CTRLCH)		/* C-                   */
    c = KCTRL | getctl ();
  else if (c == CTMECH)		/* C-M-                 */
    c = KCTRL | KMETA | getctl ();
  else if (c >= 0x00 && c <= 0x1F)	/* Relocate control.    */
    c = KCTRL | (c + '@');
  if (c == (KCTRL | 'X'))	/* C-X                  */
    c = KCTLX | getctl ();
  return (c);
}

/*
 * Put back one character into the input stream.  The
 * key will be picked up by the next getkey() call.
 */
void
ungetinp (int c)
{
  ungetchar = c;		/* save it              */
  ungetflag = TRUE;
}

/*
 * Transform a key code into a name,
 * using a table for the special keys and combination
 * of some hard code and some general processing for
 * the rest. None of this code is terminal specific any
 * more. This makes adding keys easier.
 */
void
ekeyname (char *cp, int k)
{
  register char *np = "???";
  char nbuf[3];
  register int i;

  static char hex[] = {
    '0', '1', '2', '3',
    '4', '5', '6', '7',
    '8', '9', 'A', 'B',
    'C', 'D', 'E', 'F'
  };

  if ((k & KCTLX) != 0)
    {				/* C-X prefix.          */
      *cp++ = 'C';
      *cp++ = '-';
      *cp++ = 'X';
      *cp++ = ' ';
      k &= ~KCTLX;
    }
  if ((k & KMETA) != 0)
    {				/* Add M- (escape) prefix. */
      *cp++ = 'M';
      *cp++ = '-';
      k &= ~KMETA;
    }
  if ((k & KCHAR) >= KFIRST && (k & KCHAR) <= KLAST)
    {
      if ((np = keystrings[(k & KCHAR) - KFIRST]) != NULL)
	{
	  if ((k & KCTRL) != 0)
	    {
	      *cp++ = 'C';
	      *cp++ = '-';
	    }
	  strcpy (cp, np);
	  return;
	}
    }

  /* Look it up in the special table.
   */
  for (i = 0; i < SPECSIZE; i++)
    {
      if (k == spectab[i].st_value)
	{
	  np = spectab[i].st_name;
	  break;
	}
    }

  if (i == SPECSIZE)
    {				/* Not a special        */
      if ((k & KCTRL) != 0)
	{			/* Add C- mark.         */
	  *cp++ = 'C';
	  *cp++ = '-';
	}
      np = &nbuf[0];
      if (((k & KCHAR) >= 0x20 && (k & KCHAR) <= 0x7E)
	  || ((k & KCHAR) >= 0xA0 && (k & KCHAR) <= 0xFE))
	{
	  nbuf[0] = k & KCHAR;	/* Graphic.             */
	  nbuf[1] = 0;
	}
      else
	{			/* Non graphic.         */
	  nbuf[0] = hex[(k >> 4) & 0x0F];
	  nbuf[1] = hex[k & 0x0F];
	  nbuf[2] = 0;
	}
    }
  strcpy (cp, np);
}


/*
 * Exit the current profile, or do nothing if we aren't reading a profile.
 */
void
exitprofile ()
{
  if (!inprof)			/* not in a profile?    */
    return;			/* do nothing           */
  ffpclose ();			/* close profile        */
  inprof = FALSE;		/* turn off profile flag */
  enoecho = FALSE;		/* enable echo line     */
}


/*
 * Get next character from the profile.  If there is an error,
 * display an error message, and return FALSE.
 */
static int
getpchar (char *cptr)
{
  int s;

  if ((s = ffpread (cptr)) != FIOSUC)
    {				/* read next byte OK?   */
      if (s == FIOERR)
	eprintf ("Profile read error.");
      return (FALSE);
    }
  else
    {
      *cptr &= 0xff;
      return (TRUE);		/* return profile byte  */
    }
}

/*
 * Get quoted string token from profile.  The string is stored
 * in ptoken, and its size in plength.
 */
static int
getpstring ()
{
  char c;

  if (!getpchar (&c))
    return (FALSE);
  while (c != '"')
    {				/* read to next quote   */
      if (c == '\\')
	{			/* escape code?         */
	  if (!getpchar (&c))
	    return (FALSE);
	  switch (c)
	    {
	    case 'r':
	      c = '\r';
	      break;
	    case 'n':
	      c = '\n';
	      break;
	    case 't':
	      c = '\t';
	      break;
	    case 'b':
	      c = '\b';
	      break;
	    case 'f':
	      c = '\f';
	      break;
	    }
	}
      if (plength == PSIZE)
	{
	  eprintf ("Token longer than %d bytes in profile.", PSIZE);
	  return (FALSE);
	}
      ptoken[plength++] = c;	/* store the character  */
      if (!getpchar (&c))	/* get the next char    */
	return (FALSE);
    }

  if (plength == 0)
    {
      eprintf ("Zero-length string in profile.");
      return (FALSE);
    }
  else
    return (TRUE);
}

/*
 * Check if a profile character is a white space character.
 */
static int
prowhite (char c)
{
  return (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == 0x1a);
}

/*
 * Check if the token array at pindex is equal to the specified string.
 * This is tricky because the token is an array of ints, not chars.
 * This routine ignores case differences.  If there is a match,
 * pindex is advanced beyond the matched portion, and TRUE is returned.
 * The toklen parameter contains the number of characters in the token
 * array that must match the string.
 */
static int
eqtoken (
     char *s,			/* string to compare    */
     int toklen)		/* size of token        */
{
  register int i, c1, c2;

  if (strlen (s) != toklen)	/* sizes must match     */
    return (FALSE);
  while (prowhite (ptoken[pindex]) && pindex < plength)
    pindex++;			/* skip whitespace      */
  if (pindex + toklen > plength)	/* size too large?      */
    return (FALSE);
  for (i = pindex; *s; s++, i++)
    {
      if (!CEQ (ptoken[i], *s))
	{			/* chars don't match?   */
	  c1 = ptoken[i];	/* fold case            */
	  if (c1 >= 'a' && c1 <= 'z')
	    c1 = CTOUPPER (c1);
	  c2 = *s;
	  if (c2 >= 'a' && c2 <= 'z')
	    c2 = CTOUPPER (c2);
	  if (c1 != c2)
	    return (FALSE);	/* they don't match     */
	}
    }
  pindex = i;			/* they match!          */
  return (TRUE);
}

/*
 * Get key name token from profile.  The token is converted to a single
 * word in ptoken[0] containing the internal key value.  The exception
 * to this is control keys C-@ through C-_, which are given their
 * normal ASCII values ( 0x00 to 0x1f).  This allows routines that
 * read literal strings, such as searches, to get true control characters.
 */
static int
getpkey (void)
{
  char c;
  register int key, i, found, left;
  static char asciiname[PSIZE + 1];

  if (!getpchar (&c))
    return (FALSE);
  while (c != ']')
    {				/* read to next quote   */
      if (plength == PSIZE)
	{
	  eprintf ("Key name longer than %d bytes in profile.", PSIZE);
	  return (FALSE);
	}
      asciiname[plength] = c;	/* save it for message  */
      ptoken[plength++] = c;	/* store the character  */
      if (!getpchar (&c))	/* get the next char    */
	return (FALSE);
    }

  if (plength == 0)
    {
      eprintf ("Zero-length key name in profile.");
      return (FALSE);
    }
  asciiname[plength] = 0;

  /* Parse the key name; first look for "C-X", "C-", and "M-" prefixes
   */
  key = 0;
  pindex = 0;
  if (eqtoken ("c-x", 3))
    key |= KCTLX;
  if (eqtoken ("m-", 2))
    key |= KMETA;
  if (eqtoken ("c-", 2))
    key |= KCTRL;

  /* We're past the prefixes, presumably.  Look up the key name
   * in the keystrings table.  If not found, it must be a simple
   * single ascii character.
   */
  while (prowhite (ptoken[pindex]) && pindex < plength)
    pindex++;			/* skip whitespace      */
  left = plength - pindex;
  found = FALSE;
  for (i = 0; i < 32; i++)
    {
      if (keystrings[i] != NULL)
	{
	  if (eqtoken (keystrings[i], left))
	    {
	      key |= (KFIRST + i);
	      found = TRUE;
	      break;
	    }
	}
    }

  /* If we can't find it in keystrings, look in our special
   * table of commonly used ascii character that aren't printable.
   */
  if (!found)
    {				/* not found?   */
      for (i = 0; i < SPECSIZE; i++)
	{			/* try spectab  */
	  if (eqtoken (spectab[i].st_name, left))
	    {
	      key |= spectab[i].st_value;
	      found = TRUE;
	      break;
	    }
	}
    }

  /* Key name not found in either table, must be single character.
   */
  if (!found)
    {
      if (left != 1)
	{
	  eprintf ("Bad key name in profile: %s", asciiname);
	  return (FALSE);
	}
      key |= upmap[ptoken[pindex]];	/* OR in the ascii key  */
    }

  /* Store the key value as a one-word token.  If it's a control
   * key in the range C-@ to C-_, convert back to true ASCII.
   */
  if (key >= (KCTRL | '@') && key <= (KCTRL | '_'))
    key -= KCTRL | '@';
  ptoken[0] = key;
  pindex = 0;
  plength = 1;
  return (TRUE);
}

/*
 * Get a numeric argument token from the profile, prefix it with Ctrl-U.
 */
static int
getpnum (char c)
{
  ptoken[0] = KCTRL | 'U';
  plength = 1;
  while (!prowhite (c))
    {
      if (plength == PSIZE)
	{
	  eprintf ("Number longer than %d bytes in profile.", PSIZE);
	  return (FALSE);
	}
      ptoken[plength++] = c;	/* store the character  */
      if (!getpchar (&c))	/* get the next char    */
	return (FALSE);
    }
  return (TRUE);
}

/*
 * Get a function name token from the profile, prefix it with ESC-X
 * and postfix it with CARRIAGE RETURN, so that it will get executed
 * as an extended command.
 */
static int
getpfunc (char c)
{
  ptoken[0] = KMETA | 'X';
  plength = 1;
  while (!prowhite (c))
    {
      if (plength == PSIZE - 1)
	{
	  eprintf ("Function name longer than %d bytes in profile.",
		   PSIZE - 2);
	  return (FALSE);
	}
      ptoken[plength++] = c;	/* store the character  */
      if (!getpchar (&c))	/* get the next char    */
	return (FALSE);
    }
  ptoken[plength++] = '\r';
  return (TRUE);
}

/*
 * Get next token from profile, store it in ptoken.
 * If there is an error, return FALSE.
 */
static int
getptoken (void)
{
  char c;

  for (;;)
    {				/* skip white space     */
      if (!getpchar (&c))	/* get next character   */
	return (FALSE);		/* error                */
      if (!prowhite (c))
	break;
    }
  pindex = plength = 0;
  if (c == '"')			/* quoted string?       */
    return (getpstring ());
  else if (c == '[')
    return (getpkey ());
  else if (c == '-' || (c >= '0' && c <= '9'))
    return (getpnum (c));
  else
    return (getpfunc (c));
}

/*
 * Get the next input character from the profile file, if any, or from
 * the keyboard if no profile is currently active.
 */
int
getinp (void)
{
  if (!inprof)			/* not in a profile?    */
    return (getkbd ());		/* read keyboard        */

  if (pindex >= plength)
    {				/* time to read token?  */
      if (!getptoken ())
	{			/* no more tokens?      */
	  exitprofile ();	/* close the profile    */
	  update ();		/* restore cursor       */
	  return (getkbd ());	/* read keyboard        */
	}
    }
  return (ptoken[pindex++]);	/* next token char      */
}

/*
 * Prompt for the name of a profile file, then open it for reading.
 * Then read bytes from the file in place of keyboard input, until
 * the end of the file is seen.
 */
int
readprofile (int f, int n, int k)
{
  int		s;
  char		fname[NFILEN];
  char *	expanded_fname;

  if (inprof)
    {				/* already in profile?  */
      eprintf ("Can't nest profiles.");
      return (FALSE);
    }
  if ((s = egetfname ("Read profile: ", fname, NFILEN)) != TRUE)
    return (s);
  adjustcase (fname);
  expanded_fname = fftilde (fname);
  s = ffpopen (expanded_fname);
  if (s != FIOSUC)
    {				/* able to open file?   */
      eprintf ("Unable to open profile %s", fname);
      return (FALSE);
    }
  inprof = TRUE;
  enoecho = TRUE;		/* disable echo line    */
  pindex = plength = 0;		/* force a getptoken()  */
  return (TRUE);
}
