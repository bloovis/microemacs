/*
 *	Termcap terminal file, nothing special, just make it big
 *	enough for windowing systems.
 */

#define	GOSLING	1		/* Compile in fancy display.    */
/* #define	MEMMAP		*//* Not memory mapped video.     */
#define XKEYS			/* Use termcap to handle function keys  */
#define IGNORE_TERMCAP		/* Ignore termcap defs. of func. keys   */

#define	NROW	132		/* Rows.                        */
#define	NCOL	132		/* Columns.                     */
/* #define	MOVE_STANDOUT	*//* don't move in standout mode  */
#define	STANDOUT_GLITCH		/* possible standout glitch     */
#define	TERMCAP			/* for possible use in ttyio.c  */

/*
 * Termcap function keys.  The last 10 keys correspond to the
 * non-standard termcap entries K0-K9 (instead of k0-k9).
 */
#ifdef	XKEYS
/* #define	KFIRST	K01 */
/* #define	KLAST	K1A */

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
#define	KF0	K0B
#define	KF1	K0C
#define	KF2	K0D
#define	KF3	K0E
#define	KF4	K0F
#define	KF5	K10
#define	KF6	K11
#define	KF7	K12
#define	KF8	K13
#define	KF9	K14
#define	KSF0	K15
#define	KSF1	K16
#define	KSF2	K17
#define	KSF3	K18
#define	KSF4	K19
#define	KSF5	K1A
#define	KSF6	K1B
#define	KSF7	K1C
#define	KSF8	K1D
#define	KSF9	K1E
#define KMOUSE	K1F

#define	NFKEYS	20		/* # of function keys (k0-k9, K0-K9) */
#endif
