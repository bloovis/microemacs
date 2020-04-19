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

/* $Header: /exit14/home/marka/tools/pe/RCS/symbol.c,v 1.1 2006/09/15 16:29:46 marka Exp marka $
 *
 * Name:	MicroEMACS
 *		Symbol table stuff.
 * Version:	29
 * Modified by:	Mark Alexander (amdahl!drivax!alexande)
 *
 * Symbol tables, and keymap setup.
 * The terminal specific parts of building the
 * keymap has been moved to a better place.
 *
 * $Log: symbol.c,v $
 * Revision 1.4  2005-10-18 02:18:10  bloovis
 * Rename some things to avoid conflict with ncurses.
 *
 * Revision 1.3  2004/04/20 15:18:18  bloovis
 * (namemacro):  Use ereadv instead of passing NULL arg list
 * pointer to eread.
 *
 * Revision 1.2  2003/12/03 22:14:35  bloovis
 * (USE_VMWAREINDENT): New macro to control whether to use
 * VMware-style indenting by default, initially enabled.
 * (USE_GNUINDENT): Disable.
 * (key): If USE_VMWAREINDENT enabled, bind C-J to vmwareindent.
 *
 * Revision 1.1.1.1  2003/11/06 02:51:52  bloovis
 * Imported sources
 *
 * Revision 1.8  2003/05/14 23:09:19  malexander
 * (mouse_event): Rename to mouseevent to avoid conflict with
 * Win32 header files.
 * (findcscope, findgrep, nextcscope): Disable on non-Linux host.
 *
 * Revision 1.7  2002/01/23 22:36:06  malexander
 * (key): Add mouse_event.
 *
 * Revision 1.6  2001/01/09 01:26:14  malexander
 * (key): Add entry for nextcscope.
 *
 * Revision 1.5  2000/09/29 00:19:38  malexander
 * Numerous changes to eliminate warnings and add prototypes.
 *
 * Revision 1.4  2000/07/27 15:17:39  malexander
 * (key): Bind prepgrep to M-G.
 *
 * Revision 1.3  2000/07/25 20:02:24  malexander
 * Add support for new commands: borlandindent, freetags,
 * and findcscope.  Eliminate some compiler warnings.
 *
 * Revision 1.2  2000/07/21 16:20:32  malexander
 * Reformatted with GNU indent.
 *
 * Revision 1.1.1.1  2000/07/14 19:23:11  malexander
 * Imported sources
 *
 * Revision 1.5  1996/10/22 16:08:14  marka
 * Fix bug in command name autocompletion.
 *
 * Revision 1.4  91/04/19  23:27:10  alexande
 * Added set-overstrike command.  Bind set-save-tabs to M-I.
 * Move support for autocompletion of symbol names from echo.c
 * to new routine symsearch() in this module.
 * 
 * Revision 1.3  91/01/07  10:29:21  alexande
 * Remove C++ warnings.  Add set-tab-size command.
 * 
 * Revision 1.2  89/01/13  13:01:33  MGA
 * Added getbinding() routine to look up a key binding.  Used by
 * incremental search.
 * 
 * Revision 1.1  89/01/13  11:07:38  MGA
 * Initial revision
 *
 */
#include	"def.h"

/* #define USE_VMWAREINDENT */	/* Define to use VMware indent	*/
#define USE_GNUINDENT		/* Define to use gnu indent	*/
/* #define USE_BORLANDINDENT */	/* Define to use Borland indent	*/
/* #define USE_RUBYINDENT */	/* Define to use Ruby indent */

#if defined(__linux__) || defined(__APPLE__) || defined(__CYGWIN__) || defined(__FreeBSD__)
  #define HAS_CSCOPE 1
#else
  #define HAS_CSCOPE 0
#endif

/*
 * Key binding structure.
 */
typedef struct
{
  int k_key;			/* Key to bind.                 */
  int (*k_funcp) ();		/* Function.                    */
  char *k_name;			/* Function name string.        */
}
KEY;

