/* $Header: /home/bloovis/cvsroot/pe/main.c,v 1.3 2005-10-18 02:18:05 bloovis Exp $
 *
 * Name:	MicroEMACS
 *		Mainline, macro commands.
 * Version:	29
 * By:		rex::conroy
 *		decvax!decwrl!dec-rhea!dec-rex!conroy
 *
 * $Log: main.c,v $
 * Revision 1.3  2005-10-18 02:18:05  bloovis
 * Rename some things to avoid conflict with ncurses.
 *
 * Revision 1.2  2005/05/31 18:18:22  bloovis
 * (bufinit): w_savep was uninitialized; clear it.
 *
 * Revision 1.1.1.1  2003/11/06 02:51:52  bloovis
 * Imported sources
 *
 * Revision 1.7  2002/01/23 22:36:02  malexander
 * (mouse): New variable for mouse support, set by -m option.
 *
 * Revision 1.6  2001/03/05 16:04:06  malexander
 * (bufinit): Clear mark ring of new window.
 *
 * Revision 1.5  2001/02/28 21:07:40  malexander
 * * def.h (POS): New structure for holding line position, which replaces
 * dot and mark variable pairs everywhere.
 *
 * Revision 1.4  2000/11/01 22:00:43  malexander
 * (main): Don't make buffer read-only; bcreate does that now.
 *
 * Revision 1.3  2000/09/29 00:19:38  malexander
 * Numerous changes to eliminate warnings and add prototypes.
 *
 * Revision 1.2  2000/07/21 16:20:32  malexander
 * Reformatted with GNU indent.
 *
 * Revision 1.1.1.1  2000/07/14 19:23:10  malexander
 * Imported sources
 *
 * Revision 1.4  1996/10/22 15:58:57  marka
 * Allow "C-U -" to be treated as a shortcut for -1.
 *
 * Revision 1.3  91/01/07  10:26:04  alexande
 * Remove C++ warnings.  Add tab size variable.
 * 
 * Revision 1.2  90/07/03  13:21:05  alexande
 * Changed pat[] to uchar to be consistent with new declaration in def.h.
 * 
 *
 */
#include	"def.h"

int thisflag;			/* Flags, this command          */
int lastflag;			/* Flags, last command          */
int curgoal;			/* Goal column                  */
BUFFER *curbp = 0;		/* Current buffer               */
EWINDOW *curwp = 0;		/* Current window               */
BUFFER *bheadp;			/* BUFFER listhead              */
EWINDOW *wheadp;			/* EWINDOW listhead              */
BUFFER *blistp;			/* Buffer list BUFFER           */
short kbdm[NKBDM] = { KCTLX | ')' };	/* Macro                        */
short *kbdmip;			/* Input  for above             */
short *kbdmop;			/* Output for above             */
uchar pat[NPAT] = { 0 };	/* Pattern                      */
SYMBOL *symbol[NSHASH];		/* Symbol table listhead.       */
SYMBOL *binding[NKEYS];		/* Key bindings.                */
int inprof;			/* True if reading profile      */
int bflag;			/* True if -b option specified  */
int mouse;			/* True if -m option specified  */
int rflag;			/* True if -r option specified  */
int xflag;			/* True if -x option specified  */
int zflag;			/* True if -z option specified  */
int casefold = TRUE;		/* True if searches fold case   */
int fillcol = 70;		/* Fill column for paragraphs.  */
int tabsize = 8;		/* No. of columns for a tab     */

static int nbuf;		/* number of buffers    */

/*
 * Forward declarations.
 */
static void bufinit (const char *fname);
static int execute (int c, int f, int n);

/*
 * Main program.
 */
