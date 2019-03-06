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

#include	"def.h"

#include	<process.h>

/* extern char *getenv(char *); */

char *cspec = NULL;		/* Command string.      */

/*
 * Create a subjob with a copy
 * of the command intrepreter in it. When the
 * command interpreter exits, mark the screen as
 * garbage so that you do a full repaint. Bound
 * to "C-C" and called from "C-Z".
 */
int
spawncli (int f, int n, int k)
{
  ttcolor (CTEXT);		/* Normal color.        */
  ttwindow (0, nrow - 1);	/* Full screen scroll.  */
  ttmove (nrow - 1, 0);		/* Last line.           */
  ttflush ();
  ttclose ();
  if (cspec == NULL)
    {				/* Try to find it.      */
      cspec = getenv ("COMSPEC");
      if (cspec == NULL)
	cspec = "/c/WINDOWS/system32/cmd.exe";
    }
  spawnlp (0, cspec, cspec, "", "", NULLPTR);
  ttopen ();
  sgarbf = TRUE;
  return (TRUE);
}

/*
 * Open a two-way pipe to the specified program, store the
 * input FILE pointer to *infile, store the output FILE pointer
 * to *outfile, and return TRUE if success.
 */
int
openpipe (const char *program, const char *args[],
	  FILE ** infile, FILE ** outfile)
{
  return FALSE;			/* not implemented yet on Windows */
}