/*
 * Default key binding table. This contains
 * the function names, the symbol table name, and (possibly)
 * a key binding for the builtin functions. There are no
 * bindings for C-U or C-X. These are done with special
 * code, but should be done normally.
 */
KEY key[] = {
  {KCTRL | '@',		setmark,	"set-mark"},
  {KCTRL | 'A',		gotobol,	"goto-bol"},
  {KCTRL | 'B',		backchar,	"back-char"},
  {KCTRL | 'C',		spawncli,	"spawn-cli"},
  {KCTRL | 'D',		forwdel,	"forw-del-char"},
  {KCTRL | 'E',		gotoeol,	"goto-eol"},
  {KCTRL | 'F',		forwchar,	"forw-char"},
  {KCTRL | 'G',		ctrlg,		"abort"},
  {KCTRL | 'H',		backdel,	"back-del-char"},
  {KCTRL | 'I',		selfinsert,	"ins-self"},
#if defined(USE_BORLANDINDENT)
  {KCTRL | 'J',		borlandindent,	"borland-indent"},
  {-1,			gnuindent,	"gnu-indent"},
  {-1,			vmwareindent,	"vmware-indent"},
  {-1,			rubyindent,	"ruby-indent"},
  {-1,			indent,		"ins-nl-and-indent"},
#elif defined(USE_GNUINDENT)
  {-1,			borlandindent,	"borland-indent"},
  {KCTRL | 'J',		gnuindent,	"gnu-indent"},
  {-1,			vmwareindent,	"vmware-indent"},
  {-1,			rubyindent,	"ruby-indent"},
  {-1,			indent,		"ins-nl-and-indent"},
#elif defined(USE_VMWAREINDENT)
  {-1,			borlandindent,	"borland-indent"},
  {-1,			gnuindent,	"gnu-indent"},
  {KCTRL | 'J',		vmwareindent,	"vmware-indent"},
  {-1,			rubyindent,	"ruby-indent"},
  {-1,			indent,		"ins-nl-and-indent"},
#elif defined(USE_RUBYINDENT)
  {-1,			borlandindent,	"borland-indent"},
  {-1,			gnuindent,	"gnu-indent"},
  {-1,			vmwareindent,	"vmware-indent"},
  {KCTRL | 'J',		rubyindent,	"ruby-indent"},
  {-1,			indent,		"ins-nl-and-indent"},
#else
  {-1,			borlandindent,	"borland-indent"},
  {-1,			gnuindent,	"gnu-indent"},
  {-1,			vmwareindent,	"vmware-indent"},
  {-1,			rubyindent,	"ruby-indent"},
  {KCTRL | 'J',		indent,		"ins-nl-and-indent"},
#endif
  {KCTRL | 'K',		killline,	"kill-line"},
  {KCTRL | 'L',		erefresh,	"refresh"},
  {KCTRL | 'M',		newline,	"ins-nl"},
  {KCTRL | 'N',		forwline,	"forw-line"},
  {KCTRL | 'O',		openline,	"ins-nl-and-backup"},
  {KCTRL | 'P',		backline,	"back-line"},
  {KCTRL | 'Q',		quote,		"quote"},
  {KCTRL | 'R',		backsearch,	"back-search"},
  {KCTRL | 'S',		forwsearch,	"forw-search"},
  {KCTRL | 'T',		twiddle,	"twiddle"},
  {KCTRL | 'V',		forwpage,	"forw-page"},
  {KCTRL | 'W',		killregion,	"kill-region"},
  {KCTRL | 'Y',		yank,		"yank"},
  {KCTRL | 'Z',		backpage,	"back-page"},	/* like MINCE */
/* {	KCTRL|'Z',	jeffexit,	"jeff-exit" }, */
  {KCTLX | KCTRL | 'B', listbuffers,	"display-buffers"},
  {KCTLX | KCTRL | 'C', quit,		"quit"},
  {KCTLX | KCTRL | 'E', eecho,		"echo"},
  {KCTLX | KCTRL | 'F', filename,	"set-file-name"},
  {KCTLX | KCTRL | 'I', fileinsert,	"file-insert"},
  {KCTLX | KCTRL | 'L', lowerregion,	"lower-region"},
  {KCTLX | KCTRL | 'N', mvdnwind,	"down-window"},
  {KCTLX | KCTRL | 'O', deblank,	"del-blank-lines"},
  {KCTLX | KCTRL | 'P', mvupwind,	"up-window"},
  {KCTLX | KCTRL | 'Q', togglereadonly,	"toggle-readonly"},
  {KCTLX | KCTRL | 'R', fileread,	"file-read"},
  {KCTLX | KCTRL | 'S', filesave,	"file-save"},
  {KCTLX | KCTRL | 'U', upperregion,	"upper-region"},
  {KCTLX | KCTRL | 'V', filevisit,	"file-visit"},
  {KCTLX | KCTRL | 'W', filewrite,	"file-write"},
  {KCTLX | KCTRL | 'X', swapmark,	"swap-dot-and-mark"},
  {KCTLX | KCTRL | 'Z', shrinkwind,	"shrink-window"},
  {KCTLX | '+',		balancewindows,	"balance-windows"},
  {KCTLX | '=',		showcpos,	"display-position"},
  {KCTLX | '(',		ctlxlp,		"start-macro"},
  {KCTLX | ')',		ctlxrp,		"end-macro"},
  {KCTLX | '1',		onlywind,	"only-window"},
  {KCTLX | '2',		splitwind,	"split-window"},
  {KCTLX | 'B',		usebuffer,	"use-buffer"},
  {KCTLX | 'E',		ctlxe,		"execute-macro"},
  {KCTLX | 'F',		readprofile,	"read-profile"},
  {KCTLX | 'G',		gotoline,	"goto-line"},
  {KCTLX | 'H',		checkheap,	"check-heap"},
#ifndef MINGW
  {KCTLX | 'I',		spellregion,	"spell-region"},
#endif
  {KCTLX | 'K',		killbuffer,	"kill-buffer"},
  {KCTLX | 'N',		nextwind,	"forw-window"},
  {KCTLX | 'P',		prevwind,	"back-window"},
  {KCTLX | 'R',		backisearch,	"back-i-search"},
  {KCTLX | 'S',		forwisearch,	"forw-i-search"},
  {KCTLX | 'U',		undo,		"undo"},
#if 0
  {KCTLX | 'V',		filevisitreadonly, "file-visit-readonly"},
#else
  {-1,			filevisitreadonly, "file-visit-readonly"},
#endif
  {KCTLX | 'Z',		enlargewind,	"enlarge-window"},
  {KMETA | KCTRL | 'E',	gccerror,	"gcc-error"},
  {KMETA | KCTRL | 'F',	foldcase,	"fold-case"},
  {KMETA | KCTRL | 'H', delbword,	"back-del-word"},
  {KMETA | KCTRL | 'I', settabsize,	"set-tab-size"},
  {KMETA | KCTRL | 'R', backregsearch,	"back-regexp-search"},
  {KMETA | KCTRL | 'S', forwregsearch,	"forw-regexp-search"},
  {KMETA | KCTRL | 'U', unicode,	"unicode"},
  {KMETA | KCTRL | 'V', showversion,	"display-version"},
  {KMETA | KCTRL | 'W', killpara,	"kill-paragraph"},
  {KMETA | '.',		findtag,	"find-tag"},
#if HAS_CSCOPE
  {KMETA | ',',		findcscope,	"find-cscope"},
#endif
  {KMETA | '!',		reposition,	"reposition-window"},
  {KMETA | '>',		gotoeob,	"goto-eob"},
  {KMETA | '<',		gotobob,	"goto-bob"},
  {KMETA | '[',		gotobop,	"back-paragraph"},
  {KMETA | ']',		gotoeop,	"forw-paragraph"},
  {KMETA | '+',		indentregion,	"indent-region"},
  {KMETA | '/',		regrepl,	"reg-replace"},
  {KMETA | '?',		regqueryrepl,	"reg-query-replace"},
#ifndef MINGW
  {KMETA | '$',		spellword,	"spell-word"},
#endif
  {KMETA | 'B',		backword,	"back-word"},
  {KMETA | 'C',		capword,	"cap-word"},
  {KMETA | 'D',		delfword,	"forw-del-word"},
  {KMETA | 'F',		forwword,	"forw-word"},
#if HAS_CSCOPE
  {KMETA | 'G',		findgrep,	"find-grep"},
#endif
  {KMETA | 'H',		searchagain,	"search-again"},
  {KMETA | 'I',		setsavetabs,	"set-save-tabs"},
  {KMETA | 'J',		fillpara,	"fill-paragraph"},
  {KMETA | 'L',		lowerword,	"lower-word"},
  {KMETA | 'P',		searchparen,	"search-paren"},
  {KMETA | 'Q',		queryrepl,	"query-replace"},
  {KMETA | 'R',		replstring,	"replace-string"},
  {KMETA | 'T',		filesave,	"file-save"},
  {KMETA | 'U',		upperword,	"upper-word"},
  {KMETA | 'V',		backpage,	"back-page"},
  {KMETA | 'W',		copyregion,	"copy-region"},
  {KMETA | 'X',		extend,		"extended-command"},
  {-1,			help,		"help"},
  {-1,			wallchart,	"display-bindings"},
  {-1,			bindtokey,	"bind-to-key"},
  {-1,			namemacro,	"name-macro"},
  {-1,			fillword,	"ins-self-with-wrap"},
  {-1,			setfillcol,	"set-fill-column"},
  {-1,			delwhite,	"just-one-space"},
  {-1,			nextbuffer,	"forw-buffer"},
  {-1,			prevbuffer,	"back-buffer"},
  {-1,			insertmacro,	"ins-macro"},
  {-1,			setoverstrike,	"set-overstrike"},
  {-1,			freetags,	"free-tags"},
#if HAS_CSCOPE
  {-1,			nextcscope,	"next-cscope"},
#endif
  {-1,			mouseevent,	"mouse-event"},
#if USE_RUBY
  {-1,			rubystring,	"ruby-string"},
  {-1,			rubycommand,	"ruby-command"},
  {-1,			rubyload,	"ruby-load"},
#endif
  {-1,			jeffexit,	"jeff-exit"},
  {-1,			displaymessage,	"display-message"},
  {-1,			redo,		"redo"}
};

