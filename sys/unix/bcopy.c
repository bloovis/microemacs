/*
 * Name:	MicroEMACS
 *		Ultrix-32 byte copy routines
 * Version:	1
 * Last edit:	01-Jul-86
 * By:		Mark Alexander
 *		drivax!alexande
 *
 * The functions in this file copy and fill bytes.  They should be
 * translated into assembler for speed, as they were on the 68K
 * and the 8088.  For now, the C versions will do until I figure
 * out VAX assembler.
 */


/*----------------------------------------------------------------------*/

bcopy (source, dest, count)
     register char *source, *dest;
     register int count;
{
  /* This function copies count bytes from source to dest
   * in ascending order.
   */

  while (count-- != 0)
    *dest++ = *source++;
}


/*----------------------------------------------------------------------*/

bcopyr (source, dest, count)
     register char *source, *dest;
     register int count;
{
  /* This function copies count bytes from source to dest
   * in descending order, starting at the right end of the
   * buffers.
   */

  source += count;
  dest += count;
  while (count-- != 0)
    *--dest = *--source;
}


/*----------------------------------------------------------------------*/

bfill (c, buffer, count)
     register char c, *buffer;
     register int count;
{
  /* This function fills count bytes at buffer with the character c.
   */

  while (count-- != 0)
    *buffer++ = c;
}
