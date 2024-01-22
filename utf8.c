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

/*
 * To compile this file as a test program:
 *
 *   gcc -DTEST -Isys/unix -Itty/ncurses -g -o utf8 utf8.c
 *
 * Replace -g with -O to make an optimized version in which
 * uclen (defined in def.h) is inlined.
 */

#define _XOPEN_SOURCE
#include <wchar.h>

#include "def.h"
#ifdef TEST
#include <time.h>
#endif

#include <string.h>
#include <stdio.h>

/*
 * Return true if Unicode character c is a combining character,
 * i.e. is a non-spacing character that combines
 * with a subsequent one.
 */
int
ucombining (wchar_t c)
{
  return ((c >= 0x300  && c <= 0x36f) ||
	  (c >= 0x1ab0 && c <= 0x1aff) ||
	  (c >= 0x1dc0 && c <= 0x1dff) ||
	  (c >= 0x20d0 && c <= 0x20ff) ||
	  (c >= 0xfe20 && c <= 0xfe2f));
}

/*
 * Return the address of the nth UTF-8 character in the string s.
 */
const uchar *
ugetcptr (const uchar *s, int n)
{
  while (n > 0)
    {
      s += uclen (s);
      --n;
    }
  return s;
}

/*
 * Return the byte offset of the nth UTF-8 character in the string s.
 */
int
uoffset (const uchar *s, int n)
{
  const uchar *start = s;
  while (n > 0)
    {
       s += uclen (s);
      --n;
    }
  return s - start;
}

/*
 * Return number of UTF-8 characters in the null-terminated string s.
 */
int
uslen (const uchar *s)
{
  int len = 0;

  while (*s != 0)
    {
      s += uclen (s);
      len++;
    }
  return len;
}

/*
 * Return number of UTF-8 characters in the string s of length n.
 */
int
unslen (const uchar *s, int n)
{
  int len = 0;
  const uchar *end = s + n;

  while (s < end)
    {
      s += uclen (s);
      len++;
    }
  return len;
}

/*
 * Return the number of bytes used by the next n UFT-8 characters
 * in the string s.
 */
int
unblen (const uchar *s, int n)
{
  const uchar *start = s;

  while (n > 0)
    {
      s += uclen (s);
      --n;
    }
  return s - start;
}

/*
 * Get the nth UTF-8 character in s, return it
 * as a 32-bit unicode character.  If len is not NULL,
 * return the length of the UTF-8 character to *len.
 */
wchar_t
ugetc (const uchar *s, int n, int *len)
{
  uchar c;

  s = ugetcptr (s, n);
  c = *s;
  if (c < 0x80)
    {
      if (len)
	*len = 1;
      return c;
    }
  if (c >= 0xc0 && c <= 0xdf)
    {
      if (len)
	*len = 2;
      return ((int)(c & 0x1f) << 6) + (int)(s[1] & 0x3f);
    }
  if (c >= 0xe0 && c <= 0xef)
    {
      if (len)
	*len = 3;
      return ((int)(c & 0xf) << 12) +
             ((int)(s[1] & 0x3f) << 6) +
	     (int)(s[2] & 0x3f);
    }
  if (c >= 0xf0 && c <= 0xf7)
    {
      if (len)
	*len = 4;
      return ((int)(c & 0x7) << 18) +
             ((int)(s[1] & 0x3f) << 12) +
             ((int)(s[2] & 0x3f) << 6) +
             (int)(s[3] & 0x3f);
    }
  if (c >= 0xf8 && c <= 0xfb)
    {
      if (len)
	*len = 5;
      return ((int)(c & 0x3) << 24) +
             ((int)(s[1] & 0x3f) << 18) +
             ((int)(s[2] & 0x3f) << 12) +
             ((int)(s[3] & 0x3f) << 6) +
             (int)(s[4] & 0x3f);
    }
  if (c >= 0xfc && c <= 0xfd)
    {
      if (len)
	*len = 6;
      return ((int)(c & 0x1) << 30) +
             ((int)(s[1] & 0x3f) << 24) +
             ((int)(s[2] & 0x3f) << 18) +
             ((int)(s[3] & 0x3f) << 12) +
             ((int)(s[4] & 0x3f) << 6) +
             (int)(s[5] & 0x3f);
    }
  /* Error */
  if (len)
    *len = 1;
  return c;
}
/*
 * Get the previous UTF-8 character, i.e. the character just
 * before the one pointed to by s.  Return it
 * as a 32-bit unicode character.  If len is not NULL,
 * return the length of the UTF-8 character to *len.
 */
wchar_t
ugetprevc (const uchar *s, int *len)
{
  do {
    --s;
  } while ((*s & 0xc0) == 0x80);
  return ugetc (s, 0, len);
}
  
/*
 * Convert a Unicode character c to UTF-8, writing the
 * characters to s; s must be at least 6 bytes long.
 * Return the number of bytes in the UTF-8 string.
 */