#define	NKEY	(sizeof(key) / sizeof(key[0]))


/*
 * Structure used to store key bindings in a hash table;
 * a hash of the key value is bucket index.
 */
typedef struct BINDING
{
  struct BINDING *bi_next;		/* Next binding in list		*/
  int bi_key;				/* Key value			*/
  SYMBOL *bi_symbol;			/* Pointer to symbol		*/
} BINDING;

static BINDING *binding[NSHASH];	/* Key bindings.                */

/*
 * A mode is a string containing the name of the mode, and
 * a set of key bindings.  A mode is attached to a buffer,
 * but may be missing
 */
typedef struct MODE
{
  char *m_name;
  BINDING *m_binding[NSHASH];
} MODE;

/*
 * Take a string, and compute the symbol table
 * bucket number. This is done by adding all of the characters
 * together, and taking the sum mod NSHASH. The string probably
 * should not contain any GR characters; if it does the "*cp"
 * may get a negative number on some machines, and the "%"
 * will return a negative number!
 */
static int
symhash (const char *cp)
{
  register int c;
  register int n;

  n = 0;
  while ((c = *cp++) != 0)
    n += c;
  return (n % NSHASH);
}

/*
 * Take a key code, and compute the binding table
 * bucket number. This is done by adding all of the bytes
 * together, and taking the sum mod NSHASH.
 */