int 
main (int argc, char *argv[])
{
  register int c;
  register int f;
  register int n;
  register int mflag;
  register char *arg;
  char *proptr;
  int line = 0;

  proptr = NULLPTR;		/* profile name         */
  for (n = 1; n < argc; n++)
    {				/* Search for options   */
      arg = argv[n];
      if (arg[0] == '-')
	{
	  switch (arg[1])
	    {
#if	BACKUP
	    case 'b':
	      bflag = TRUE;
	      break;
#endif
	    case 'g':
	      n++;
	      if (n < argc)
		line = atoi (argv[n]);
	      break;
	    case 'm':
	      mouse = TRUE;
	      break;
	    case 'p':
	      n++;
	      if (n < argc)
		{
		  proptr = argv[n];
		  adjustcase (proptr);
		}
	      break;
	    case 'r':
	      rflag = TRUE;
	      break;
	    case 'x':
	      xflag = TRUE;
	      break;
	    case 'z':
	      zflag = TRUE;
	      break;
	    }
	}
      else if (arg[0] == '+')
	{
	  line = atoi (&arg[1]);
	}
    }
  vtinit ();			/* Virtual terminal.    */
  if ((blistp = bcreate ("")) == NULL)	/* Special list buffer. */
    abort ();
  keymapinit ();		/* Symbols, bindings.   */
  upmapinit ();			/* Upper case map table */

  for (n = 1; n < argc; n++)
    {				/* Read in files        */
      arg = argv[n];
      if (arg[0] == '-')
	{			/* ignore options       */
	  if (arg[1] == 'p' || arg[1] == 'g')
	    n++;		/* skip name after -p,-g */
	}
      else if (arg[0] != '+')
	{			/* it's a filename      */
	  /* If the filename is followed by :N, where N is a decimal number,
	   * go to the specified line number in the file.
	   */
	  char * colon = strchr (arg, ':');
	  if (colon != NULL && colon[1] >= '0' && colon[1] <= '9')
	    {
	      *colon = '\0';
	      line = atoi (colon + 1);
	    }
	  bufinit (arg);	/* make buffer & window */
	  update ();
	  readin (arg);		/* read in the file     */
	  if (line != 0)	/* goto line specified  */
	    {
	      gotoline (TRUE, line, 0);
	      line = 0;
	    }
	}
    }

  if (nbuf == 0)
    {				/* no files read in?    */
      bufinit ("main");		/* make an empty buffer */
      update ();
    }
  else
    {
      curwp = wheadp;		/* reset current window */
      curbp = curwp->w_bufp;	/* reset current buffer */
    }
  lastflag = 0;			/* Fake last flags.     */

  inprof = enoecho = (ffpopen (proptr) == FIOSUC);
  /* open default profile */
  if (!inprof && proptr != NULLPTR)	/* -p option failed?    */
    eprintf ("Unable to open profile %s", proptr);

loop:
  if (!inprof && !ttstat ())	/* If not in a profile, */
    update ();			/*  fix up the screen.  */
  c = getkey ();
  if (epresf != FALSE)
    {				/* Stuff on echo line?  */
      eerase ();		/* Get rid of echo line */
      if (!inprof && !ttstat ())
	update ();
    }
  f = FALSE;
  n = 1;
  if (c == (KCTRL | 'U'))
    {				/* ^U, start argument.  */
      f = TRUE;
      n = 4;
      while ((c = getkey ()) == (KCTRL | 'U'))
	n *= 4;
      if ((c >= '0' && c <= '9') || c == '-')
	{
	  if (c == '-')
	    {
	      n = 0;
	      mflag = TRUE;
	    }
	  else
	    {
	      n = c - '0';
	      mflag = FALSE;
	    }
	  while ((c = getkey ()) >= '0' && c <= '9')
	    n = 10 * n + c - '0';
	  if (mflag != FALSE)
	    n = n == 0 ? -1 : -n;
	}
      if (n == -n && n != 0)	/* if n == 0x8000       */
	n = 1;			/* change it to default */
    }
  if (kbdmip != NULL)
    {				/* Save macro strokes.  */
      if (c != (KCTLX | ')') && kbdmip > &kbdm[NKBDM - 6])
	{
	  ctrlg (FALSE, 0, KRANDOM);
	  goto loop;
	}
      if (f != FALSE)
	{
	  *kbdmip++ = (KCTRL | 'U');
	  *kbdmip++ = n;
	}
      *kbdmip++ = c;
    }
  execute (c, f, n);		/* Do it.               */
  goto loop;
}

