/* $Header: /home/bloovis/cvsroot/pe/nt/tty.c,v 1.1 2003-11-06 02:51:52 bloovis Exp $
 *
 * Name:        MicroEMACS
 *              OS/2 80x25 to 80x50 text-mode display
 * By:      Mark Alexander
 *              alexande@borland.com
 *
 * $Log: tty.c,v $
 * Revision 1.1  2003-11-06 02:51:52  bloovis
 * Initial revision
 *
 * Revision 1.1  2001/04/19 20:26:08  malexander
 * New files for NT version of MicroEMACS.
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

#include <io.h>

#include "def.h"

#define BEL     0x07                    /* BEL character.               */

static  int     ttattr;                 /* IBM PC screen attributes     */

static  int     attinv = 0x70;          /* attributes for inverse video */
static  int     attnorm = 7;            /* attributes for normal video  */

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
void ttinit()
{
    CHAR_INFO buf;
    COORD size, coord;
    SMALL_RECT region;

    hout = GetStdHandle(STD_OUTPUT_HANDLE);
    hin =  GetStdHandle(STD_INPUT_HANDLE);

    size.X = 1;
    size.Y = 1;
    coord.X = 0;
    coord.Y = 0;
    region.Left = 0;
    region.Top = nrow-1;
    region.Right = 0;
    region.Bottom = nrow-1;
    if (ReadConsoleOutput(hout, &buf, size, coord, &region) == TRUE)
    {
        attnorm = buf.Attributes;   /* current attributes   */
            attinv  = (attnorm & 0x88)          /* blink, invert bits   */
                | ((attnorm >> 4) & 0x07)   /* foreground color     */
                | ((attnorm << 4) & 0x70);  /* background color     */
            ttcolor(CTEXT);
    }
}

/*
 * The PC needs no tidy up.
 */
void tttidy()
{
}

/*
 * Move the cursor to the specified
 * origin 0 row and column position. Try to
 * optimize out extra moves; redisplay may
 * have left the cursor in the right
 * location last time!
 */
void ttmove(row, col)
{
    COORD coord;

    if (ttrow!=row || ttcol!=col) {
	if (row > nrow)
            row = nrow;
	if (col > ncol)
	    col = ncol;
        coord.X = col + windowcol;
        coord.Y = row + windowrow;
        SetConsoleCursorPosition(hout, coord);
        ttrow = row;
        ttcol = col;
    }
}

/*
 * Erase to end of line.
 */
void tteeol()
{
    COORD coord;
    char space = ' ';
    DWORD nwritten;

    coord.X = ttcol + windowcol;
    coord.Y = ttrow + windowrow;

    /* Very inefficient.  Rewrite to blast entire string of spaces at once.
     */
    SetConsoleTextAttribute(hout, ttattr);
    while (coord.X < ncol + windowcol)
    {
        WriteConsoleOutputCharacter(hout, &space, 1, coord, &nwritten);
        coord.X++;
    }
}

/*
 * Erase to end of page.
 */
void tteeop()
{
    COORD coord;
    char space = ' ';
    DWORD nwritten;

    coord.X = ttcol + windowcol;
    coord.Y = ttrow + windowrow;

    /* Very inefficient.  Rewrite to blast entire string of spaces at once.
     */
    SetConsoleTextAttribute(hout, ttattr);
    while (coord.Y < ttrow + windowrow)
    {   
        while (coord.X < ttcol + windowcol)
        {
            WriteConsoleOutputCharacter(hout, &space, 1,
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

void ttbeep()
{
    write(1,"\007",1);
}

/*
 * No-op.
 */
void ttwindow(top, bot)
{
}

/*
 * No-op.
 */
void ttnowindow()
{
}

/*
 * Set display color on IBM PC.  Just convert MicroEMACS
 * color to IBM display adapter attributes.
 */
void ttcolor(color)
int     color;
{
    if ((tthue = color) == CMODE)               /* modeline color?      */
        ttattr = attinv;                        /* inverse video        */
    else
        ttattr = attnorm;                       /* normal video         */
}

/*
 * Resize the screen.  Pass the flag (0 or 1) to qvinit.  The flag
 * says whether to switch to 43-line or 50-line mode on EGA or VGA.
 */
void ttresize()
{
#if 0
        qvmode(f);
        nrow = nrow;
        ncol = ncol;
        if (nrow >= 43)                         /* high res screen?     */
                qvcursor(2);                    /* block cursor         */
#endif
}

/*
 * Write character.
 */
int
ttputc(int c)
{
    DWORD nwritten;
    char c1 = (char)c;

    SetConsoleTextAttribute(hout, ttattr);
    WriteFile(hout, &c1, 1, &nwritten, NULL);
    return c;
}

/*
 * High speed screen update.  row and col are 1-based.
 */
void putline(row,col,buf)
int row,col;
char *buf;
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
        cinfo[i].Char.AsciiChar = *buf++;
        cinfo[i].Attributes = ttattr;
    }

    coord.X = 0;
    coord.Y = 0;
    region.Left = windowcol + col;
    region.Right = windowcol + ncol - 1;
    region.Top = region.Bottom = windowrow + row;
    WriteConsoleOutput(hout, cinfo, size, coord, &region);
}