int
keyhash (int key)
{
  int i, c, n;

  n = 0;
  for (i = 0; i < 4; i++)
    {
      c = (key >> (i * 8)) & 0xff;
      n += c;
    }
  return (n % NSHASH);
}

/*
 * Return a pointer to the SYMBOL node that is bound
 * to a particular key.  Look first in the mode binding
 * table, then in the global table.
 */
static SYMBOL *
findbinding (int key, BINDING **table)
{
  BINDING *bp;
  int hash;

  hash = keyhash (key);
  for (bp = table[hash]; bp != NULL; bp = bp->bi_next)
    if (bp->bi_key == key)
      return bp->bi_symbol;
  return NULL;		/* not found */
}

/*
 * Return a pointer to the SYMBOL node that is bound
 * to a particular key.  Look first in the mode binding
 * table, then in the global table.
 */
SYMBOL *
getbinding (int key)
{
  SYMBOL *s = NULL;
  if (curbp == NULL || curbp->b_mode == NULL ||
      (s = findbinding (key, curbp->b_mode->m_binding)) == NULL)
    s = findbinding (key, binding);
  return s;
}

/*
 * Add a key binding to the specified binding table,
 * which is either the global binding table or a mode
 * binding table.
 */
static void
addbinding (int key, SYMBOL *sym, BINDING **table)
{
  BINDING *bp, *bp1;
  int hash;

  /* If a binding already exists for this binding, modify it
   * to point to the new symbol.
   */
  hash = keyhash (key);
  bp = NULL;
  for (bp1 = table[hash]; bp1 != NULL; bp1 = bp1->bi_next)
    if (bp1->bi_key == key)
      {
	bp = bp1;
	break;
      }
  if (bp != NULL)
    --bp->bi_symbol->s_nkey;	/* Unbind from old symbol */
  else
    {
      /* Create a new binding, insert into hash chain. */
      bp = calloc (1, sizeof (*bp));
      if (bp == NULL)
	abort ();
      bp->bi_next = table[hash];
      bp->bi_key = key;
      table[hash] = bp;
    }
  bp->bi_symbol = sym;		/* Bind to new symbol */
  ++sym->s_nkey;
}