/*
 * Command execution. Look up the binding in the the
 * binding array, and do what it says. Return a very bad status
 * if there is no binding, or if the symbol has a type that
 * is not usable (there is no way to get this into a symbol table
 * entry now). Also fiddle with the flags.
 */
static int
execute (int c, int f, int n)
{
  register SYMBOL *sp;
  register int status;

  if ((sp = binding[c]) != NULL)
    {
      thisflag = 0;
      if (sp->s_macro)
	status = domacro (sp->s_macro, n);
      else
	status = (*sp->s_funcp) (f, n, c);
      lastflag = thisflag;
      return (status);
    }
  eprintf ("Unknown command");
  lastflag = 0;
  return (ABORT);
}

/*
 * Initialize a new buffer and window.  The filename is passed down as
 * an argument, because the main routine may have been told to read in
 * a file by default, and we want the buffer name to be right.  If there
 * is a conflict in buffer names, keep modifying the name until the
 * conflict goes away.  This function also creates up to two initial
 * windows for the first one or two files being edited.
 */
static void
bufinit (const char *fname)
{
  char bname[NBUFN];		/* Buffer name          */
  char *mod;			/* Ptr to name modifier */
  register BUFFER *bp;
  register EWINDOW *wp;

  makename (bname, fname);	/* Get buffer name      */
  if (bfind (bname, FALSE))	/* if names conflict    */
    {
      strcat (bname, ".0");	/* Append modifier      */
      mod = &bname[strlen (bname) - 1];	/* Point to '0' at end  */
      while (bfind (bname, FALSE))	/* While still conflicts */
	(*mod)++;		/* Bump the modifier    */
    }
  if ((bp = bfind (bname, TRUE)) == NULL)	/* Create text buffer.  */
    abort ();
  curbp = bp;			/* Current buffer       */
  if (++nbuf <= 2)		/* If 2 or less buffers */
    {				/* Get a new window     */
      if ((wp = (EWINDOW *) malloc (sizeof (EWINDOW))) == NULL)
	abort ();		/* Out of memory        */
      curwp = wp;		/* Current window       */
      if (nbuf == 1)		/* First window?        */
	{
	  wheadp = wp;
	  wp->w_toprow = 0;
	}
      else			/* Second window.   */
	{
	  wheadp->w_wndp = wp;
	  wp->w_toprow = nrow / 2;	/* 2nd half of screen   */
	  wheadp->w_ntrows = wp->w_toprow - 1;	/* shrink  */
	}			/* 1 = mode line        */
      wp->w_ntrows = nrow - wp->w_toprow - 2;	/* 2 = mode,echo */
      wp->w_wndp = NULL;	/* Initialize window.   */
      wp->w_bufp = bp;
      bp->b_nwnd = 1;		/* Displayed.           */
      wp->w_linep = bp->b_linep;
      wp->w_dot.p = bp->b_linep;
      wp->w_dot.o = 0;
      wp->w_mark.p = NULL;
      wp->w_mark.o = 0;
      clearring (&wp->w_ring);
      wp->w_force = 0;
      wp->w_flag = WFMODE | WFHARD;	/* Full.                */
      wp->w_leftcol = 0;	/* Display at left edge */
      wp->w_savep = NULL;
    }
}

/*
 * Fancy quit command, as implemented
 * by Jeff. If the current buffer has changed
 * do a write current buffer. Otherwise run a command
 * interpreter in a subjob. Two of these will get you
 * out. Bound to "C-Z".
 */
