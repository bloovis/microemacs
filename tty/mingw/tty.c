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

#include <io.h>

#include "def.h"

#define BEL     0x07                    /* BEL character.               */

static  int     ttattr;                 /* creen attributes     	*/

static  int     attinv = 0x70;          /* attributes for inverse video */
	int     attnorm = 7;            /* attributes for normal video  */

extern  int     tttop;
extern  int     ttbot;
extern  int     tthue;

/* Variables from ttyio.c */
extern	int	windowrow;
extern	int	windowcol;
extern  int     ttrow;
extern  int     ttcol;

#if     GOSLING
int     tceeol  =       2;              /* Costs.                       */
int     tcinsl  =       11;
int     tcdell  =       11;
#endif

/*
 * Forward declarations.
 */
#if 0
void    ttinit(), tttidy(), ttmove(), tteeol(), tteeop(), ttbeep(),
    waittick(), ttwindow(), ttnowindow(), ttcolor(), ttresize(),
    ttputc(), putline();
#endif

static HANDLE hout, hin;

/*
 * Initialize the terminal.  Get the handles for console input and output.
 * Take a peek at the video buffer to see what video attributes are being used.
 */
void
ttinit (void)
{
  CHAR_INFO buf;
  COORD size, coord;
  SMALL_RECT region;

  hout = GetStdHandle (STD_OUTPUT_HANDLE);
  hin =  GetStdHandle (STD_INPUT_HANDLE);

  size.X = 1;
  size.Y = 1;
  coord.X = 0;
  coord.Y = 0;
  region.Left = 0;
  region.Top = nrow-1;
  region.Right = 0;
  region.Bottom = nrow-1;
  if (ReadConsoleOutput (hout, &buf, size, coord, &region) == TRUE)
    {
      attnorm = buf.Attributes;		  /* current attributes   */
      attinv  = (attnorm & 0x88)          /* blink, invert bits   */
	      | ((attnorm >> 4) & 0x07)   /* foreground color     */
	      | ((attnorm << 4) & 0x70);  /* background color     */
      ttcolor (CTEXT);
    }
}

/*
 * The Win32 console needs no tidy up.
 */
void
tttidy (void)
{
}

/*
 * Move the cursor to the specified
 * origin 0 row and column position. Try to
 * optimize out extra moves; redisplay may
 * have left the cursor in the right
 * location last time!
 */
void
ttmove (int row, int col)
{
  COORD coord;

  if (ttrow!=row || ttcol!=col)
    {
      if (row > nrow)
	row = nrow;
      if (col > ncol)
	col = ncol;
      coord.X = col + windowcol;
      coord.Y = row + windowrow;
      SetConsoleCursorPosition (hout, coord);
      ttrow = row;
      ttcol = col;
    }
}

/*
 * Erase to end of line.
 */
void
tteeol (void)
{
  COORD coord;
  char space = ' ';
  DWORD nwritten;

  coord.X = ttcol + windowcol;
  coord.Y = ttrow + windowrow;

  /* Very inefficient.  Rewrite to blast entire string of spaces at once.
   */
  SetConsoleTextAttribute (hout, ttattr);
  while (coord.X < ncol + windowcol)
    {
      WriteConsoleOutputCharacter (hout, &space, 1, coord, &nwritten);
      coord.X++;
    }
}

/*
 * Erase to end of page.
 */
void
tteeop (void)
{
  COORD coord;
  char space = ' ';
  DWORD nwritten;

  coord.X = ttcol + windowcol;
  coord.Y = ttrow + windowrow;

  /* Very inefficient.  Rewrite to blast entire string of spaces at once.
   */
  SetConsoleTextAttribute (hout, ttattr);
  while (coord.Y < ttrow + windowrow)
    {   
      while (coord.X < ttcol + windowcol)
        {
	  WriteConsoleOutputCharacter (hout, &space, 1,
				       coord, &nwritten);
	  coord.X++;
        }
      coord.X = windowcol;
      coord.Y++;
    }
}

/*
 * Make a noise.
 */

void
ttbeep (void)
{
  write (1,"\007",1);
}

/*
 * No-op.
 */
void
ttwindow (int top, int bot)
{
}

/*
 * No-op.
 */
void
ttnowindow (void)
{
}

/*
 * Set display color on the Win32 console.
 */
void
ttcolor (int color)
{
  if ((tthue = color) == CMODE)             /* modeline color?      */
    ttattr = attinv;                        /* inverse video        */
  else
    ttattr = attnorm;                       /* normal video         */
}

/*
 * Resize the screen.  Pass the flag (0 or 1) to qvinit.  The flag
 * says whether to switch to 43-line or 50-line mode on EGA or VGA.
 */
void
ttresize (void)
{
#if 0
  qvmode (f);
  nrow = nrow;
  ncol = ncol;
  if (nrow >= 43)                    /* high res screen?     */
    qvcursor (2);                    /* block cursor         */
#endif
}

/*
 * Write character.
 */
int
ttputc (int c)
{
  DWORD nwritten;
  char c1 = (char)c;

  SetConsoleTextAttribute (hout, ttattr);
  WriteFile (hout, &c1, 1, &nwritten, NULL);
  return c;
}

/*
 * High speed screen update.  row and col are 1-based.
 */
void
putline (int row, int col, const wchar_t *buf)
{
  COORD size, coord;
  SMALL_RECT region;
  static CHAR_INFO cinfo[NCOL];
  int i;

  /* Adjust row and col to zero-based values.
   */
  row--;
  col--;

  /* The size of the data to copy is the remaining number of characters
   * on the line.
   */
  size.X = ncol - col;
  size.Y = 1;

  /* Copy the text into a char/attribute buffer. */
  for (i = 0; i < size.X; i++)
    {
      cinfo[i].Char.UnicodeChar = *buf++;
      cinfo[i].Attributes = ttattr;
    }

  coord.X = 0;
  coord.Y = 0;
  region.Left = windowcol + col;
  region.Right = windowcol + ncol - 1;
  region.Top = region.Bottom = windowrow + row;
  WriteConsoleOutputW (hout, cinfo, size, coord, &region);
}
