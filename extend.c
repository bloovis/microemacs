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
 *		Extended (M-X) commands.
 * Version:	29
 * By:		rex::conroy
 *		decvax!decwrl!dec-rhea!dec-rex!conroy
 * Last edit:	24-Mar-88
 * By:		Mark Alexander
 *		drivax!alexande
 */
#include	"def.h"

/*
 * This function modifies the keyboard
 * binding table, by adjusting the entries in the
 * big "bindings" array. Most of the grief deals with the
 * prompting for additional arguments.
 */
int
bindtokey (int f, int n, int k)
{
  register int s;
  register SYMBOL *sp;
  register int c;
  char xname[NXNAME];

  if ((s = ereadv ("Function: ", xname, NXNAME, EFAUTO)) != TRUE)
    return (s);
  if ((sp = symlookup (xname)) == NULL)
    {
      eprintf ("Unknown function for binding");
      return (FALSE);
    }
  if (kbdmop != NULL)		/* Are we in a macro?   */
    c = *kbdmop++;		/* Get key from macro.  */
  else
    {
      eputc (' ');		/* Prompt for the key.  */
      eputc ('K');
      eputc ('e');
      eputc ('y');
      eputc (':');
      eputc (' ');
      ttflush ();
      c = getkey ();		/* Read key.            */
      ekeyname (xname, c);	/* Display ekeyname.     */
      eputs (xname);
      ttflush ();
    }
  if (kbdmip != NULL)
    {				/* Save key in macro.   */
      if (kbdmip > &kbdm[NKBDM - 2])
	{
	  ctrlg (FALSE, 0, KRANDOM);
	  return (FALSE);
	}
      *kbdmip++ = c;
    }
  setbinding (c, sp);
  return (TRUE);
}

/*
 * Extended command. Call the message line
 * routine to read in the command name and apply autocompletion
 * to it. When it comes back, look the name up in the symbol table
 * and run the command if it is found and has the right type.
 * Print an error if there is anything wrong.
 */
int
extend (int f, int n, int k)
{
  register SYMBOL *sp;
  register int s;
  char xname[NXNAME];

  if ((s = ereadv (": ", xname, NXNAME, EFNEW | EFAUTO)) != TRUE)
    return (s);
  if ((sp = symlookup (xname)) != NULL)
    {
      if (sp->s_macro)
	{
	  if (kbdmip != NULL || kbdmop != NULL)
	    {
	      eprintf ("Not now");
	      return (FALSE);
	    }
	  return (domacro (sp->s_macro, n));
	}
#if USE_RUBY
      else if (sp->s_funcp == NULL)
	return rubycall (sp->s_name, f, n);
#endif
      else
	return ((*sp->s_funcp) (f, n, KRANDOM));
    }
  eprintf ("Unknown extended command");
  return (ABORT);
}

/*
 * Read a key from the keyboard, and look it
 * up in the binding table. Display the name of the function
 * currently bound to the key. Say that the key is not bound
 * if it is indeed not bound, or if the type is not a
 * "builtin". This is a bit of overkill, because this is the
 * only kind of function there is.
 */
int
help (int f, int n, int k)
{
  register SYMBOL *sp;
  register int c;
  char b[20];

  c = getkey ();
  ekeyname (b, c);
  if ((sp = getbinding (c)) == NULL)
    eprintf ("[%s is unbound]", b);
  else
    eprintf ("[%s is bound to %s]", b, sp->s_name);
  return (TRUE);
}

/*
 * Variables used by the insert-macro command.
 * Save is a buffer of characters to be inserted
 * into the current buffer; sindex is the current
 * buffer index.  Stringlen is the length of a
 * quoted string that is currently being inserted
 * into the buffer.
 */
static char save[32];		/* Saved string.                */
static int sindex;		/* Index into save.             */
static int stringlen;		/* Length of quoted string      */

/*
 * Flush the save buffer to the current buffer.
 */
static int
flush (void)
{
  if (sindex == 0)
    return (TRUE);
  if (linsert (sindex, 0, save) == FALSE)
    return (FALSE);
  sindex = 0;
  return (TRUE);
}

/*
 * Write a character to the save buffer, and
 * flush it if the buffer is full.
 */
static int
outchar (char c)
{
  if (sindex == sizeof (save))	/* Buffer full?         */
    if (flush () == FALSE)	/* Insert the whole lot */
      return (FALSE);
  save[sindex++] = c;
  return (TRUE);
}

/*
 * Write a null-terminated string to the save buffer.
 */
static int
outbuf (const char *s)
{
  register char c;

  while ((c = *s++) != 0)
    if (outchar (c) == FALSE)
      return (FALSE);
  return (TRUE);
}

/*
 * Write a space character to the current buffer,
 * do a new line if we're past the fill column.
 * Also, flush the buffer.
 */
static int
outspace (void)
{
  if (flush () == FALSE)
    return (FALSE);
  if (getcolpos () >= fillcol)
    return (lnewline ());
  else
    return (linsert (1, ' ', NULLPTR));
}

