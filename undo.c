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

static UNDO ustack[N_UNDO];	/* undo records			*/
static int unext = 0;		/* next unused entry in u	 */

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
      ++nline;
    }
  return nline;
}


int
saveundo (UKIND kind, ...)
{
  va_list ap;
  UNDO *up;

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
	up->u.move.l = lineno (lp);	/* Line number		*/
        up->u.move.o = va_arg(ap, int);	/* Offset		*/
        break;
      }

    case UCH:				/* Insert character	*/
      up->u.ch.n = va_arg(ap, int);	/* Count		*/
      up->u.ch.c = va_arg(ap, int);	/* Character		*/
      break;

    case USTR:			/* Insert string		*/
      {
	const uchar *s = va_arg(ap, const uchar *);
	up->u.str.s = (uchar *)strdup((const char *)s);
	if (up->u.str.s == NULL)
	  {
	  eprintf("Out of memory in undo!");
	  return FALSE;
	  }
	break;
      }
    case UDEL:			/* Delete N characters		*/
      up->u.del.n = va_arg(ap, int);
      break;

    case USTART:
    case UEND:
      break;

    default:
      eprintf("Unimplemented undo type %d", kind);
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
      status = gotoline(TRUE, up->u.move.l + 1, 0);
      curwp->w_dot.o = up->u.move.o;
      curwp->w_flag |= WFMOVE;
      break;
    case UCH:
      status = linsert(up->u.ch.n, up->u.ch.c, NULL);
      break;
    case USTR:
      status = linsert(up->u.ch.n, 0, (char *)up->u.str.s);
      break;
    case UDEL:
      status = ldelete(up->u.del.n, FALSE);
      break;
    default:
      eprintf("Unknown undo kind %d", up->kind);
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
  while (TRUE)
    {
      if (unext == 0)
	nx = N_UNDO;
      else
	nx = unext - 1;
      up = &ustack[nx];
      if (up->kind == UUNUSED)
	{
          eprintf("Undo stack is empty.");
          return FALSE;
	}
      unext = nx;
      if (up->kind == UEND)
	/* End of a multi-step undo: keep moving back until USTART is reached */
	multi = TRUE;
      else if (up->kind == USTART)
	/* Reached start of multi-step undo: we can stop now and replay the steps. */
	break;
      else if (!multi)
        /* Execute a single undo step. */
	return undostep(up);
    }

  /* Replay each step of a multi-step undo in the order
   * in which they were pushed on the stack.
   */
  for (++up; up->kind != UEND; ++up)
    {
      status = undostep(up);
      if (status != TRUE)
	return status;
    }
  return TRUE;
}
