/* $Header: /home/bloovis/cvsroot/pe/nt/ttyio.c,v 1.1 2003-11-06 02:51:52 bloovis Exp $
 *
 * Name:    MicroEMACS
 *      WIN32 text mode I/O.
 * Modified by: Mark Alexander
 *      marka@pobox.com
 *
 * $Log: ttyio.c,v $
 * Revision 1.1  2003-11-06 02:51:52  bloovis
 * Initial revision
 *
 * Revision 1.1  2001/04/19 20:26:08  malexander
 * New files for NT version of MicroEMACS.
 *
 *
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
void ttopen()
{
    CONSOLE_SCREEN_BUFFER_INFO binfo;
    CONSOLE_CURSOR_INFO cinfo;

    /* Get handles for console output and input
     */
    hout = GetStdHandle(STD_OUTPUT_HANDLE);
    hin =  GetStdHandle(STD_INPUT_HANDLE);

    /* Save current keyboard mode, then disable line editing.
     */
    GetConsoleMode(hin, &hinmode);
    SetConsoleMode(hin, 0);         /* disable line mode */

    /* Save current console output mode.
     */
    GetConsoleMode(hout, &houtmode);

    /* Get screen size.
     */
    if (GetConsoleScreenBufferInfo(hout, &binfo) == TRUE)
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
    SetConsoleCursorInfo(hout, &cinfo);
}

/*
 * Restore video system, control-break flag.
 */
void ttclose()
{
    SetConsoleMode(hin, hinmode);
}

/*
 * No operation in MS-DOS.
 */
void ttflush()
{
}

/*
 * Read character.
 */
int
ttgetc(void)
{
    int ch;
#if 0
    DWORD nread;

    if (ReadFile(hin, &ch, 1, &nread, NULL) != TRUE)
        exit(3);
    else
        return (ch);
#else
    if ((ch = getch()) == 0 || ch == 0xe0)	/* extended key */
    {
	if ((ch = getch()) == 3)	/* null? */
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
ttstat(void)
{
#if 0
    KBDKEYINFO  kinfo;

    return (KbdPeek(&kinfo,0) == 0);
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

  if (!GetConsoleScreenBufferInfo(hout, &binfo))
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
  if (!ScrollConsoleScreenBuffer(
      hout,
      &src,
      NULL,
      dst,
      &fill))
    ttputc('!');
  else
    ttputc(c); 
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

  if (!GetConsoleScreenBufferInfo(hout, &binfo))
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
  if (!ScrollConsoleScreenBuffer(
      hout,
      &src,
      NULL,
      dst,
      &fill))
    ttputc('!');
}

