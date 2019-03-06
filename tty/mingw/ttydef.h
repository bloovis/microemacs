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

#define GOSLING 0			/* Use fancy redisplay. 	*/
#define MEMMAP	1			/* Not memory mapped video.	*/

#define	NROW	66			/* Genius can do 60 rows	*/
#define NCOL	132			/* Some VGAs can do 90 cols	*/

/*
 * Special keys, as on the IBM PC.
 * The codes are all just redefinitions for the standard extra
 * key codes. Using the standard names ensures that the
 * codes land in the right place.
 */
#define	KUP	K01
#define	KDOWN	K02
#define	KLEFT	K03
#define	KRIGHT	K04
#define	KPGUP	K05
#define	KPGDN	K06
#define	KHOME	K07
#define	KEND	K08
#define	KINS	K09
#define	KDEL	K0A
#define	KF1	K0B
#define	KF2	K0C
#define	KF3	K0D
#define	KF4	K0E
#define	KF5	K0F
#define	KF6	K10
#define	KF7	K11
#define	KF8	K12
#define	KF9	K13
#define	KF10	K14
#define	KSF1	K15
#define	KSF2	K16
#define	KSF3	K17
#define	KSF4	K18
#define	KSF5	K19
#define	KSF6	K1A
#define	KSF7	K1B
#define	KSF8	K1C
#define	KSF9	K1D
#define	KSF10	K1E

/* Functions. */
void putline(int row, int col, const wchar_t *buf);
