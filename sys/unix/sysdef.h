/*
    Copyright (C) 2008 Mark Alexander

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
 * Name:	MicroEMACS
 *		Ultrix-32 system header file.
 * Version:	29
 * Last edit:	15-Jul-86
 * By:		Mark Alexander
 *		drivax!alexande
 */
#define	PCC	1		/* "[]" gets an error.          */
#define	KBLOCK	8192		/* Kill grow.                   */
#define	GOOD	0		/* Good exit status.            */
#define NULLPTR ((char *) 0)	/* Portable NULL pointer for    */
					/*  use in function calls       */
/*
 * Macros used by the buffer name making code.
 * Start at the end of the file name, scan to the left
 * until BDC1 (or BDC2, if defined) is reached. The buffer
 * name starts just to the right of that location, and
 * stops at end of string (or at the next BDC3 character,
 * if defined). BDC2 and BDC3 are mainly for VMS.
 */
#define	BDC1	'/'		/* Buffer names.                */