/*
 * Add the binding for a key to the specified symbol to
 * the global binding table.
 */
void
setbinding (int key, SYMBOL *sym)
{
  addbinding (key, sym, binding);
}

/*
 * Add the binding for a key to the specified symbol to
 * the mode binding table, if it exists.  Otherwise,
 * add it to the global binding table.
 */
void
setmodebinding (int key, SYMBOL *sym)
{
  if (curbp != NULL && curbp->b_mode != NULL)
    addbinding (key, sym, curbp->b_mode->m_binding);
  else
    addbinding (key, sym, binding);
}

/*
 * Symbol table lookup.
 * Return a pointer to the SYMBOL node, or NULL if
 * the symbol is not found.
 */
SYMBOL *
symlookup (const char *cp)
{
  register SYMBOL *sp;

  sp = symbol[symhash (cp)];
  while (sp != NULL)
    {
      if (strcmp (cp, sp->s_name) == 0)
	return (sp);
      sp = sp->s_symp;
    }
  return (NULL);
}

/*
 * Build initial keymap. The funny keys
 * (commands, odd control characters) are mapped using
 * a big table and calls to "keyadd". The printing characters
 * are done with some do-it-yourself handwaving. The terminal
 * specific keymap initialization code is called at the
 * very end to finish up. All errors are fatal.
 */
