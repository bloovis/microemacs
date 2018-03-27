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
 *		Word mode commands.
 * Version:	29
 * Last edit:	15-Jul-86
 * By:		rex::conroy
 *		decvax!decwrl!dec-rhea!dec-rex!conroy
 * Modified by:	Mark Alexander
 *		drivax!alexande
 *
 * The routines in this file
 * implement commands that work word at
 * a time. There are all sorts of word mode
 * commands. If I do any sentence and/or paragraph
 * mode commands, they are likely to be put in
 * this file.
 */
#include	"def.h"

/*
 * Move the cursor backward by
 * "n" words. All of the details of motion
 * are performed by the "backchar" and "forwchar"
 * routines. Error if you try to move beyond
 * the buffers.
 */
int
backword (int f, int n, int k)
{
  if (n < 0)
    return (forwword (f, -n, KRANDOM));
  if (backchar (FALSE, 1, KRANDOM) == FALSE)
    return (FALSE);
  while (n--)
    {
      while (inword () == FALSE)
	{
	  if (backchar (FALSE, 1, KRANDOM) == FALSE)
	    return (FALSE);
	}
      while (inword () != FALSE)
	{
	  if (backchar (FALSE, 1, KRANDOM) == FALSE)
	    return (FALSE);
	}
    }
  return (forwchar (FALSE, 1, KRANDOM));
}

/*
 * Move the cursor forward by
 * the specified number of words. All of the
 * motion is done by "forwchar". Error if you
 * try and move beyond the buffer's end.
 */
int
forwword (int f, int n, int k)
{
  if (n < 0)
    return (backword (f, -n, KRANDOM));
  while (n--)
    {
      while (inword () == FALSE)
	{
	  if (forwchar (FALSE, 1, KRANDOM) == FALSE)
	    return (FALSE);
	}
      while (inword () != FALSE)
	{
	  if (forwchar (FALSE, 1, KRANDOM) == FALSE)
	    return (FALSE);
	}
    }
  return (TRUE);
}

/*
 * Move the cursor forward by
 * the specified number of words. As you move,
 * convert any characters to upper case. Error
 * if you try and move beyond the end of the
 * buffer.
 */
int
upperword (int f, int n, int k)
{
  register int c;

  if (n < 0)
    return (FALSE);
  if (checkreadonly () == FALSE)
    return FALSE;
  saveundo (UMOVE, &curwp->w_dot);
  while (n--)
    {
      while (inword () == FALSE)
	{
	  if (forwchar (FALSE, 1, KRANDOM) == FALSE)
	    return (FALSE);
	}
      while (inword () != FALSE)
	{
	  c = wlgetc (curwp->w_dot.p, curwp->w_dot.o);
	  if (CISLOWER (c) != FALSE)
	    {
	      c = CTOUPPER (c);
	      lputc (curwp->w_dot, c);
	      lchange (WFHARD);
	    }
	  else if (forwchar (FALSE, 1, KRANDOM) == FALSE)
	    return (FALSE);
	}
    }
  return (TRUE);
}

/*
 * Move the cursor forward by
 * the specified number of words. As you move
 * convert characters to lower case. Error if you
 * try and move over the end of the buffer.
 */
int
lowerword (int f, int n, int k)
{
  register int c;

  if (n < 0)
    return (FALSE);
  if (checkreadonly () == FALSE)
    return FALSE;
  saveundo (UMOVE, &curwp->w_dot);
  while (n--)
    {
      while (inword () == FALSE)
	{
	  if (forwchar (FALSE, 1, KRANDOM) == FALSE)
	    return (FALSE);
	}
      while (inword () != FALSE)
	{
	  c = wlgetc (curwp->w_dot.p, curwp->w_dot.o);
	  if (CISUPPER (c) != FALSE)
	    {
	      c = CTOLOWER (c);
	      lputc (curwp->w_dot, c);
	      lchange (WFHARD);
	    }
	  else if (forwchar (FALSE, 1, KRANDOM) == FALSE)
	    return (FALSE);
	}
    }
  return (TRUE);
}

/*
 * Move the cursor forward by
 * the specified number of words. As you move
 * convert the first character of the word to upper
 * case, and subsequent characters to lower case. Error
 * if you try and move past the end of the buffer.
 */
