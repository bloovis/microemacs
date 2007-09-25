/*
 * Name:	MicroEMACS
 *		MS-DOS spawn command.com
 * Version:	29
 * Last edit:	29-Mar-88
 * By:		Mark Alexander
 *		drivax!alexande
 */
#include	"def.h"

#include	<process.h>

/* extern char *getenv(char *); */

char	*cspec	= NULL; 			/* Command string.	*/

/*
 * Create a subjob with a copy
 * of the command intrepreter in it. When the
 * command interpreter exits, mark the screen as
 * garbage so that you do a full repaint. Bound
 * to "C-C" and called from "C-Z".
 */
spawncli(f, n, k)
{
	ttcolor(CTEXT); 			/* Normal color.	*/
	ttwindow(0, nrow-1);			/* Full screen scroll.	*/
	ttmove(nrow-1, 0);			/* Last line.		*/
	ttflush();
	ttclose();
	if (cspec == NULL) {			/* Try to find it.	*/
		cspec = getenv("COMSPEC");
		if (cspec == NULL)
			cspec = "\\os2\\cmd.exe";
	}
	spawnlp(0,cspec,cspec,"","",NULLPTR);
	ttopen();
	sgarbf = TRUE;
	return(TRUE);
}

/*
 * Run the spell checker.
 * FIXME: does nothing on Windows.
 */
int
spellcheck (f, n, k)
{
	return (FALSE);
}