void
keymapinit ()
{
  register SYMBOL *sp;
  register KEY *kp;
  register int i;

  for (i = 0; i < NSHASH; ++i)
    binding[i] = NULL;
  for (kp = &key[0]; kp < &key[NKEY]; ++kp)
    keyadd (kp->k_key, kp->k_funcp, kp->k_name);
  keydup (KCTLX | KCTRL | 'G', "abort");
  keydup (KMETA | KCTRL | 'G', "abort");
  keydup (0x7F, "back-del-char");
  keydup (KMETA | ' ', "set-mark");
  keydup (KMETA | '%', "query-replace");
  keydup (KCTLX | 'Q', "quote");	/* For terminals   */
  keydup (KMETA | 'S', "forw-search");	/*  using xon/xoff */
  keydup (KMETA | 0x7F, "back-del-word");
  keydup (KCTLX | 'O', "forw-window");	/* like MINCE */
  /*
   * Should be bound by "tab" already.
   */
  if ((sp = symlookup ("ins-self")) == NULL)
    abort ();
  for (i = 0x20; i < 0x7F; ++i)
    {
      if (getbinding (i) != NULL)
	abort ();
      setbinding (i, sp);
    }
  ttykeymapinit ();
}

/*
 * Add a symbol to the appropriate hash chain.
 */
static void
addsym (SYMBOL *sp)
{
  int hash = symhash (sp->s_name);
  sp->s_symp = symbol[hash];
  symbol[hash] = sp;
}

/*
 * Create a new builtin function "name"
 * with function "funcp". If the "new" is a real
 * key, bind it as a side effect. All errors
 * are fatal.
 */
void
keyadd (
     int new,
     int (*funcp) (),
     const char *name)
{
  register SYMBOL *sp;

  if ((sp = (SYMBOL *) malloc (sizeof (SYMBOL))) == NULL)
    abort ();
  sp->s_nkey = 0;
  sp->s_name = name;
  sp->s_funcp = funcp;
  sp->s_macro = NULL;
  addsym (sp);			/* Add symbol to hash chain.	*/
  if (new >= 0)
    {				/* Bind this key.       */
      if (getbinding (new) != NULL)
	abort ();
      setbinding (new, sp);
    }
}

/*
 * Bind key "new" to the existing
 * routine "name". If the name cannot be found,
 * or the key is already bound, abort.
 */
void
keydup (int new, const char *name)
{
  register SYMBOL *sp;

  if (getbinding (new) != NULL || (sp = symlookup (name)) == NULL)
    abort ();
  setbinding (new, sp);
}

/*
 * Find out which key is currently bound to the specified command name.
 * We do this with forw-i-search and back-i-search so that the
 * user can use those keys in incremental search mode instead
 * of Control-S and Control-R.  -1 is returned if there is no key
 * bound to that command.
 */
int
getbindingforcmd (const char *s)
{
  SYMBOL *sp;
  BINDING *bp;
  int hash;

  if ((sp = symlookup (s)) == NULL)
    return (-1);

  for (hash = 0; hash < NSHASH; hash++)
    {
      for (bp = binding[hash]; bp != NULL; bp = bp->bi_next)
	if (bp->bi_symbol == sp)
	  return bp->bi_key;
    }
  return (-1);
}

/*
 * Prompt for a name, and associate the current
 * macro with the symbol of that name.
 * If the symbol already exists and is a macro, that macro is
 * deleted first.  If the symbol does not exist, it is created.
 * The body of the macro is saved in a buffer that is attached to
 * the symbol.  Return FALSE if an error occurs; otherwise TRUE.
 */
