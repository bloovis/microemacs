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
 * Name:	MicroEmacs
 * Version:	30
 *		Termcap keyboard driver
 * Created:	21-Aug-1986
 *		Mic Kaczmarczik ...!ihnp4!seismo!ut-sally!ut-ngp!mic
 * Last edit:	03-Sep-86
 *
 * [ Several of the nasty comments about the XKEYS code are
 *   by me.  [Bob Larson (usc-oberon!blarson)]  It is my opinion
 *   that function keys cannot be made to work with standard
 *   emacs keybindings except on a very limited set of terminals.
 *   I just work with to many that do not fit the assumptions Mic's
 *   XKEYS code makes to consider it useful to me, and think that
 *   others considering using this code should look and see what
 *   it realy does first.
 * ]
 *
 * If XKEYS is defined this routine looks for the following 
 * termcap sequences, which are obtained by "tty.c":
 *
 *	ks	-- start using the function keypad
 *	ke	-- finish using the function keypad
 *	kh	-- home key
 *	ku	-- up arrow
 *	kd	-- down arrow
 *	kl	-- left arrow
 *	kr	-- right arrow
 *	k0-k9	-- standard termcap function keys
 *	l0-l9	-- labels for termcap function keys
 *	(nonstandard)
 *	K0-K9	-- extra keys that we look for -- the get mapped
 *		   internally to F10-F19
 *	L0-L9	-- labels for same.
 *
 * Bugs/features/problems:
 *
 *	XKEYS and DPROMPT do not work together well.
 *
 *	If the META introducer is used as the initial character of
 *	a function key sequence, what should the key parser do when the
 *	user wants to type a META-ed key, or just the META introducer
 *	alone?	This is of practical importance on DEC terminals, where
 *	the META introducer is the Escape key.  Even worse things happen
 *	on terminals that have something (or more than one thing) other
 *	than the META introducer as the inital character of a function
 *	sequence.
 *
 *	The approach I took was that if the META introducer is the first
 *	character in a function sequence, and the second character c
 *	isn't part of a function key sequence, the parser returns
 *	(KMETA | c).  If it sees two META introducers in a row, it
 *	returns one instance of METACH.   This approach is subject to
 *	discussion and debate, but it works.  [In at lease some cases.]
 *
 *	If the META introducer is NOT the first character in a function
 *	sequence (including arrow keys) this code has a very nasty
 *	side effect of eating that key.  For example, on an Adds viewpoint
 *	60, six normal control characters are eaten if you have defined
 *	XKEYS and put the keys in the termcap.  More than a little 
 *	creativity is needed because ^U is one of the arrow keys, and
 *	prefixes aren't bindable.
 *
 *	[ From a quick look at the code, it seems that a single character
 *	  function key won't work, but it is still put in the table.
 *	]
 */
#include	"def.h"

#define DEBUG	0

#define SUNOS 	0		/* set to 1 for SunOS, 0 for everybody else */

#define	KEY	short

/*
 * Default key name table.  Can be overridden by
 * definitions of l0-l9 in the termcap entry.  You
 * can't redefine the names for the arrow keys
 * and the home key.
 */

#ifdef	XKEYS
/* key sequences (from tty.c) */
extern char *K[], *L[], *KS, *KE, *KH, *KU, *KD, *KL, *KR;
char *keystrings[32] = {
  NULL, "Up", "Down", "Left",
  "Right", "PgUp", "PgDn", "Home",
  "End", "Insert", "Delete", "F1",
  "F2", "F3", "F4", "F5",
  "F6", "F7", "F8", "F9",
  "F10", "S-F1", "S-F2", "S-F3",
  "S-F4", "S-F5", "S-F6", "S-F7",
  "S-F8", "S-F9", "S-F10", "mouse"
};

/* The character sequences produced by the various keys
 * on Solaris and Linux.  These aren't described in any
 * termcap entry; their values were determined empirically.
 */
#if SUNOS
char *keycodes[32] = {
/*80*/ NULL, "\033OA", "\033OB", "\033OD",
/*84*/ "\033OC", "\033[5~", "\033[6~", "\033O\377",
/*88*/ "\033O\020", "\033[2~", NULL, "\033OP",
/*8c*/ "\033OQ", "\033OR", "\033OS", "\033OT",
  "\033On", "\033Os", "\033Ow", "\033Ox",
  "\033Oy", NULL, NULL, NULL,
  NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL
};

#else

