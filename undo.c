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

/* Maximum number of undo operations saved. */
#define N_UNDO 100

/* A single undo step, as part of a larger group. */
typedef struct UNDO
{
  UKIND kind;			/* Kind of information		*/
  int l;			/* Line number			*/
  int o;			/* Offset into line		*/
  union
  {
    struct
    {
      int n;			/* # of characters to insert	*/
      uchar c;			/* Character			*/
    } ch;
    struct
    {
      int n;			/* Length of string		*/
      uchar *s;			/* String			*/
    } str;
    struct
    {
      int n;			/* # of characters to delete	*/
    } del;
  } u;
}
UNDO;

typedef struct LINKS
{
  struct LINKS *next;
  struct LINKS *prev;
}
LINKS;

/* Group of UNDO steps, treated as one undo operation. */
typedef struct UNDOGROUP
{
  LINKS links;		/* head = prev group, tail = next group */
  UNDO *undos;		/* array of undo steps */
  int next;		/* next free entry in group */
  int avail;		/* size of group array */
  int b_flag;		/* copy of curbp->b_flag before any changes */
}
UNDOGROUP;

/* Linked list of undo groups, treated as a stack of maximum size N_UNDO */
typedef struct UNDOSTACK
{
  LINKS links;		/* head and tail of group list */
  int ngroups;		/* size of group stack */
}
UNDOSTACK;

#define ukind(up)  (up->kind & 0xff)
#define ustart(up) ((up->kind & USTART) != 0)
#define uend(up)   ((up->kind & UEND) != 0)

#define NOLINE  -1		/* UNDO.{l,o} value meaing not used	*/

static int startl = NOLINE;	/* lineno saved by startundo		*/
static int starto;		/* offset saved by startundo		*/
static int undoing = FALSE;	/* currently undoing an operation? 	*/
static int b_flag;		/* copy of curbp->b_flag		*/

/* Initialize group links */
static void
initlinks (LINKS *links)
{
  links->next = links->prev = links;
}

/*
 * Remove group from list.
 */
static void
unlinkgroup (LINKS *links)
{
  links->prev->next = links->next;
  links->next->prev = links->prev;
}

/*
 * Append new group into list after oldgroup .
 */
static void
appendgroup (LINKS *newlinks, LINKS *oldlinks)
{
  newlinks->prev = oldlinks;
  newlinks->next = oldlinks->next;
  oldlinks->next->prev = newlinks;
  oldlinks->next = newlinks;
}

/*
 * Allocate a new undo group structure.
 */
static UNDOGROUP *
newgroup (void)
{
  UNDOGROUP *g = malloc (sizeof (*g));
  UNDO *u = malloc (sizeof (*u));
  g->undos = u;
  g->next = 0;
  g->avail = 1;
  g->b_flag = b_flag;
  return g;
}

/*
 * Free up an undo stepand any resources associated with it.
 */
static void
freeundo (UNDO *up)
{
  if (ukind (up) == USTR)
    free (up->u.str.s);
  up->kind = UUNUSED;
}


/*
 * Calculate the zero-based line number for a given line pointer.
 */