int
namemacro (int f, int n, int k)
{
  register SYMBOL *sp;		/* Symbol name pointer. */
  register int *mp;		/* Macro pointer.       */
  char xname[NXNAME];		/* Symbol name.         */
  register int msize;		/* Size of macro.       */
  register int s;		/* Status code.         */

  /* Can't do this while executing or defining a macro.
   */
  if (kbdmip != NULL || kbdmop != NULL)
    {
      eprintf ("Not now");
      return (FALSE);
    }

  /* Read the name of the symbol to use, store the name in
   * a local array.
   */
  if ((s = ereadv ("Macro name: ", xname, NXNAME, EFAUTO)) != TRUE)
    return (s);

  /* Compute the size of the macro, then allocate space
   * to copy the macro into.
   */
  msize = sizeof (kbdm[0]);
  for (mp = kbdm; *mp != (KCTLX | ')'); mp++)
    msize += sizeof (kbdm[0]);
  if ((mp = (int *) malloc (msize)) == NULL)
    {
      eprintf ("Out of memory.");
      return (FALSE);
    }
  memcpy (mp, kbdm, msize);	/* Copy the macro       */

  /* See if the symbol already exists.  If it doesn't, create
   * a new symbol; otherwise reuse it if it isn't a function.
   */
  if ((sp = symlookup (xname)) == NULL)
    {
      /* Allocate a new symbol structure.
       */
      if ((sp = (SYMBOL *) malloc (sizeof (SYMBOL))) == NULL)
	{
	  eprintf ("Out of memory.");
	  free (mp);
	  return (FALSE);
	}

      /* Copy the symbol name and add the symbol to the hash chain.
       */
      if ((sp->s_name = strdup (xname)) == NULL)
	{
	  eprintf ("Out of memory.");
	  free (mp);
	  free (sp);
	  return (FALSE);
	}
      sp->s_nkey = 0;
      addsym (sp);
    }
  else
    {				/* Symbol exists.       */
      if (sp->s_macro == NULL)
	{			/* Symbol isn't macro?  */
	  eprintf ("Symbol is already defined.");
	  free (mp);
	  return (FALSE);
	}
      free (sp->s_macro);	/* Get rid of old macro */
    }

  sp->s_macro = mp;		/* Set the macro ptr.   */
  return (TRUE);		/* Successful return.   */
}

/*
 * Search for a symbol, given a partial name.
 * If prev is null, start searching at the beginning of the list.
 * Otherwise resume the search from the previous point.  Return the name
 * of the first symbol that matches the partial name, or NULL
 * if no match can be found.
 */
const char *
symsearch (
     const char *sname,		/* partial symbol name to match         */
     int cpos,			/* number of characters in symbol name  */
     const char *prev)		/* NULL if starting from beginning      */
{
  static int h;
  static SYMBOL *sp;
  const char *name;

  if (prev == NULL)
    {				/* restart search at beginning?         */
      h = 0;
      sp = NULL;
    }
  for (;;)
    {
      while (sp == NULL)
	{
	  if (h == NSHASH)	/* tried all hash chains */
	    return (NULL);
	  sp = symbol[h++];	/* next hash chain      */
	}
      name = sp->s_name;	/* save name            */
      sp = sp->s_symp;		/* skip to next symbol  */
      if (strncmp (sname, name, cpos) == 0)
	return (name);
    }
}

/*
 * Sort the pop-up buffer so that it is in
 * order by key name.  This uses the terribly
 * slow bubblesort algorithm, but it's fast enough
 * for this purpose.
 */
static void
sortblist (void)
{
  int swapped, done;
  LINE *lp, *next;

  done = FALSE;
  while (!done)
    {
      swapped = FALSE;
      for (lp = firstline (blistp);
	   lp != blistp->b_linep &&
	   (next = lforw (lp)) != blistp->b_linep;
	   lp = next)
	{
	  int len, nlen;

	  len = llength (lp);
	  nlen = llength (next);
	  if (len > nlen)
	    len = nlen;
	  if (strncmp ((const char *) lgets (lp),
	               (const char *) lgets (next), len) > 0)
	    {
	      /* Swap lp and next.
	       */
	      lback (lforw (next)) = lp;
	      lforw (lp) = lforw (next);
	      lforw (lback (lp)) = next;
	      lback (next) = lback (lp);
	      lback (lp) = next;
	      lforw (next) = lp;
	      swapped = TRUE;
	      next = lp;
	    }
	}
      if (!swapped)
	done = TRUE;
    }
}