/*
 * Write a decimal number to the save buffer.
 */
static int
outdec (int n)
{
  static char buf[5];
  register int i;

  if (n < 0)
    {
      n = -n;
      if (outchar ('-') == FALSE)
	return (FALSE);
    }
  for (i = 6; i > 0;)
    {
      buf[--i] = (n % 10) + '0';
      if ((n /= 10) == 0)
	break;
    }
  while (i < 6)
    if (outchar (buf[i++]) == FALSE)
      return (FALSE);
  return (outspace ());
}

/*
 * Write one character of a quoted string to
 * the save buffer.
 */
static int
outstrchr (char c)
{
  if (stringlen == 0)		/* empty string?        */
    if (outchar ('"') == FALSE)	/* send opening quote   */
      return (FALSE);
  stringlen++;
  if (c == '"')			/* prefix a quote with an escape */
    if (outchar ('\\') == FALSE)
      return (FALSE);
  return (outchar (c));
}

/*
 * Flush a quoted string to the current buffer.
 */
static int
flushstring (void)
{
  if (stringlen == 0)
    return (TRUE);		/* empty string         */
  stringlen = 0;
  if (outchar ('"') == FALSE)	/* closing quote        */
    return (FALSE);
  if (flush () == FALSE)
    return (FALSE);
  return (outspace ());
}

/*
 * Prompt for a name, and insert the text of the specifed macro into the
 * current buffer.  Return FALSE if an error occurs; otherwise TRUE.
 */
int
insertmacro (int f, int n, int k)
{
  register int *mp;		/* Macro pointer.       */
  char xname[NXNAME + 2];	/* Symbol name.         */
  register SYMBOL *sp = NULL;
  register int named;

  /* Read the name of the symbol to use, store the name in
   * a local array.
   */
  if ((named = ereadv ("Macro name: ", xname, NXNAME, EFAUTO)) == ABORT)
    return (named);
  if (named == FALSE)		/* no name given?       */
    mp = &kbdm[0];		/* use current macro    */
  else
    {
      /* See if the symbol exists, and if it is a named macro.
       */
      if ((sp = symlookup (xname)) == NULL)
	{
	  eprintf ("No such symbol.");
	  return (FALSE);
	}
      if ((mp = sp->s_macro) == NULL)
	{
	  eprintf ("Not a named macro.");
	  return (FALSE);
	}
    }

  /* Insert the text of each character in the macro into the
   * the current buffer.  Put strings of printable ascii
   * characters into quotes; convert other characters
   * into their appropriate names in brackets.
   */
  sindex = 0;
  for (k = KCTLX | '(';; k = *mp++)
    {
      if (k > 0x00 && k <= 0x1F)	/* Relocate control.    */
	k = KCTRL | (k + '@');
      ekeyname (xname, k);	/* Get key name.        */
      if ((k >= 0x20 && k <= 0x7e) || (strlen (xname) <= 1))
	{			/* Simple ASCII?        */
	  if (outstrchr (k) == FALSE)
	    return (FALSE);
	}
      else if (k == 0)
	{			/* Null: end of string  */
	  if (outstrchr ('\\') == FALSE)
	    return (FALSE);
	  if (outstrchr ('r') == FALSE)
	    return (FALSE);
	  if (flushstring () == FALSE)
	    return (FALSE);
	}
      else if (k == (KCTRL | 'U'))
	{			/* Control-U arg prefix */
	  if (flushstring () == FALSE)
	    return (FALSE);	/* Flush prev. string.  */
	  if (outdec (*mp++) == FALSE)
	    return (FALSE);	/* Output argument      */
	}
      else
	{			/* Not a simple key.    */
	  if (flushstring () == FALSE)
	    return (FALSE);	/* Flush prev. string.  */
	  if (outchar ('[') == FALSE)
	    return (FALSE);	/* Opening bracket.     */
	  if (outbuf (xname) == FALSE)
	    return (FALSE);	/* Write out key name,  */
	  if (outchar (']') == FALSE)
	    return (FALSE);	/* Closing bracket.     */
	  if (outspace () == FALSE)
	    return (FALSE);	/* followed by a space. */
	}
      if (k == (KCTLX | ')'))	/* End of macro?        */
	break;
    }
  if (flushstring () == FALSE)	/* Flush last string.   */
    return (FALSE);
  if (named)
    {				/* A named macro?       */
      if (outbuf ("name-macro") == FALSE)
	return (FALSE);		/* Bind macro to name.  */
      if (outspace () == FALSE)
	return (FALSE);		/* followed by a space. */
      if (outchar ('"') == FALSE)	/* Opening quote.       */
	return (FALSE);
      if (outbuf (sp->s_name) == FALSE)	/* Output symbol name.  */
	return (FALSE);
      if (outbuf ("\\r\"") == FALSE)	/* Append carriage ret. */
	return (FALSE);
      if (flush () == FALSE)	/* Flush symbol name.   */
	return (FALSE);
    }
  return (lnewline ());		/* Blank line.          */
}
