/*
    Copyright (C) 2018 Mark Alexander

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

#include	"def.h"

#define N_UNDO 100

static UNDO ustack[N_UNDO];	/* undo records				*/
static int unext = 0;		/* next unused entry in u		*/
static int undoing = FALSE;	/* currently undoing an operation? 	*/

static void
freeundo(UNDO *up)
{
  if (up->kind == USTR)
    free(up->u.str.s);
  up->kind = UUNUSED;
}


static int
lineno (const LINE *lp)
{
  LINE *clp;
  int nline;

  clp = lforw (curbp->b_linep);	/* Collect the data.    */
  nline = 0;
  for (;;)
    {
      if (clp == curbp->b_linep || clp == lp)
	break;
      clp = lforw (clp);
      ++nline;
    }
  return nline;
}


int
saveundo (UKIND kind, ...)
{
  va_list ap;
  UNDO *up;

  if (undoing)
    return TRUE;
  va_start(ap, kind);
  up = &ustack[unext];
  freeundo(up);
  unext = (unext + 1) % N_UNDO;

  up->kind = kind;
  switch (kind)
    {
    case UMOVE:			/* Move to (line #, offset)	*/
      {
	const LINE *lp = va_arg (ap, const LINE *);
	up->u.move.l = lineno (lp);		/* Line number		*/
        up->u.move.o = va_arg (ap, int);	/* Offset		*/
        break;
      }

    case UCH:				/* Insert character	*/
      up->u.ch.n = va_arg (ap, int);	/* Count		*/
      up->u.ch.c = va_arg (ap, int);	/* Character		*/
      break;

    case USTR:			/* Insert string		*/
      {
	int n = va_arg(ap, int);
	const uchar *s = va_arg (ap, const uchar *);

	up->u.str.s = (uchar *) malloc (n);
	if (up->u.str.s == NULL)
	  {
	  eprintf("Out of memory in undo!");
	  return FALSE;
	  }
	memcpy (up->u.str.s, s, n);
	up->u.str.n = n;
	break;
      }
    case UDEL:			/* Delete N characters		*/
      up->u.del.n = va_arg (ap, int);
      break;

    case USTART:
    case UEND:
      break;

    default:
      eprintf ("Unimplemented undo type %d", kind);
      break;
    }

  va_end(ap);
  return TRUE;
}

static int
undostep(UNDO *up)
{
  int status;

  switch (up->kind)
    {
    case UMOVE:
      status = gotoline (TRUE, up->u.move.l + 1, KRANDOM);
      curwp->w_dot.o = up->u.move.o;
      curwp->w_flag |= WFMOVE;
      break;

    case UCH:
      if (up->u.ch.c == '\n')
	status = newline (FALSE, up->u.ch.n, KRANDOM);
      else
	status = linsert (up->u.ch.n, up->u.ch.c, NULL);
      break;

    case USTR:
      {
	const uchar *s = up->u.str.s;
	const uchar *end = s + up->u.str.n;
	status = TRUE;

	while (status == TRUE && s < end)
	  {
	    const uchar *nl = memchr(s, '\n', end - s);
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
	break;
      }

    case UDEL:
      status = ldelete (up->u.del.n, FALSE);
      break;

    default:
      eprintf ("Unknown undo kind %d", up->kind);
      status = FALSE;
      break;
    }
  freeundo(up);
  return status;
}

int
undo (int f, int n, int k)
{
  UNDO *up;
  int nx;
  int status;
  int multi = FALSE;

  /* Pop the last undo record from the stack.  Execute it
   * if it is a single step; or if it is a multi-step, find
   * the beginning of the multi-step sequence.
   */
  undoing = TRUE;
  while (TRUE)
    {
      if (unext == 0)
	nx = N_UNDO;
      else
	nx = unext - 1;
      up = &ustack[nx];
      if (up->kind == UUNUSED)
	{
          eprintf ("Undo stack is empty.");
	  undoing = FALSE;
          return FALSE;
	}
      unext = nx;
      if (up->kind == UEND)
	{
	  /* End of a multi-step undo: keep moving back until USTART is reached */
	  multi = TRUE;
	}
      else if (up->kind == USTART)
	{
	  /* Reached start of multi-step undo: we can stop now and replay the steps. */
	  freeundo (up);
	  break;
	}
      else if (!multi)
	{
	  /* Execute a single undo step. */
	  status = undostep (up);
	  undoing = FALSE;
	  return status;
	}
    }

  /* Replay each step of a multi-step undo in the order
   * in which they were pushed on the stack.
   */
  for (++up; up->kind != UEND; ++up)
    {
      status = undostep (up);
      if (status != TRUE)
	{
	  undoing = FALSE;
	  return status;
	}
    }
  freeundo (up);
  undoing = FALSE;
  return TRUE;
}

static void
printone (UNDO *up)
{
  switch (up->kind)
    {
    case UCH:
      if (up->u.ch.c == '\n')
	printf ("  Char: NEWLINE");
      else
	printf ("  Char: '%c'", up->u.ch.c);
      printf (", n = %d\r\n", up->u.ch.n);
      break;

    case USTR:
      {
	const uchar *s;
	int n;

	printf ("  String: '");
	for (s = up->u.str.s, n = up->u.str.n; n > 0; --n, ++s)
	  {
	    uchar c = *s;
	    if (c == '\n')
	      printf ("\\n");
	    else
	      printf ("%c", c);
	  }
	printf ("'\r\n");
        break;
      }

    case UMOVE:
      printf ("  Move: line %d, offset %d\r\n", up->u.move.l, up->u.move.o);
      break;

    case UDEL:
      printf ("  Delete: %d characters\r\n", up->u.del.n);
      break;

    default:
      printf ("  Unexpected kind %d\r\n", up->kind);
      break;
    }
}

void
printundo(void)
{
  int level;
  UNDO *up;

  up = unext == 0 ? &ustack[N_UNDO] : &ustack[unext - 1];
  level = 1;
  while (up->kind != UUNUSED)
    {
      printf("%d:\r\n", level);
      if (up->kind == UEND)
	{
	  UNDO *up1;
	  for (--up; up->kind != USTART; --up)
	    ;
	  for (up1 = up + 1; up1->kind != UEND; ++up1)
	    printone(up1);
	}
      else
	printone (up);
      --up;
      ++level;
    }
}