char *keycodes[32] = {
/*80*/ NULL, "\033OA", "\033OB", "\033OD",
/*84*/ "\033OC", "\033[5~", "\033[6~", "\033OH",
/*88*/ "\033OF", "\033[2~", NULL, "\033OP",
/*8c*/ "\033OQ", "\033OR", "\033OS", "\033[15~",
  "\033[17~", "\033[18~", "\033[19~", "\033[20~",
  "\033[21~", "\033[23~", "\033[24~", "\033[25~",
  "\033[26~", "\033[28~", "\033[29~", "\033[31~",
  "\033[32~", "\033[33~", "\033[34~", "\033[M"
};

#endif /* linux or solaris */

#else /* XKEYS */

char *keystrings[] = {
  NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL
};
#endif

#ifdef	XKEYS
/*
 * Type declarations for data structure we
 * use to parse for function key sequences
 */
#define	NODE		0	/* internal node                */
#define	VALUE		1	/* internal key code value      */
#define SENTINEL	2	/* sentinel value               */

typedef struct trienode
{
  int type;			/* one of NODE, LEAF */
  struct trienode *sibling, *child;
  KEY value;
}
TRIENODE, *TRIE;

TRIE keywords, sentinel;
#endif

/*
 * Forward declarations.
 */
#if DEBUG
static void tprint (TRIE p, int level);
#endif
static void tdelete (TRIE t);
static void adddict (char *kstr, KEY kcode);
static void tinit (void);
static void tdelete (TRIE t);
static TRIE talloc (void);
static TRIE tinsert (const char *kstring, KEY kcode, TRIE first);
static int parse (TRIE first);
static int parse_or_mouse (TRIE first);


/*
 * Get keyboard character, and interpret
 * any special keys on the keyboard.  If XKEYS is
 * #defined, use a dictionary organized as a
 * trie to keep the parsing overhead down.
 *
 * To keep the function call overhead down, do the
 * first level of parse() inside getkbd().
 *
 * Also, since ESC (the usual value of METACH) is
 * the first character in many function key sequences,
 * we  return (KMETA | ch) if METACH-<ch> is not
 * the start of an escape sequence.  Blecch.  Furthermore,
 * if we see METACH-METACH, we return the value METACH.
 * Phhhht.
 */
int
getkbd (void)
{
#ifndef	XKEYS
  return (ttgetc ());
#else
  register TRIE t;
  register int c;

  c = ttgetc ();
#if DEBUG
  fprintf (stderr, "getkbd(1): c = %x\n", c);
#endif
  for (t = keywords; t->type == NODE; t = t->sibling)
    if (t->value == c)
      {				/* possible function key sequence  */
	if (c != METACH)
	  return parse_or_mouse (t->child);
	else
	  {			/* maybe sequence, maybe META char */
	    c = ttgetc ();
#if DEBUG
	    fprintf (stderr, "getkbd(2): c = %x\n", c);
#endif
	    for (t = t->child; t->type == NODE; t = t->sibling)
	      if (t->value == c)
		return parse_or_mouse (t->child);
	    /* METACH-METACH -> METACH */
	    if (c == METACH)
	      return (METACH);
	    /* Else make c into a META character */
	    if (CISLOWER (c) != FALSE)
	      c = CTOUPPER (c);
	    if (c >= 0x00 && c <= 0x1F)
	      c = KCTRL | (c + '@');
	    return (KMETA | c);
	  }
      }
  return (c);
#endif
}

#ifdef	XKEYS
static int
parse (first)
     TRIE first;
{
  register TRIE t;
  register int c;

  if (first->type == VALUE)	/* found a match!       */
    {
#if DEBUG
      fprintf (stderr, "parse() returning first->value = %x\n", first->value);
#endif
      return (first->value);
    }

  c = ttgetc ();
#if DEBUG
  fprintf (stderr, "parse: c = %x\n", c);
#endif
  for (t = first; t->type == NODE; t = t->sibling)	/* look thru list   */
    if (t->value == c)
      {
#if DEBUG
	fprintf (stderr, "parse() returning parse(t->child = %p)\n",
		 t->child);
#endif
	return (parse (t->child));	/* try next level   */
      }
#if DEBUG
  fprintf (stderr, "parse() returning c = %x\n", c);
#endif
  return (c);			/* nothing matched */
}
#endif

