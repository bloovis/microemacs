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