/*
 * Helper function for wallchart.  It searches the specified
 * binding table, and adds all entries to the popup buffer.
 * If mode is TRUE, search the current buffer's mode table,
 * and add a '+' character to each entry; otherwise
 * search the global binding table.  Return TRUE if success.
 */
static int
showbindings (int f, int mode)
{
  register int key;
  register SYMBOL *sp;
  register char *cp1;
  char buf[64];
  BINDING *bp;
  BINDING **table;
  int hash;

  if (mode == TRUE)
    {
      if (curbp == NULL || curbp->b_mode == NULL)
	return TRUE;
      table = curbp->b_mode->m_binding;
    }
  else
    table = binding;
  for (hash = 0; hash < NSHASH; hash++)
    {
      for (bp = table[hash]; bp != NULL; bp = bp->bi_next)
	{
	  key = bp->bi_key;
	  sp = bp->bi_symbol;
	  if (f != FALSE || strcmp (sp->s_name, "ins-self") != 0)
	    {
	      ekeyname (buf, key);
	      cp1 = &buf[0];	/* Find end.            */
	      while (*cp1 != '\0')
		++cp1;
	      while (cp1 < &buf[16])	/* Goto column 16.      */
		*cp1++ = ' ';
	      strcpy (cp1, sp->s_name);	/* Add function name.   */
	      if (mode)
		strcat (cp1++, "+");
	      if (addline (buf) == FALSE)
		return (FALSE);
	    }
	}
    }
  return TRUE;
}

/*
 * This function creates a table, listing all
 * of the command keys and their current bindings, and stores
 * the table in the standard pop-op buffer (the one used by the
 * directory list command, the buffer list command, etc.). This
 * lets MicroEMACS produce its own wall chart. The bindings to
 * "ins-self" are only displayed if there is an argument.
 */
int
wallchart (int f, int n, int k)
{
  register int s;

  if ((s = bclear (blistp)) != TRUE)	/* Clear it out.        */
    return (s);
  strcpy (blistp->b_fname, "");
  if (showbindings (f, FALSE) != TRUE)
    return FALSE;
  if (showbindings (f, TRUE) != TRUE)
    return FALSE;
  sortblist ();
  return (popblist ());
}

/*
 * Remove the current mode structure from the current
 * buffer and free all of its substructures.
 */
void
removemode (BUFFER *bp)
{
  MODE *m;
  int i;

  if (bp == NULL)
    return;
  m = bp->b_mode;
  if (m == NULL)
    return;
  free (m->m_name);
  for (i = 0; i < NSHASH; i++)
    {
      BINDING *b, *next;

      for (b = m->m_binding[i]; b != NULL; b = next)
	{
	  next = b->bi_next;
	  free (b);
	}
    }
  bp->b_mode = NULL;
}

/*
 * Create a new mode structure with the given name and
 * an empty key binding table, and attach it to the
 * current buffer.  If there is already a mode structure
 * for the buffer, remove it first.
 */
void
createmode (const char *name)
{
  MODE *m;

  if (curbp == NULL)
    return;
  if (curbp->b_mode != NULL)
    removemode (curbp);
  m = calloc (1, sizeof (*m));
  if (m == NULL)
    {
      eprintf ("Unable to create mode structure!");
      return;
    }
  m->m_name = strdup (name);
  if (m->m_name == NULL)
    {
      eprintf ("Unable to allocate mode name!");
      return;
    }
  curbp->b_mode = m;
  curwp->w_flag |= WFMODE;	/* Force redisplay of mode line */
}

/*
 * Return the specified buffer's mode's name, or NULL if the buffer
 * has no mode.
 */
const char *
modename (BUFFER *bp)
{
  if (bp != NULL && bp->b_mode != NULL)
    return bp->b_mode->m_name;
  else
    return NULL;
}