static int
parse_or_mouse (TRIE first)
{
  int c;

  c = parse (first);
  if (c == KMOUSE)
    {
      mouse_button = ttgetc () - ' ';
      mouse_column = ttgetc () - ' ' - 1;
      mouse_row    = ttgetc () - ' ' - 1;
#if DEBUG
      fprintf (stderr, "button %d, row %d, column %d\n",
	       mouse_button, mouse_row, mouse_column);
#endif
    }
  return c;
}

/*
 * If XKEYS is defined, get key definitions from the termcap 
 * entry and put them in the parse table.
 */
void
ttykeymapinit (void)
{
#ifdef	XKEYS
  register int i;

  if (KS && *KS)		/* turn on keypad       */
    putpad (KS);

  tinit ();			/* set up initial trie */

#ifdef IGNORE_TERMCAP		/* ignore the termcap definitions for F-keys  */

  /*
   * Enter escape sequence for  PC-specific keys, ignore the termcap
   * entries.  These should work on both Sun boxes and Linux.
   */
  for (i = 0; i < 32; i++)
    if (keycodes[i])
      adddict (keycodes[i], KFIRST + i);
#else

  for (i = 0; i < NFKEYS; i++)
    {
      if (L[i] && *L[i])	/* record new name */
	keystrings[(KF0 - KFIRST) + i] = L[i];
      if (K[i] && *K[i])
	adddict (K[i], (KEY) (KF0 + i));
      else
	adddict (keycodes[KF0 - KFIRST + i], KF0 + i);
    }

  /*
   * Add the home and arrow keys
   */
  if (KH && *KH)
    adddict (KH, (KEY) KHOME);
  else
    adddict (keycodes[KHOME - KFIRST], KHOME);
  if (KU && *KU)
    {
#if DEBUG
      fprintf (stderr, "KU = %s\n", KU);
#endif
      adddict (KU, (KEY) KUP);
    }
  else
    adddict (keycodes[KUP - KFIRST], KUP);
  if (KD && *KD)
    adddict (KD, (KEY) KDOWN);
  else
    adddict (keycodes[KDOWN - KFIRST], KDOWN);
  if (KL && *KL)
    adddict (KL, (KEY) KLEFT);
  else
    adddict (keycodes[KLEFT - KFIRST], KLEFT);
  if (KR && *KR)
    adddict (KR, (KEY) KRIGHT);
  else
    adddict (keycodes[KRIGHT - KFIRST], KRIGHT);

#endif

  /*
   * Bind things to the movement keys
   */
  keydup (KUP, "back-line");
  keydup (KDOWN, "forw-line");
  keydup (KLEFT, "back-char");
  keydup (KRIGHT, "forw-char");
  keydup (KCTRL | KLEFT, "back-word");
  keydup (KCTRL | KRIGHT, "forw-word");
  keydup (KPGUP, "back-page");
  keydup (KPGDN, "forw-page");
  keydup (KCTRL | KPGUP, "up-window");
  keydup (KCTRL | KPGDN, "down-window");
  keydup (KHOME, "goto-bol");
  keydup (KEND, "goto-eol");
  keydup (KCTRL | KHOME, "goto-bob");
  keydup (KCTRL | KEND, "goto-eob");
  keydup (KINS, "set-overstrike");
  keydup (KDEL, "forw-del-char");
  keydup (KF0, "help");
  keydup (KF1, "file-save");
  keydup (KF2, "file-visit");
  keydup (KF3, "quit");
  keydup (KF4, "undo");
#if USE_RUBY
  keydup (KF6, "ruby-string");
#else
  keydup (KF5, "display-buffers");
#endif
  keydup (KF6, "forw-window");
  keydup (KF7, "forw-buffer");
  keydup (KF8, "search-again");
  keydup (KF9, "only-window");
  keydup (KSF0, "find-cscope");
  keydup (KSF1, "next-cscope");
  keydup (KMOUSE, "mouse-event");
  /*
   * These bindings sort of go with the termcap I use for my vt220
   * clone, which is why they're #ifdef'd.  I don't really use
   * them, but it gives you an example of what to do...
   */
#ifdef	MPK
  keydup ((KEY) KF0, "describe-key-briefly");	/* Help         */
  keydup ((KEY) KF1, "execute-extended-command");	/* Do           */
  keydup ((KEY) KF2, "search-forward");	/* Find         */
  keydup ((KEY) KF3, "yank");	/* Insert here  */
  keydup ((KEY) KF4, "kill-region");	/* Remove       */
  keydup ((KEY) KF5, "set-mark-command");	/* Select       */
  keydup ((KEY) KF6, "scroll-down");	/* Prev Screen  */
  keydup ((KEY) KF7, "scroll-up");	/* Next Screen  */

  /* Don't expect these to make much sense, I'm just filling in   */
  /* the keymap B-]                                               */
  keydup ((KEY) KF10, "suspend-emacs");	/* PF1          */
  keydup ((KEY) KF11, "query-replace");	/* PF2          */
  keydup ((KEY) KF12, "call-last-kbd-macro");	/* PF3          */
  keydup ((KEY) KF13, "save-buffers-kill-emacs");	/* PF4          */
#endif /* MPK */
#endif /* XKEYS */

#if DEBUG
  tprint (keywords, 0);
#endif

}

