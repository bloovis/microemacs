/*
 * Name:	MicroEMACS
 *		Mark ring utilities
 * By:		Mark Alexander
 *		marka@pobox.com
 *
 * The functions in this file manipulate mark rings.
 * The ring is a stack in which overflow is handled
 * by recycling the oldest entries.
 *
 * There are functions for clearing a ring, pushing
 * a mark on a ring, and popping a mark off the ring.
 *
 */
#include	"def.h"

static POS nullpos = { NULL, 0 };

/*
 * Pop a mark off of the stack.  If there are no more marks to pop,
 * a mark with a null line pointer is returned and the stack pointer
 * is not moved.
 */
POS popring (MARKRING *ring)
{
  POS top;
  int i, count;

  /* Check for empty stack. */
  count = ring->m_count;
  if (count == 0)
    return nullpos;

  /* Rotate the stack. */
  top = ring->m_ring[0];
  for (i = 0; i < count - 1; i++)
    ring->m_ring[i] = ring->m_ring[i + 1];
  ring->m_ring[count - 1] = top;

  /* Return old top stack item. */
  return top;
}

/*
 * Return the top mark of the stack without popping it.
 */
POS topring (MARKRING *ring)
{
  /* Check for empty stack. */
  if (ring->m_count == 0)
    return nullpos;

  /* Return top stack item. */
  return ring->m_ring[0];
}

/*
 * Push a mark onto the stack.  If there the stack is full, reuse the
 * oldest mark.
 */
void pushring (MARKRING *ring, POS pos)
{
  int i, count;

  if (pos.p == NULL)
    return;

  /* Check for full stack. */
  if (ring->m_count < RINGSIZE)
    ring->m_count++;
  count = ring->m_count;

  /* Make room on the stack for a new item. */
  for (i = count - 1; i > 0; i--)
    ring->m_ring[i] = ring->m_ring[i - 1];

  /* Push the mark. */
  ring->m_ring[0] = pos;
}

/*
 * Clear a mark ring.
 */
void clearring (MARKRING *ring)
{
  int i;

  for (i = 0; i < RINGSIZE; i++)
    {
      ring->m_ring[i].p = NULL;
      ring->m_ring[i].o = 0;
    }
  ring->m_count = 0;
}