int
jeffexit (f, n, k)
{
  if ((curbp->b_flag & BFCHG) != 0)	/* Changed.             */
    return (filesave (f, n, KRANDOM));
  return (spawncli (f, n, KRANDOM));	/* Suspend.             */
}

/*
 * Quit command. If an argument, always
 * quit. Otherwise confirm if a buffer has been
 * changed and not written out. Normally bound
 * to "C-X C-C".
 */
int
quit (f, n, k)
{
  register int s;

  if (f != FALSE		/* Argument forces it.  */
      || anycb () == FALSE	/* All buffers clean.   */
      || (s = eyesno ("Quit")) == TRUE)
    {				/* User says it's OK.   */
      vttidy ();
      exit (GOOD);
    }
  return (s);
}

/*
 * Begin a keyboard macro.
 * Error if not at the top level
 * in keyboard processing. Set up
 * variables and return.
 */
int
ctlxlp (f, n, k)
{
  if (kbdmip != NULL || kbdmop != NULL)
    {
      eprintf ("Not now");
      return (FALSE);
    }
  eprintf ("[Start macro]");
  kbdmip = &kbdm[0];
  return (TRUE);
}

/*
 * End keyboard macro. Check for
 * the same limit conditions as the
 * above routine. Set up the variables
 * and return to the caller.
 */
int
ctlxrp (f, n, k)
{
  if (kbdmip == NULL)
    {
      eprintf ("Not now");
      return (FALSE);
    }
  eprintf ("[End macro]");
  *(kbdmip - 1) = KCTLX | ')';
  kbdmip = NULL;
  return (TRUE);
}

/*
 * Execute the current (un-named) macro.
 * The command argument is the
 * number of times to loop. Quit as
 * soon as a command gets an error.
 * Return TRUE if all ok, else
 * FALSE.
 */
int
ctlxe (f, n, k)
{
  return domacro (&kbdm[0], n);
}

/*
 * Execute the specified macro (either the current
 * keyboard macro, or a named macro).
 * The command argument is the
 * number of times to loop. Quit as
 * soon as a command gets an error.
 * Return TRUE if all ok, else
 * FALSE.
 */
int
domacro (short *macrop, int n)
{
  register int c;
  register int af;
  register int an;
  register int s;

  if (kbdmip != NULL || kbdmop != NULL)
    {
      eprintf ("Not now");
      return (FALSE);
    }
  if (n <= 0)
    return (TRUE);
  do
    {
      kbdmop = macrop;
      do
	{
	  af = FALSE;
	  an = 1;
	  if ((c = *kbdmop++) == (KCTRL | 'U'))
	    {
	      af = TRUE;
	      an = *kbdmop++;
	      c = *kbdmop++;
	    }
	  s = TRUE;
	}
      while (c != (KCTLX | ')') && (s = execute (c, af, an)) == TRUE);
      kbdmop = NULL;
    }
  while (s == TRUE && --n);
  return (s);
}

/*
 * Abort.
 * Beep the beeper.
 * Kill off any keyboard macro,
 * etc., that is in progress.
 * Sometimes called as a routine,
 * to do general aborting of
 * stuff.
 */
int
ctrlg (f, n, k)
{
  ttbeep ();
  if (kbdmip != NULL)
    {
      kbdm[0] = (KCTLX | ')');
      kbdmip = NULL;
    }
  return (ABORT);
}

/*
 * Display the version. All this does
 * is copy the text in the external "version" array into
 * the message system, and call the message reading code.
 * Don't call display if there is an argument.
 */
int
showversion (f, n, k)
{
  register char **cpp;
  register char *cp;

  cpp = &version[0];
  while ((cp = *cpp++) != NULL)
    {
      if (writemsg (cp) == FALSE)
	return (FALSE);
    }
  if (f != FALSE)		/* No display if arg.   */
    return (TRUE);
  return (readmsg ());
}
