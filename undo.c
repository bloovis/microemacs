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

/* A single undo step, as part of a larger group.
 * Each step is like a journal entry for the editor.
 * It consists of a:
 *  - UDELETE entry when the editor deletes a string
 *  - UINSERT entry when the editor inserts a string
 *  - UMOVE entry when the editor moves the dot
 */
typedef struct UNDO
{
  UKIND kind;			/* Kind of information		*/
  int l;			/* Line number			*/
  int o;			/* Offset into line		*/
  union
  {
    /* UDELETE record */
    struct
    {
      int chars;		/* # of characters deleted	*/
      int bytes;		/* # of bytes deleted		*/
      uchar *s;			/* string that was deleted	*/
    } del;

    /* UINSERT record */
    struct
    {
      int copies;		/* # of copies of string	*/
      int chars;		/* # of characters inserted	*/
      int bytes;		/* # of bytes inserted		*/
      uchar *s;			/* string that was inserted	*/
    } ins;
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
  LINKS undolist;	/* head and tail of undo group list */
  LINKS redolist;	/* head and tail of redo group list */
  int ngroups;		/* size of group stack */
}
UNDOSTACK;

#define ukind(up)  (up->kind & 0xff)
#define ustart(up) ((up->kind & USTART) != 0)
#define uend(up)   ((up->kind & UEND) != 0)

#define NOLINE  -1		/* UNDO.{l,o} value meaning "not used"	*/

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
unlinkgroup (UNDOGROUP *g)
{
  g->links.prev->next = g->links.next;
  g->links.next->prev = g->links.prev;
}

/*
 * Append a group to the end of a list.
 */
static void
appendgroup (UNDOGROUP *g, LINKS *list)
{
  g->links.prev = list->prev;
  g->links.next = list;
  list->prev->next = &g->links;
  list->prev = &g->links;
}

/*
 * Is a group list empty?
 */
static int
emptylist (LINKS *list)
{
  return list->next == list;
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
 * Free up any resources associated with an undo step.
 */
static void
freeundo (UNDO *up)
{
  switch (ukind (up))
    {
    case UDELETE:
      free (up->u.del.s);
      break;
    case UINSERT:
      free (up->u.ins.s);
      break;
    }
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

  clp = firstline (curbp);
  nline = 0;
  for (;;)
    {
      if (clp == lastline (curbp) || clp == lp)
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
  initlinks (&st->undolist);
  initlinks (&st->redolist);
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
  unlinkgroup (g);

  /* Free up any resources used by the undo steps in the group.
   */
  end = &g->undos[g->next];
  for (up = &g->undos[0]; up != end; up++)
    freeundo (up);

  /* Free up the undo array.
   */
  free (g->undos);

  /* Finally, free up the group record.
   */
  free (g);
}

/*
 * Free up all undo groups on a list.
 */
static void
freegrouplist (UNDOSTACK *st, LINKS *list)
{
  UNDOGROUP *g;

  for (g = (UNDOGROUP *) list->next;
       &g->links != list;
       g = (UNDOGROUP *) list->next)
    {
      freegroup (g);
      st->ngroups--;
    }
}

/*
 * Return a pointer to the most recently saved undo record,
 * or NULL if there is none.
 */
static UNDO *
lastundo (UNDOSTACK *st)
{
  UNDOGROUP *g;

  /* Is group list empty?
   */
  if (emptylist (&st->undolist))
    return NULL;

  /* Is group itself empty?
   */
  g = (UNDOGROUP *) st->undolist.prev;
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
      appendgroup (g, &st->undolist);

      /* If we've reached the maximum number of undo groups, recycle the
       * first one in the list.
       */
      if (st->ngroups >= N_UNDO)
	freegroup ((UNDOGROUP *) st->undolist.next);
      else
	st->ngroups++;
    }
  else
    /* This is not the first undo record in a group.  Get
     * the last group in the list
     */
    g = (UNDOGROUP *) st->undolist.prev;

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
 * Make one or more copies of byte sequence s whose length in bytes is n,
 * and store the result in dest, which must be at least (copies * n) bytes.
 */
static uchar *
memdup (uchar *dest, int copies, const uchar *s, int n)
{
  int i;

  for (i = 0; i < copies; i++)
    memcpy (dest + (i * n), s, n);
  return dest;
}

/*
 * Save a single undo record, which is a record of a move, deletion, or insertion.
 * The first two parameters are fixed:
 *  - the kind of undo record
 *  - a pointer to a line/offset pair to be recorded in
 *    the record, or NULL if no line/offset pair is to be recorded.
 * Following these two parameters there may be other parameters,
 * depending on the kind:
 *  - UMOVE:
 *    - takes no extra parameters
 *  - UDELETE:
 *    - size of deleted string in characters
 *    - size of deleted string in bytes
 *    - pointer to deleted string (may not be null-terminated)
 *  - UINSERT:
 *    - # of copies of inserted string
 *    - size of inserted string in characters
 *    - size of inserted string in bytes
 *    - pointer to inserted string (may not be null-terminated)
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

  /* Free up any redo records that have accumulated.
   */
  st = curwp->w_bufp->b_undo;
  freegrouplist (st, &st->redolist);

  switch (kind)
    {
    case UMOVE:			/* Move to (line #, offset)	*/
      up = newundo (st, kind, line, offset);
      break;

    case UDELETE:		/* Delete string		*/
      {
	int chars = va_arg (ap, int);
	int bytes = va_arg (ap, int);
	const uchar *s = va_arg (ap, const uchar *);

	up = newundo (st, kind, line, offset);
	up->u.del.s = (uchar *) malloc (bytes);
	if (up->u.del.s == NULL)
	  {
	  eprintf ("Out of memory in undo!");
	  return FALSE;
	  }
        memcpy (up->u.del.s, s, bytes);
	up->u.del.chars = chars;
	up->u.del.bytes = bytes;
	break;
      }

    case UINSERT:		/* Insert N copies of a string	*/
      {
	int copies = va_arg (ap, int);
	int chars = va_arg (ap, int);
	int bytes = va_arg (ap, int);
	const uchar *s = va_arg (ap, const uchar *);
	UNDO *prev = lastundo (st);
	int totalbytes = bytes * copies;

	if (prev != NULL &&
            ukind (prev) == UINSERT &&
	    prev->l == line &&
	    prev->o + prev->u.ins.chars == offset)
	  {
	    prev->u.ins.chars += chars;
	    prev->u.ins.s = (uchar *) realloc (prev->u.ins.s, prev->u.ins.bytes + totalbytes);
	    if (prev->u.ins.s == NULL)
	      {
		eprintf ("Out of memory in undo!");
		return FALSE;
	      }
	    memdup (prev->u.ins.s + prev->u.ins.bytes, copies, s, bytes);
	    prev->u.ins.bytes += totalbytes;
	  }
	else
	  {
	    up = newundo (st, kind, line, offset);
	    up->u.ins.chars = chars * copies;
	    up->u.ins.bytes = totalbytes;

	    /* Make copy of string. */
	    up->u.ins.s = (uchar *) malloc (totalbytes);
	    if (up->u.ins.s == NULL)
	      {
		eprintf ("Out of memory in undo!");
		return FALSE;
	      }
	    memdup (up->u.ins.s, copies, s, bytes);
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
      if (up->o > wllength (curwp->w_dot.p))
	eprintf ("Offset too large");
      else
	{
	  curwp->w_dot.o = up->o;
	  curwp->w_flag |= WFMOVE;
	}
    }

  if (status == TRUE)
    {
      switch (ukind (up))
	{
	case UMOVE:
	  break;

	case UDELETE:
	  {
	    const uchar *s = up->u.del.s;
	    status = insertwithnl ((const char *) s, up->u.del.bytes);
	    break;
	  }

	case UINSERT:
	  status = ldelete (up->u.ins.chars, FALSE);
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
  if (emptylist (&st->undolist))
    {
      eprintf ("undo stack is empty");
      undoing = FALSE;
      return FALSE;
    }
  g = (UNDOGROUP *) st->undolist.prev;

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

  /* Set the buffer change flag.
   */
  if (g->b_flag & BFCHG)
    curbp->b_flag |= BFCHG;
  else
    curbp->b_flag &= ~BFCHG;
  updatemode ();

  /* Pop this undo group from the undo list and move it
   * to the redo list.
   */
  unlinkgroup (g);
  appendgroup (g, &st->redolist);

  undoing = FALSE;
  return status;
}

/*
 * Redo a single step in a possibly larger sequence of undo records.
 */
static int
redostep (UNDO *up)
{
  int status = TRUE;

  if (up->l != NOLINE)
    {
      status = gotoline (TRUE, up->l + 1, KRANDOM);
      if (up->o > wllength (curwp->w_dot.p))
	eprintf ("Offset too large");
      else
	{
	  curwp->w_dot.o = up->o;
	  curwp->w_flag |= WFMOVE;
	}
      curwp->w_flag |= WFMOVE;
    }

  if (status == TRUE)
    {
      switch (ukind (up))
	{
	case UMOVE:
	  break;

	case UDELETE:
	  {
	    status = ldelete (up->u.del.chars, FALSE);
	    break;
	  }

	case UINSERT:
	  status = insertwithnl ((const char *) up->u.ins.s, up->u.ins.bytes);
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
 * Redo the topmost undo group on the redo stack.
 */
int
redo (int f, int n, int k)
{
  UNDO *up;
  UNDO *start;
  UNDO *end;
  UNDOSTACK *st;
  UNDOGROUP *g;
  int status = TRUE;

  /* Get the top undo group on the redo stack, or error out
   * if the stack is empty.
   */
  undoing = TRUE;
  st = curwp->w_bufp->b_undo;
  if (emptylist (&st->redolist))
    {
      eprintf ("redo stack is empty");
      undoing = FALSE;
      return FALSE;
    }
  g = (UNDOGROUP *) st->redolist.prev;

  /* Undo all steps of this redo group.
   */
  end = &g->undos[g->next];
  start = &g->undos[0];
  for (up = start; up != end; up++)
    {
      int s = redostep (up);

      if (s != TRUE)
	status = s;
    }

  /* Set the buffer change flag.
   */
  curbp->b_flag |= BFCHG;
  updatemode ();

  /* Move this undo group from the redo stack back to the top of the
   * the undo stack.
   */
  unlinkgroup (g);
  appendgroup (g, &st->undolist);
  undoing = FALSE;
  return status;
}

#if DEBUG

/*
 * Print a non-null-terminated string.
 */
static void
printstring (const uchar *s, int bytes)
{
  printf ("'");
  while (bytes > 0)
    {
      uchar c = *s;
      if (c == '\n')
	printf ("\\n");
      else
	printf ("%c", c);
      --bytes;
      ++s;
    }
  printf ("'");
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
    case UDELETE:
      {
	printf ("Delete string: ");
	printstring (up->u.del.s, up->u.del.bytes);
        break;
      }

    case UMOVE:
      printf ("Move");
      break;

    case UINSERT:
      printf ("Insert string: ");
      printstring (up->u.ins.s, up->u.ins.bytes);
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
  for (g = (UNDOGROUP *) st->undolist.next;
       &g->links != &st->undolist;
       g = (UNDOGROUP *) g->links.next)
    {
      printf ("%d:\r\n", level);
      end = &g->undos[g->next];
      for (up = &g->undos[0]; up != end; up++)
	printone (up);
      ++level;
    }
}

#endif	/* DEBUG */

/*
 * Free up the undo records associated with a buffer.
 */
void
killundo (BUFFER *bp)
{
  UNDOSTACK *st = bp->b_undo;

  freegrouplist (st, &st->undolist);
  freegrouplist (st, &st->redolist);
  free (st);
  bp->b_undo = NULL;
}


/*
 * Set the "buffer changed" flag in all undo records for
 * the buffer buffer.  This should be called after a buffer
 * is saved to disk, so that any subsequent undo operations will
 * set the "buffer changed" flag for that buffer.
 */
void
setundochanged (void)
{
  UNDOSTACK *st;
  UNDOGROUP *g;

  st = curbp->b_undo;
  for (g = (UNDOGROUP *) st->undolist.next;
       &g->links != &st->undolist;
       g = (UNDOGROUP *) g->links.next)
    {
      g->b_flag = BFCHG;
    }
}
