/* $Header: /home/bloovis/cvsroot/pe/nt/sysdef.h,v 1.1 2003-11-06 02:51:52 bloovis Exp $
 *
 * Name:	MicroEMACS
 *		OS/2 system header file.
 * By:		Mark Alexander
 *		alexande@borland.com
 *
 * $Log: sysdef.h,v $
 * Revision 1.1  2003-11-06 02:51:52  bloovis
 * Initial revision
 *
 * Revision 1.1  2001/04/19 20:26:08  malexander
 * New files for NT version of MicroEMACS.
 *
 * 
 */
#define LARGE	0			/* Large model. 		*/
#define PCC	1			/* "[]" will work.              */
#define GOOD	0			/* not in Lattice stdio.h	*/
#define NULLPTR	((char *) 0)		/* Portable null pointer	*/
/*
 * Macros used by the buffer name making code.
 * Start at the end of the file name, scan to the left
 * until BDC0, (or BDC1 or BDC2, if defined) is reached. The buffer
 * name starts just to the right of that location, and
 * stops at end of string (or at the next BDC3 character,
 * if defined). BDC2 and BDC3 are mainly for VMS.
 */
#define BDC0	':'                     /* Buffer names.                */
#define BDC1	'\\'
#define BDC2	'/'

#define OS2_SAFE			/* Don't hang on OS/2		*/
