/*----------------------------------------------------------------------
 *	bcopy - byte string operations for 4.2 BSD
 */

/*----------------------------------------------------------------------
 * BCOPYR - move BYTEs from source to destination, start at right end
 */
void
bcopyr (char *source, char *dest, int count)
{
  source += count;
  dest += count;
  while (count-- != 0)
    *--dest = *--source;
}

/*----------------------------------------------------------------------
 * BFILL - fill a buffer with a single BYTE value
 */
void
bfill (char c, char *buffer, int count)
{
  while (count-- != 0)
    *buffer++ = c;
#if 0
  memset (buffer, c, count);
#endif
}