#ifdef	XKEYS
/*
 * Clean up the keyboard -- called by tttidy()
 */
void
ttykeymaptidy (void)
{
  tdelete (keywords);		/* get rid of parse tree        */
  free (sentinel);		/* remove sentinel value        */
  if (KE && *KE)
    putpad (KE);		/* turn off keypad              */
}

/*
 * * * * * * * * Dictionary management * * * * * * * * *
 */

/*
 * Add a key string to the dictionary.
 */

static void
adddict (char *kstr,
         KEY kcode)
{
  keywords = tinsert (kstr, kcode, keywords);
}

/*
 * Initialize the parse tree by creating the sentinel value
 */

static void
tinit (void)
{
  keywords = sentinel = talloc ();
  sentinel->type = SENTINEL;
  sentinel->value = (KEY) - 1;
  sentinel->sibling = sentinel->child = sentinel;	/* set up a loop */
}

/*
 * Deallocate all the space used by the trie --
 * Tell all the siblings to deallocate space, then
 * all the children.
 */

static void
tdelete (TRIE t)
{
  if (t->type != SENTINEL)
    {
      tdelete (t->sibling);
      tdelete (t->child);
      free (t);
    }
}

/*
 * Insert a dictionary key string and a value into the dictionary,
 * returning as the value the first sibling in the current sublevel,
 * which may have been changed by an insertion into the list of siblings.
 */

static TRIE
tinsert (const char *kstring, KEY kcode, TRIE first)
{
  register TRIE match;
  register TRIE p;
  KEY key;

/* fprintf(stderr, "tinsert: string '%s', code %x\n", kstring, kcode); */
  if ((key = *kstring & 0xff) == 0xff)	/* special kludge to allow FF */
    key = 0;			/* in string to mean null character */
  if (!*kstring)
    {				/* base case -- return a value node */
      p = talloc ();
      p->type = VALUE;
      p->value = kcode;
      p->sibling = p->child = sentinel;
      return (p);
    }
  /* recursive case -- insert rest of string in trie */

  /* search for sibling that matches the current character */
  match = NULL;
  for (p = first; p->type == NODE; p = p->sibling)
    if (p->value == key)
      {
	match = p;
	break;
      }

  if (match == NULL)
    {				/* if not, add it to beginning of the list */
      match = talloc ();
      match->type = NODE;
      match->value = key;
      match->sibling = first;
      match->child = sentinel;
      first = match;
    }
  /* add rest of string to this child's subtrie */
  match->child = tinsert (kstring + 1, kcode, match->child);
  return (first);
}

#if DEBUG
void
tprint (TRIE p, int level)
{
  int i;
  char buf[3];
  int c;

  while (p->type != SENTINEL)
    {
      for (i = 0; i < level * 2; i++)
	fputc (' ', stderr);
      c = p->value;
      if (p->type == VALUE)
	fprintf (stderr, "value 0x%x\n", c);
      else
	{
	  if (c < 0x20)
	    {
	      buf[0] = '^';
	      buf[1] = c + '@';
	      buf[2] = '\0';
	    }
	  else if (c >= 0x7f)
	    {
	      buf[0] = '?';
	      buf[1] = '?';
	      buf[2] = '\0';
	    }
	  else
	    {
	      buf[0] = c;
	      buf[1] = '\0';
	    }
	  fprintf (stderr, "%s (0x%x)\n", buf, c);
	  tprint (p->child, level + 1);
	}
      p = p->sibling;
    }
}
#endif

/*
 * Allocate a trie node
 */
static TRIE
talloc (void)
{
  TRIE t;

  if ((t = (TRIE) malloc (sizeof (TRIENODE))) == NULL)
    panic ("talloc: can't allocate trie node!");
  return (t);
}
#endif