int
uputc (wchar_t c, uchar *s)
{
  if (c < 0x80)
    {
      s[0] = c;
      return 1;
    }
  if (c >= 0x80 && c <= 0x7ff)
    {
      s[0] = 0xc0 | ((c >> 6) & 0x1f);
      s[1] = 0x80 | (c & 0x3f);
      return 2;
    }
  if (c >= 0x800 && c <= 0xffff)
    {
      s[0] = 0xe0 | ((c >> 12) & 0x0f);
      s[1] = 0x80 | ((c >>  6) & 0x3f);
      s[2] = 0x80 | (c & 0x3f);
      return 3;
    }
  if (c >= 0x10000 && c <= 0x1fffff)
    {
      s[0] = 0xf0 | ((c >> 18) & 0x07);
      s[1] = 0x80 | ((c >> 12) & 0x3f);
      s[2] = 0x80 | ((c >>  6) & 0x3f);
      s[3] = 0x80 | (c & 0x3f);
      return 4;
    }
  if (c >= 0x200000 && c <= 0x3ffffff)
    {
      s[0] = 0xf8 | ((c >> 24) & 0x03);
      s[1] = 0x80 | ((c >> 18) & 0x3f);
      s[2] = 0x80 | ((c >> 12) & 0x3f);
      s[3] = 0x80 | ((c >>  6) & 0x3f);
      s[4] = 0x80 | (c & 0x3f);
      return 5;
    }
  if (c >= 0x4000000 && c <= 0x7fffffff)
    {
      s[0] = 0xfc | ((c >> 30) & 0x01);
      s[1] = 0x80 | ((c >> 24) & 0x3f);
      s[2] = 0x80 | ((c >> 18) & 0x3f);
      s[3] = 0x80 | ((c >> 12) & 0x3f);
      s[4] = 0x80 | ((c >>  6) & 0x3f);
      s[5] = 0x80 | (c & 0x3f);
      return 6;
    }
  /* Error */
  s[0] = c;
  return 1;
}

/*
 * Return the display width of a Unicode character.
 * This is just a wrapper for wcwidth.
 */
int
uwidth (wchar_t c)
{
  return wcwidth(c);
}

/*
 * Prompt for a string of hex numbers separated by spaces.
 * Treat each hex number as a Unicode character, convert
 * it to UTF-8, and insert into the current buffer.
 */
#ifndef TEST
int
unicode (int f, int n, int k)
{
  int s, len, i;
  char buf[80];
  char *p;
  unsigned int c[40];
  int count;

  s = ereply ("Enter Unicode characters in hex: ", buf, sizeof (buf));
  if (s != TRUE)
    return s;
  for (p = buf, count = 0; p < &buf[sizeof (buf)] && *p != '\0'; p += len, ++count)
    {
      s = sscanf (p, " %x%n", &c[count], &len);
      if (s != 1)
	{
	  eprintf("Illegal hex number: %s", p);
	  return FALSE;
	}
    }
  for (i = 0; i < count; i++)
    {
      if (linsert (1, c[i], NULL) == FALSE)
	return FALSE;
    }
  return TRUE;
}
#endif

#ifdef TEST

int
main(int argc, char *argv[])
{
  static uchar s[] = { 'a', '=', 0xc3, 0xa4, ',', 'i', '=', 0xe2, 0x88, 0xab, ',',
                      '+', '=', 0xf0, 0x90, 0x80, 0x8f, ',',
		      'j', '=', 0xf0, 0x9f, 0x82, 0xab, '.',
		      'a', '=', 0xc3, 0xa4, ',', 'i', '=', 0xe2, 0x88, 0xab, ',',
                      '+', '=', 0xf0, 0x90, 0x80, 0x8f, ',',
		      'j', '=', 0xf0, 0x9f, 0x82, 0xab, '.', 0 };

  wchar_t c;
  int i, len;
  double time_spent;
  clock_t begin, end;
  const uchar *p;

  printf("size of wchar_t is %ld\n", sizeof(c));
  printf("s is '%s'\n", s);
  printf("length of s is %ld\n", sizeof(s));
  len = uslen(s);
  printf("number of UTF-8 chars in s is %d\n", len);
  printf("Scanning forward...\n");
  for (i = 0; i < len; i++)
    {
      int u, size;

      printf("offset of UTF-8 char #%d in s is %ld\n", i, ugetcptr(s, i) - s);
      u = ugetc(s, i, &size);
      printf("char #%d in s in unicode is %x, size %d\n", i, u, size);
    }
  printf("Scanning backwards...\n");
  p = s + strlen (s);
  i = len;
  while (p > s)
    {
      int u, size;

      u = ugetprevc (p, &size);
      p -= size;
      --i;
      printf("offset of UTF-8 char #%d in s is %ld\n", i, p - s);
      printf("char #%d in s in unicode is %x, size %d\n", i, u, size);
    }
  begin = clock ();
  for (i = 0; i < 10000000; i++)
    if ((p = ugetcptr (s, len)) == s)
      printf ("Should never get here!\n");
  end = clock ();
  time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
  printf ("Time spent in %d loops of ugetcptr is %f\n", i, time_spent);
  return 0;
}

#endif
