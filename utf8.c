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

/* #define TEST */

#ifndef TEST
#include "def.h"
#else
typedef unsigned char uchar;
#endif

#include <wchar.h>
#include <string.h>
#include <stdio.h>

/*
 * Return the length in bytes of the utf-8 character whose first byte is c.
 */
int
uclen (const uchar *s)
{
  int n;
  uchar c = *s;

  if (c < 0x80)
    n = 1;
  if (c >= 0xc0 && c <= 0xdf)
    n = 2;
  else if (c >= 0xe0 && c <= 0xef)
    n = 3;
  else if (c >= 0xf0 && c <= 0xf7)
    n = 4;
  else if (c >= 0xf8 && c <= 0xfb)
    n = 5;
  else if (c >= 0xfc && c <= 0xfd)
    n = 6;
  else
    n = 1;	/* error */
  return n;
}

/*
 * Return the byte offset of the nth utf-8 character in the string s.
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
 * Return number of utf-8 characters in the null-terminated string s.
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
 * Return number of utf-8 characters in the string s of length n.
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
 * Get the nth utf-8 character in s, return it
 * as a 32-bit unicode character.  Return the
 * length of the utf-8 character to *len.
 */
wchar_t
ugetc (const uchar *s, int n, int *len)
{
  uchar c;

  s += uoffset (s, n);
  c = *s;
  if (c < 0x80)
    {
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
  
#ifdef TEST

int
main(int argc, char *argv[])
{
  static unsigned char s[] = { 'a', '=', 0xc3, 0xa4, ',', 'i', '=', 0xe2, 0x88, 0xab, ',',
                      '+', '=', 0xf0, 0x90, 0x80, 0x8f, ',',
		      'j', '=', 0xf0, 0x9f, 0x82, 0xab, '.', 0 };

  wchar_t c;
  int i, len;

  printf("size of wchar_t is %ld\n", sizeof(c));
  printf("s is '%s'\n", s);
  printf("length of s is %ld\n", sizeof(s));
  len = uslen(s);
  printf("number of utf-8 chars in s is %d\n", len);
  for (i = 0; i < len; i++)
    {
      int u, size;

      printf("offset of utf-8 char #%d in s is %d\n", i, uoffset(s, i));
      u = ugetc(s, i, &size);
      printf("char #%d in s in unicode is %x, size %d\n", i, u, size);
    }
  return 0;
}

#endif
