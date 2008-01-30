/* $Header: /home/bloovis/cvsroot/pe/nt/ttyio.c,v 1.1 2003-11-06 02:51:52 bloovis Exp $
 *
 * Name:    MicroEMACS
 *      OS/2 text mode I/O.
 * Modified by: Mark Alexander
 *      alexande@borland.com
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

#ifdef __BORLANDC__
#include <excpt.h>
#include <conio.h>
#else
#include	<termios.h>
#include	<sys/ioctl.h>
#endif

#if 0
#include <windef.h>
#include <winbase.h>
#include <wincon.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "def.h"

/* Required by spawn.c.
 */
struct termios oldtty;
struct termios newtty;

/* Variables that MicroEMACS uses for screen size.  Initialized by
 * ttopen().
 */
int nrow = 25;          /* Terminal size, rows.     		*/
int ncol = 80;          /* Terminal size, columns.  		*/
int windowrow;		/* Row in buffer of top window line.	*/
int windowcol;		/* Column in buffer of top window line.	*/

static HANDLE hout, hin;
static DWORD  conmode;

/*
 * Initialization.
 * Set up the video system, and set the keyboard to binary mode.
 * Apparently, OS/2 doesn't allow control-BREAK to be disabled
 * when the keyboard is in binary mode, using signal.
 */
void ttopen()
{
    CONSOLE_SCREEN_BUFFER_INFO binfo;
    CONSOLE_CURSOR_INFO cinfo;

    /* Get Cygwin "before" tty structure.
     */
    tcgetattr (0, &oldtty);

    /* Get handles for console output and input
     */
    hout = GetStdHandle(STD_OUTPUT_HANDLE);
    hin =  GetStdHandle(STD_INPUT_HANDLE);

    /* Save current keyboard mode, then disable line editing.
     */
    GetConsoleMode(hin, &conmode);
    SetConsoleMode(hin, 0);         /* disable line mode */

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

    /* Get Cygwin "after" tty structure.
     */
    tcgetattr (0, &newtty);
}


/*
 * Restore video system, control-break flag.
 */
void ttclose()
{
    SetConsoleMode(hin, conmode);
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
int ttgetc()
{
#ifndef __BORLANDC__
  DWORD nread;
  INPUT_RECORD ir;
  KEY_EVENT_RECORD *ker;
  unsigned int ch;

  for (;;)
    {
      if (!ReadConsoleInput(hin, &ir, 1, &nread))
	return 0;
      if (ir.EventType != KEY_EVENT)
	continue;
      ker = &ir.Event.KeyEvent;
      if (!ker->bKeyDown)
	continue;
      ch = ker->uChar.AsciiChar;
      if (ch != 0)
	{
	  if (ch == 0x20 && (ker->dwControlKeyState & (LEFT_CTRL_PRESSED |
                             RIGHT_CTRL_PRESSED)) != 0)
	    {
	      ch = 0;
	    }
            return ch;
	}
      else
	{
	  ch = ker->wVirtualScanCode;
	  int shifted = ker->dwControlKeyState & SHIFT_PRESSED;
	  if (ch >= 0x3b && ch <= 0x44)
	    {
	      /* Function keys.  Test for shifted state. */
	      if (shifted)
		return (ch - 0x3b + 0x54) | 0x100;
	      else
		return ch | 0x100;
	    }
          else if ((ch >= 0x47 && ch <= 0x53) ||
                   (ch >= 0x57 && ch <= 0x58))
	    {
	      /* Direction keys, or F11-F12. */
	      return ch | 0x100;
	    }
        }
    }


#else
    int ch;
    if ((ch = getch()) == 0)		/* extended key */
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
int ttstat()
{
#if 0
    KBDKEYINFO  kinfo;

    return (KbdPeek(&kinfo,0) == 0);
#else
    return 0;
#endif
}
