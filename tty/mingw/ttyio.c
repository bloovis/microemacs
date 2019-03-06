/*
    Copyright (C) 2019 Mark Alexander

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

#include <excpt.h>
#if 0
#include <windef.h>
#include <winbase.h>
#include <wincon.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <conio.h>

#include "def.h"

extern int attnorm;	/* In tty.c */

/* Variables that MicroEMACS uses for screen size.  Initialized by
 * ttopen().
 */
int nrow = 25;          /* Terminal size, rows.     		*/
int ncol = 80;          /* Terminal size, columns.  		*/
int npages = 1;		/* Number of pages on terminal.		*/
int windowrow;		/* Row in buffer of top window line.	*/
int windowcol;		/* Column in buffer of top window line.	*/

static HANDLE hout, hin;
static DWORD  hinmode;
static DWORD  houtmode;

/*
 * Initialization.
 * Set up the video system, and set the keyboard to binary mode.
 */
void
ttopen (void)
{
  CONSOLE_SCREEN_BUFFER_INFO binfo;
  CONSOLE_CURSOR_INFO cinfo;

  /* Get handles for console output and input
   */
  hout = GetStdHandle (STD_OUTPUT_HANDLE);
  hin =  GetStdHandle (STD_INPUT_HANDLE);

  /* Save current keyboard mode, then disable line editing.
   */
  GetConsoleMode (hin, &hinmode);
  SetConsoleMode (hin, 0);         /* disable line mode */

  /* Save current console output mode.
   */
  GetConsoleMode (hout, &houtmode);

  /* Get screen size.
   */
  if (GetConsoleScreenBufferInfo (hout, &binfo) == TRUE)
    {
      windowrow = binfo.srWindow.Top;
      windowcol = binfo.srWindow.Left;
      nrow = binfo.srWindow.Bottom - windowrow + 1;
      ncol = binfo.srWindow.Right  - windowcol + 1;
    }

  /* Set block cursor
   */
  cinfo.dwSize = 100;     /* make it 100% visible */
  cinfo.bVisible = TRUE;
  SetConsoleCursorInfo (hout, &cinfo);
}

/*
 * Restore video system, control-break flag.
 */
void
ttclose (void)
{
  SetConsoleMode (hin, hinmode);
}

/*
 * No operation in MS-DOS.
 */
void
ttflush (void)
{
}

/*
 * Read character.
 */
int
ttgetc (void)
{
  int ch;
#if 0
  DWORD nread;

  if (ReadFile (hin, &ch, 1, &nread, NULL) != TRUE)
    exit (3);
  else
    return (ch);
#else
  if ((ch = _getch ()) == 0 || ch == 0xe0)	/* extended key */
    {
      if ((ch = _getch ()) == 3)	/* null? */
	return 0;			/* convert to 0 */
      else
	return (ch + 0x100);	/* flag this as extended key */
    }
  else
    return (ch);			/* normal key */
#endif
}

/*
 * Check for the presence of a keyboard character.
 * Return TRUE if a key is already in the queue.
 */
int
ttstat (void)
{
#if 0
  KBDKEYINFO  kinfo;

  return (KbdPeek (&kinfo,0) == 0);
#else
  return 0;
#endif
}

/*
 * Insert character in the display.  Characters to the right
 * of the insertion point are moved one space to the right.
 */
int
ttinsertc (int c)
{
  CONSOLE_SCREEN_BUFFER_INFO binfo;
  COORD pos;
  SMALL_RECT src;
  COORD dst;
  static CHAR_INFO fill;

  if (!GetConsoleScreenBufferInfo (hout, &binfo))
    return c;
  pos = binfo.dwCursorPosition;
  src.Left = pos.X;
  src.Top = pos.Y;
  src.Right = ncol - 2;
  src.Bottom = pos.Y;
  dst.X = pos.X + 1;
  dst.Y = pos.Y;
  fill.Char.AsciiChar = ' ';
  fill.Attributes = attnorm;
  if (!ScrollConsoleScreenBuffer (
      hout,
      &src,
      NULL,
      dst,
      &fill))
    ttputc ('!');
  else
    ttputc (c); 
  return c;
}

/*
 * Delete character in the display.  Characters to the right
 * of the deletion point are moved one space to the left.
 */
void
ttdelc (void)
{
  CONSOLE_SCREEN_BUFFER_INFO binfo;
  COORD pos;
  SMALL_RECT src;
  COORD dst;
  static CHAR_INFO fill;

  if (!GetConsoleScreenBufferInfo (hout, &binfo))
    return;
  pos = binfo.dwCursorPosition;
  src.Left = pos.X + 1;
  src.Top = pos.Y;
  src.Right = ncol - 1;
  src.Bottom = pos.Y;
  dst.X = pos.X;
  dst.Y = pos.Y;
  fill.Char.AsciiChar = ' ';
  fill.Attributes = attnorm;
  if (!ScrollConsoleScreenBuffer (
      hout,
      &src,
      NULL,
      dst,
      &fill))
    ttputc ('!');
}
