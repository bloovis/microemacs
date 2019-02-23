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
 * Fetch the most recent mark from the ring, move it to the end of
 * the ring, and return it to the caller.  If the ring is empty,
 * return a mark with a null line pointer.
 */
POS
popmark (void)
{
  POS top;
  int i, count;
  MARKRING *ring = &curwp->w_ring;

  /* Check for empty ring. */
  count = ring->m_count;
  if (count == 0)
    return nullpos;

  /* Rotate the ring. */
  top = ring->m_ring[0];
  for (i = 0; i < count - 1; i++)
    ring->m_ring[i] = ring->m_ring[i + 1];
  ring->m_ring[count - 1] = top;

  /* Return old top stack item. */
  return top;
}

/*
 * Push a mark onto the current window's mark ring.
 * If the ring is full, reuse the oldest mark.
 */
void
pushmark (POS pos)
{
  int i, count;
  MARKRING *ring = &curwp->w_ring;

  if (pos.p == NULL)
    return;

  /* Check for full ring. */
  if (ring->m_count < RINGSIZE)
    ring->m_count++;
  count = ring->m_count;

  /* Make room on the ring for a new item. */
  for (i = count - 1; i > 0; i--)
    ring->m_ring[i] = ring->m_ring[i - 1];

  /* Push the mark. */
  ring->m_ring[0] = pos;
}

/*
 * Clear a mark ring.
 */
void
clearmarks (MARKRING *ring)
{
  int i;

  for (i = 0; i < RINGSIZE; i++)
    {
      ring->m_ring[i].p = NULL;
      ring->m_ring[i].o = 0;
    }
  ring->m_count = 0;
}