int
lineno (const LINE *lp)
{
  LINE *clp;
  int nline;

  clp = lforw (curbp->b_linep);
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

/*
 * Allocate a new undo stack structure, but don't
 * add any undo groups to it yet.
 */
static UNDOSTACK *
newstack (void)
{
  UNDOSTACK *st = malloc (sizeof (*st));
  initlinks (&st->links);
  st->ngroups = 0;
  return st;
}

/*
 * Call this at the start of an undo save sequence,
 * i.e. before the first saveundo.  It saves
 * some context about the current location: the
 * line number and offset, and the buffer changed flag.
 */
void
startsaveundo (void)
{
  UNDOSTACK *st;

  st = curwp->w_bufp->b_undo;
  if (st == NULL)
    {
      st = newstack ();
      curwp->w_bufp->b_undo = st;
    }
  startl = lineno (curwp->w_dot.p);
  starto = curwp->w_dot.o;
  b_flag = curbp->b_flag;
  undoing = FALSE;
}


/*
 * Call this at the end of an undo save sequence,
 * i.e. after the last saveundo.  Currently it does
 * nothing, but conceivably it could free up any
 * resources that might have been allocated by
 * startsaveundo and that are not longer needed.
 */
void
endsaveundo (void)
{
}


/*
 * Remove an undo group from its list, then free up
 * its undo records, and finally free the group record itself.
 */
static void
freegroup (UNDOGROUP *g)
{
  UNDO *up, *end;

  /* Remove group from its list.
   */
  unlinkgroup (&g->links);

  /* Free up any resources used by the undo steps in the group.
   */
  end = &g->undos[g->next];
  for (up = &g->undos[0]; up != end; up++)
    freeundo (up);

  /* Free up the undo array.
   */
  free (g->undos);

  /* Set the buffer change flag.
   */
  if (g->b_flag & BFCHG)
    curbp->b_flag |= BFCHG;
  else
    curbp->b_flag &= ~BFCHG;
  curwp->w_flag |= WFMODE;

  /* Finally, free up the group record.
   */
  free (g);
}

/*
 * Return a pointer to the most recently saved undo record,
 * or NULL if there is none.
 */
static UNDO *
lastundo (UNDOSTACK *st)
{
  UNDOGROUP *g;

  g = (UNDOGROUP *) st->links.prev;

  /* Is group list empty?
   */
  if (&g->links == &st->links)
    return NULL;

  /* Is group itself empty?
   */
  if (g->next == 0)
    return NULL;

  /* Return last undo record in group.
   */
  return &g->undos[g->next - 1];
}

/*
 * Allocate a new undo record and return a pointer to it.
 * Initialize its kind, line number, and offset from the
 * passed-in values.  Also create a new undo group for
 * this undostack if this its first undo record.
 */
static UNDO *
newundo (UNDOSTACK *st, UKIND kind, int line, int offset)
{
  UNDO *up;
  UNDOGROUP *g;

  /* If startl has been set by startundo, this is the first
   * undo record for the current command, so allocate a new
   * undo group.
   */
  if (startl != NOLINE)
    {
      /* This is the start of a new undo group.  Create a group
       * and place it at the end of list of groups.
       */
      g = newgroup ();
      appendgroup (&g->links, st->links.prev);

      /* If we've reached the maximum number of undo groups, recycle the
       * first one in the list.
       */
      if (st->ngroups >= N_UNDO)
	freegroup ((UNDOGROUP *) st->links.next);
      else
	st->ngroups++;
    }
  else
    /* This is not the first undo record in a group.  Get
     * the last group in the list
     */
    g = (UNDOGROUP *) st->links.prev;

  /* Do we need to expand the array of undo records in this group?
   */
  if (g->next >= g->avail)
    {
      g->avail = g->avail << 1;
      g->undos = (UNDO *) realloc (g->undos, g->avail * sizeof (UNDO));
    }
  up = &g->undos[g->next];
  g->next++;

  /* Initialize the undo record with its kind, and the specified
   * line number and offset (which may be NOLINE if unknown).
   */
  up->kind = kind;
  up->l = line;
  up->o = offset;

  return up;
}


/*
 * Prevent subsequent saveundo calls from storing data.  Currently
 * not used, but conceivably could be used for code sections
 * that should not be saving undo records.
 */
void
disablesaveundo (void)
{
  undoing = TRUE;
}

/* Allow subsequent saveundo calls to store data.  See disablesaveundo.
 */
void
enablesaveundo (void)
{
  undoing = FALSE;
}

/*
 * Save a single undo record.  The first two parameters are fixed:
 *  - the kind of undo record
 *  - a pointer to a line/offset pair to be recorded in
 *    the record, or NULL if no line/offset pair is to be recorded.
 * Following these two parameters there may be other parameters,
 * depending on the kind:
 *  - UMOVE:
 *    - takes no extra parameters
 *  - UCH:
 *    - count
 *    - character to be inserted
 *  - USTR:
 *    - size of string
 *    - pointer to string (may not be null-terminated)
 *  - UDEL:
 *    - count of characters to be deleted
 */
int
saveundo (UKIND kind, POS *pos, ...)
{
  va_list ap;
  UNDO *up;
  UNDOSTACK *st;
  int line, offset;

  if (undoing)
    return TRUE;
  va_start (ap, pos);

  /* Figure out what line number and offset to use for this undo record.
   * If POS was passed in, calculate the corresponding line number and offset.
   * Otherwise, if this is the first record after a startundo, use the line
   * number and offset saved by startundo.  Otherwise don't use any line
   * number or offset.
   */
  if (pos != NULL)
    {
      line = lineno (pos->p);	/* Line number		*/
      offset = pos->o;		/* Offset		*/
    }
  else if (startl != NOLINE)
    {
      line = startl;
      offset = starto;
    }
  else
    {
      line = NOLINE;
      offset = NOLINE;
    }

  st = curwp->w_bufp->b_undo;

  switch (kind)
    {
    case UMOVE:			/* Move to (line #, offset)	*/
      up = newundo (st, kind, line, offset);
      break;

    case UCH:				/* Insert character	*/
      up = newundo (st, kind, line, offset);
      up->u.ch.n = va_arg (ap, int);	/* Count		*/
      up->u.ch.c = va_arg (ap, int);	/* Character		*/
      break;

    case USTR:			/* Insert string		*/
      {
	int n = va_arg (ap, int);
	const uchar *s = va_arg (ap, const uchar *);

	if (n == 1)
	  {
	    /* Treat single-character strings as a UCH
	     * for efficiency
	     */
	    up = newundo (st, UCH, line, offset);
	    up->u.ch.n = 1;
	    up->u.ch.c = s[0];
	  }
	else
	  {
            up = newundo (st, kind, line, offset);
	    up->u.str.s = (uchar *) malloc (n);
	    if (up->u.str.s == NULL)
	      {
	      eprintf ("Out of memory in undo!");
	      return FALSE;
	      }
	    memcpy (up->u.str.s, s, n);
	    up->u.str.n = n;
	  }
	break;
      }
    case UDEL:			/* Delete N characters		*/
      {
	int n = va_arg (ap, int);
	UNDO *prev = lastundo (st);

	if (prev != NULL &&
            ukind (prev) == UDEL &&
	    prev->l == line &&
	    prev->o + prev->u.del.n == offset)
	  {
	    prev->u.del.n += n;
	  }
	else
	  {
	    up = newundo (st, kind, line, offset);
	    up->u.del.n = n;
	  }
        break;
      }

    default:
      eprintf ("Unimplemented undo type %d", kind);
      break;
    }

  va_end (ap);
  startl = NOLINE;
  return TRUE;
}

/*
 * Undo a single step in a possibly larger sequence of undo records.
 */
static int
undostep (UNDO *up)
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
		const uchar *nl = memchr (s, '\n', end - s);
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

  return status;
}

