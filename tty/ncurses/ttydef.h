/*
 *	Ncurses terminal file.
 */

/* #define	GOSLING	1 */	/* Compile in fancy display.    */
#define	MEMMAP 1		/* Ncurses emulates memory mapped video.*/
#define XKEYS			/* Use termcap to handle function keys  */
#define IGNORE_TERMCAP		/* Ignore termcap defs. of func. keys   */

#define	NROW	256		/* Rows.                        */
#define	NCOL	256		/* Columns.                     */

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
#define	KF11	K15
#define	KF12	K16

#define	NFKEYS	12		/* # of function keys (k0-k9, K0-K9) */
#endif

/* Functions. */
void putline(int row, int col, const char *buf);
