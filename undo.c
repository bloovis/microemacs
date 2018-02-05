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

/* Flags to add to undo kind to indicate start and end of sequence.	*/
#define USTART	0x100
#define UEND	0x200

#define ukind(up)  (up->kind & 0xff)
#define ustart(up) ((up->kind & USTART) != 0)
#define uend(up)   ((up->kind & UEND) != 0)

#define NOLINE  -1		/* UNDO.{l,o} value meaing not used	*/

static UNDO ustack[N_UNDO];	/* undo records				*/
static UNDO *uptr = ustack;	/* ptr to next unused entry in ustack	*/
static UNDO *startup;		/* ptr saved by startundo		*/
static int undoing = FALSE;	/* currently undoing an operation? 	*/

static void
freeundo(UNDO *up)
{
  if (ukind (up) == USTR)
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


static UNDO *
unext (UNDO *up)
{
  if (up == &ustack[N_UNDO - 1])
    return &ustack[0];
  else
    return up + 1;
}

static UNDO *
uprev (UNDO *up)
{
  if (up == &ustack[0])
    return &ustack[N_UNDO - 1];
  else
    return up - 1;
}

void
startundo (void)
{
  startup = uptr;
}


void
endundo (void)
{
  UNDO *up;

  if (uptr != startup)
    {
      up = uprev (uptr);
      up->kind |= UEND;
    }
  startup = NULL;
}


int
saveundo (UKIND kind, POS *pos, ...)
{
  va_list ap;
  UNDO *up;

  if (undoing)
    return TRUE;
  va_start(ap, pos);
  up = uptr;
  freeundo(up);
  uptr = unext (uptr);

  up->kind = kind;
  if (pos != NULL)
    {
      up->l = lineno (pos->p);	/* Line number		*/
      up->o = pos->o;		/* Offset		*/
    }
  else
    {
      up->l = NOLINE;
    }

  if (up == startup)
    {
      up->kind |= USTART;
      startup = FALSE;
    }
  switch (kind)
    {
    case UMOVE:			/* Move to (line #, offset)	*/
      break;

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
	  eprintf ("Out of memory in undo!");
	  return FALSE;
	  }
	memcpy (up->u.str.s, s, n);
	up->u.str.n = n;
	break;
      }
    case UDEL:			/* Delete N characters		*/
      {
	int n = va_arg (ap, int);
	UNDO *prev = uprev (up);
        if (ukind (prev) == UDEL &&
	    prev->l == up->l &&
	    prev->o + prev->u.del.n == up->o)
	  {
	    prev->u.del.n += n;
	    uptr = up;
	  }
	else
	  {
	    up->u.del.n = n;
	    break;
	  }
      }
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
  int status = TRUE;

  if (up->l != NOLINE)
    {
      status = gotoline (TRUE, up->l + 1, KRANDOM);
      curwp->w_dot.o = up->o;
      curwp->w_flag |= WFMOVE;
    }

  if (status == TRUE)
    {
      switch (ukind (up))
	{
	case UMOVE:
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
	  eprintf ("Unknown undo kind 0x%x", up->kind);
	  status = FALSE;
	  break;
	}
    }

  freeundo(up);
  return status;
}

int
undo (int f, int n, int k)
{
  UNDO *up;
  UNDO *end;
  int status = TRUE;

  /* Step back in the undo stack until we find the start
   * a sequence.
   */
  undoing = TRUE;
  up = uprev (uptr);
  end = up;
  while (ustart (up) != TRUE)
    {
      if (up->kind == UUNUSED)
	{
          eprintf ("Undo stack is empty.");
	  undoing = FALSE;
          return FALSE;
	}
      up = uprev (up);
    }
  uptr = up;

  /* Replay all steps of a multi-step undo in the order
   * in which they were pushed on the stack.
   */
  while (TRUE)
    {
      int st = undostep (up);

      if (st != TRUE)
	status = st;
      if (up == end)
	break;
      up = unext (up);
    }

  undoing = FALSE;
  return status;
}

static void
printone (UNDO *up)
{
  switch (ukind (up))
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
      printf ("  Move: line %d, offset %d\r\n", up->l, up->o);
      break;

    case UDEL:
      printf ("  Delete: %d characters at line %d, offset %d\r\n",
              up->u.del.n, up->l, up->o);
      break;

    default:
      printf ("  Unexpected kind 0x%x\r\n", up->kind);
      break;
    }
}

void
printundo(void)
{
  int level;
  UNDO *up;
  UNDO *up1;

  up = uprev (uptr);
  level = 1;
  while (up->kind != UUNUSED && up != uptr)
    {
      while (ustart (up) != TRUE)
	{
	  up = uprev (up);
	  if (up->kind == UUNUSED)
	    {
	      eprintf("Error finding start of undo sequence!");
	      return;
	    }
	}
      printf("%d:\r\n", level);
      up1 = up;
      while (TRUE)
	{
	  printone (up1);
	  if (uend (up1) == TRUE)
	    break;
	  up1 = unext (up1);
	}
      ++level;
      up = uprev (up);
    }
}