/*
 * Undo the topmost undo group on the undo stack.
 * Each group consists of a linear sequence of undo steps.
 * This sequence is split into subsequences; the
 * start of each subsequence is any undo record that
 * moves the dot.  These subsequences are processed
 * in reverse order, but within each subsequence,
 * the undo records are processed in forward order.
 * This ordering is necessary to account for any
 * undo sequences that move the dot.
 */
int
undo (int f, int n, int k)
{
  UNDO *up;
  UNDO *start;
  UNDO *end;
  UNDOSTACK *st;
  UNDOGROUP *g;
  int status = TRUE;

  /* Get the last undo group on the list, or error out
   * if the list is empty.
   */
  undoing = TRUE;
  st = curwp->w_bufp->b_undo;
  g = (UNDOGROUP *) st->links.prev;
  if (&g->links == &st->links)
    {
      eprintf ("undo stack is empty");
      undoing = FALSE;
      return FALSE;
    }

  /* Replay all steps of the most recently saved undo.  Break up
   * the steps into subsequences that start with moves.  Play these
   * subsequences in reverse order, but play the individual steps
   * within a subsequence in forward order.
   */
  end = &g->undos[g->next];
  start = end - 1;
  while (start >= g->undos)
    {
      while (start > g->undos && start->l == NOLINE)
	--start;
      for (up = start; up != end; up++)
	{
	  int s = undostep (up);

	  if (s != TRUE)
	    status = s;
	}
      end = start;
      --start;
    }

  /* Pop this undo group from the list and free it up.
   */
  freegroup (g);
  st->ngroups--;
  undoing = FALSE;
  return status;
}

/*
 * Print a single undo record.  The \r characters are necessary
 * because this function is called from gdb, and
 * at this point the editor has tweaked the tty so that
 * newline doesn't generate a carriage return.
 */
static void
printone (UNDO *up)
{
  printf ("  ");
  switch (ukind (up))
    {
    case UCH:
      if (up->u.ch.c == '\n')
	printf ("Char: NEWLINE");
      else
	printf ("Char: '%c'", up->u.ch.c);
      printf (", n = %d", up->u.ch.n);
      break;

    case USTR:
      {
	const uchar *s;
	int n;

	printf ("String: '");
	for (s = up->u.str.s, n = up->u.str.n; n > 0; --n, ++s)
	  {
	    uchar c = *s;
	    if (c == '\n')
	      printf ("\\n");
	    else
	      printf ("%c", c);
	  }
	printf ("'");
        break;
      }

    case UMOVE:
      printf ("Move");
      break;

    case UDEL:
      printf ("Delete: %d characters",
              up->u.del.n);
      break;

    default:
      printf ("Unexpected kind 0x%x", up->kind);
      break;
    }
  if (up->l != NOLINE)
    printf (", line %d, offset %d",
            up->l, up->o);
  printf ("\r\n");
}

/*
 * Print the current window's undo stack.  This is intended
 * to be called from gdb for debugging purposes only.
 */
void
printundo (void)
{
  int level;
  UNDOSTACK *st;
  UNDOGROUP *g;
  UNDO *up;
  UNDO *end;

  level = 1;
  st = curwp->w_bufp->b_undo;
  for (g = (UNDOGROUP *) st->links.next;
       &g->links != &st->links;
       g = (UNDOGROUP *) g->links.next)
    {
      printf ("%d:\r\n", level);
      end = &g->undos[g->next];
      for (up = &g->undos[0]; up != end; up++)
	printone (up);
      ++level;
    }
}


/*
 * Free up the undo records associated with a buffer.
 */
void
killundo (BUFFER *bp)
{
  UNDOGROUP *g;
  UNDOSTACK *st = bp->b_undo;

  for (g = (UNDOGROUP *) st->links.next;
       &g->links != &st->links;
       g = (UNDOGROUP *) st->links.next)
    freegroup (g);
  free (st);
  bp->b_undo = NULL;
}
