/* $Header: /home/bloovis/cvsroot/pe/nt/ttydef.h,v 1.1 2003-11-06 02:51:52 bloovis Exp $
 *
 * Name:	MicroEMACS
 *		IBM PC tty header file
 * Last edit:	29-Mar-88
 * By:		Mark Alexander
 *		alexande@borland.com
 *
 * $Log: ttydef.h,v $
 * Revision 1.1  2003-11-06 02:51:52  bloovis
 * Initial revision
 *
 * Revision 1.1  2001/04/19 20:26:08  malexander
 * New files for NT version of MicroEMACS.
 *
 *
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