int
capword (int f, int n, int k)
{
  register int c;

  if (n < 0)
    return (FALSE);
  if (checkreadonly () == FALSE)
    return FALSE;
  saveundo (UMOVE, &curwp->w_dot);
  while (n--)
    {
      while (inword () == FALSE)
	{
	  if (forwchar (FALSE, 1, KRANDOM) == FALSE)
	    return (FALSE);
	}
      if (inword () != FALSE)
	{
	  c = lgetc (curwp->w_dot.p, curwp->w_dot.o);
	  if (CISLOWER (c) != FALSE)
	    {
	      c = CTOUPPER (c);
	      lputc (curwp->w_dot, c);
	      lchange (WFHARD);
	    }
	  else if (forwchar (FALSE, 1, KRANDOM) == FALSE)
	    return (FALSE);
	  while (inword () != FALSE)
	    {
	      c = lgetc (curwp->w_dot.p, curwp->w_dot.o);
	      if (CISUPPER (c) != FALSE)
		{
		  c = CTOLOWER (c);
		  lputc (curwp->w_dot, c);
		  lchange (WFHARD);
		}
	      else if (forwchar (FALSE, 1, KRANDOM) == FALSE)
		return (FALSE);
	    }
	}
    }
  return (TRUE);
}

/*
 * Kill forward by "n" words. The rules for final
 * status are now different. It is not considered an error
 * to delete fewer words than you asked. This lets you say
 * "kill lots of words" and have the command stop in a reasonable
 * way when it hits the end of the buffer. Normally this is
 */
int
delfword (int f, int n, int k)
{
  register int size;
  register LINE *dotp;
  register int doto;

  if (n < 0)
    return (FALSE);
  kdelete ();			/* Purge kill buffer.   */
  dotp = curwp->w_dot.p;
  doto = curwp->w_dot.o;
  size = 0;
  while (n--)
    {
      while (inword () == FALSE)
	{
	  if (forwchar (FALSE, 1, KRANDOM) == FALSE)
	    goto out;		/* Hit end of buffer.   */
	  ++size;
	}
      while (inword () != FALSE)
	{
	  if (forwchar (FALSE, 1, KRANDOM) == FALSE)
	    goto out;		/* Hit end of buffer.   */
	  ++size;
	}
    }
out:
  curwp->w_dot.p = dotp;
  curwp->w_dot.o = doto;
  return (ldelete (size, TRUE));
}

/*
 * Kill backwards by "n" words. The rules
 * for success and failure are now different, to prevent
 * strange behavior at the start of the buffer. The command
 * only fails if something goes wrong with the actual delete
 * of the characters. It is successful even if no characters
 * are deleted, or if you say delete 5 words, and there are
 * only 4 words left. I considered making the first call
 * to "backchar" special, but decided that that would just
 * be wierd. Normally this is bound to "M-Rubout" and
 * to "M-Backspace".
 */
int
delbword (int f, int n, int k)
{
  register int size;

  if (n < 0)
    return (FALSE);
  kdelete ();			/* purge kill buffer    */
  if (backchar (FALSE, 1, KRANDOM) == FALSE)
    return (TRUE);		/* Hit buffer start.    */
  size = 1;			/* One deleted.         */
  while (n--)
    {
      while (inword () == FALSE)
	{
	  if (backchar (FALSE, 1, KRANDOM) == FALSE)
	    goto out;		/* Hit buffer start.    */
	  ++size;
	}
      while (inword () != FALSE)
	{
	  if (backchar (FALSE, 1, KRANDOM) == FALSE)
	    goto out;		/* Hit buffer start.    */
	  ++size;
	}
    }
  if (forwchar (FALSE, 1, KRANDOM) == FALSE)
    return (FALSE);
  --size;			/* Undo assumed delete. */
out:
  saveundo (UMOVE, &curwp->w_dot);
  return (ldelete (size, TRUE));
}

/*
 * Return TRUE if the character at dot
 * is a character that is considered to be
 * part of a word. The word character list is hard
 * coded. Should be setable.
 */
int
inword (void)
{
  if (curwp->w_dot.o == wllength (curwp->w_dot.p))
    return (FALSE);
  if (CISWORD (wlgetc (curwp->w_dot.p, curwp->w_dot.o)) != FALSE)
    return (TRUE);
  return (FALSE);
}
