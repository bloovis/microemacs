/*
    Copyright (C) 2018 Mark Alexander

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

#include	"def.h"

#define CCHR(x)		((x)-'@')

/*
 * Local variables.
 */
static FILE *ispell_input;
static FILE *ispell_output;

static char word[NPAT];		/* word to check for spelling */
static char repl[NPAT];		/* string to replace word */
static char buf[256];		/* line buffer for input from ispell */
static char *guesses[10];	/* guesses returned by ispell */
static int nguesses;		/* number of guesses */
static LINE *clp;		/* saved line pointer           */
static int cbo;			/* offset into the saved line   */
static int nrepl;		/* number of replacements performed */

/*
 * Return TRUE if the character at dot is a letter or an
 * apostrophe (as in "doesn't", for example).
 */
static int
inalpha (void)
{
  return inwordpos (curwp->w_dot.p, curwp->w_dot.o, TRUE);
}

/*
 * Open a two-way pipe to the ispell program.
 */
static int
open_ispell (void)
{
  const char *args[3];

  if (ispell_input != NULL)
    return TRUE;

  args[0] = "ispell";
  args[1] = "-a";
  args[2] = NULL;
  if (openpipe ("ispell", args, &ispell_input, &ispell_output) == FALSE)
    return FALSE;

  /* Read the identification message from ispell.
   */
  if (fgets (buf, sizeof (buf), ispell_input) == NULL)
    return FALSE;
  else
    return TRUE;
}

/*
 * Replace the string 'word' with 'repl' at dot.  The dot
 * must be past the end of the old word, because lreplace expects
 * dot to be after, not before, the string being replaced.
 */
static int
replace (void)
{
  int oldlen = uslen ((const uchar *) word);
  int status;

  curwp->w_savep = clp;
  status = lreplace (oldlen, repl, FALSE);
  ++nrepl;
  clp = curwp->w_savep;
  return status;
}

/*
 * Remove plus signs from a string, and remove suffixes preceded
 * with minus signs.  This is useful for cleaning up ispell
 * suggestions about suffixes.  For example, it will suggest
 * "instinctual+ly" for "instinctually", and will suggest
 * "instinctual-ism" for "instinctualism".
 */
static void
cleanupguess (char *s)
{
  char *d = s;
  char c;

  while ((c = *s) != '\0')
    {
      if (c == '-')
	{
	  ++s;
	  while ((c = *s) != '\0' && c != '+')
	    ++s;
	}
      else if (c == '+')
	++s;
      else
	{
	  *d = *s;
	  ++d;
	  ++s;
	}
    }
  *d = '\0';
}

/*
 * Prompt for a replacement word.  If the user specifies a replacement
 * successfully, copy the replacement to repl and return TRUE.
 * Otherwise return FALSE, or ABORT if the user enters Q or Control-G.
 */
static int
getrepl (const char *prompt)
{
  int c;
  int status;
  int done;

  eprintf (prompt);
  done = FALSE;
  while (!done)
    {
      if (!inprof)
	update ();		/* show current position	*/
      c = getinp ();
      switch (c)
        {
	case CCHR ('G'):
	  /* Abort the replacement.
	   */
	  status = ctrlg (FALSE, 0, KRANDOM);
	  done = TRUE;
	  break;
	case 'q':
	case 'Q':
	  status = ABORT;
	  done = TRUE;
	  break;
	case ' ':
	  /* Ignore the word and don't replace it.
	   */
	  status = FALSE;
	  done = TRUE;
	  break;
	case 'r':
	case 'R':
	  /* User doesn't like any of the suggestions. Prompt
	   * for a replacement string.
	   */
	  status = ereply ("Replace with: ", repl, sizeof (repl));
	  done = TRUE;
	  break;
	case 'a':
	case 'A':
	  /* Tell ispell to accept the word in the future, and
	   * leave it unchanged.
	   */
	  fputc ('@', ispell_output);
	  fputs (word, ispell_output);
	  fputc ('\n', ispell_output);
	  fflush (ispell_output);
	  status = FALSE;
	  done = TRUE;
	  break;
	default:
	  /* A digit from 0 to 9 means use ispell's nth
	   * suggested replacement.
	   */
	  if (c >= '0' && c <= '9')
	    {
	      int n = c - '0';
	      if (n < nguesses)
		{
		  cleanupguess (guesses[n]);
		  strncpy (repl, guesses[n], sizeof (repl));
		  status = TRUE;
		  done = TRUE;
		}
	    }
	  break;
	}
    }
  eerase ();
  return status;
}

/*
 * Ask ispell to check a word, and if it is not correct,
 * prompt the user with the suggestions from ispell.
 * If info is TRUE, print status messages on the echo line
 * about the replacement that was performed, if any.
 */
static int
ask_ispell (int info)
{
  char *s;
  char prompt[256];
  int status;
  int chars;
  int i;
  const char *fmt;

  fputs (word, ispell_output);
  fputc ('\n', ispell_output);
  fflush (ispell_output);
  status = FALSE;
  while (TRUE)
    {
      if (fgets (buf, sizeof (buf), ispell_input) == NULL)
	return FALSE;

      /* ispell outputs a blank line after the line containing
       * spelling suggestions, so we need to read and discard it.
       */
      if (buf[0] == '\n' || buf[0] == '\0')
	break;

      /* Zap the terminating line feed.
       */
      s = strchr (buf, '\n');
      if (s != NULL)
	*s = '\0';

      /* Parse the ispell response.
       */
      switch (buf[0])
        {
	case '*':
	  if (info)
	    eprintf ("%s is spelled correctly", word);
	  status = TRUE;
	  break;
	case '+':
	  if (info)
	    eprintf ("%s found via root %s", word, &buf[2]);
	  status = TRUE;
	  break;
	case '#':
	case '&':
	case '?':
	  /* Ignore the first part of the response, which gives
	   * the original word, an offset, and possibly the
	   * number of guesses.
	   */
	  if (buf[0] == '#')
	    fmt = " %*s %*d%n";
	  else
	    fmt = " %*s %*d %*d: %n";
	  s = &buf[1];
	  if (sscanf (s, fmt, &chars) != 0)
	    break;
	  s += chars;

	  /* The rest of the response contains guesses separated by commas.
	   * Break up the guesses into individual words.
	   */
	  nguesses = 0;
	  for (i = 0; i < 10 && *s != '\0'; i++)
	    {
	      guesses[i] = s;
	      nguesses = i + 1;
	      while (*s != ',' && *s != '\0')
		++s;
	      if (*s == '\0')
		break;
	      *s++ = '\0';
	      while (*s != '\0' && *s != ' ')
		++s;
	      if (*s == '\0')
		break;
	      ++s;
	    }

	  /* Construct a prompt string that includes as many guesses
	   * as will fit on one line.
	   */
	  strcpy (prompt, word);
	  strcat (prompt, ": SPC=ignore,A=accept,R=replace,Q=quit");
	  for (i = 0; i < nguesses; i++)
	    {
	      char n[2];

	      /* If the prompt exceeds the window width, truncate
	       * the number of guesses.  This is a horrible hack
	       * but I can't see a good way to display a long list
	       * of guesses on a single line.
	       */
	      if (strlen (prompt) + strlen (guesses[i]) + 3 > ncol)
		{
		  nguesses = i;
		  break;
		}
	      n[0] = i + '0';
	      n[1] = '\0';
	      strcat (prompt, ",");
	      strcat (prompt, n);
	      strcat (prompt, "=");
	      strcat (prompt, guesses[i]);
	    }

	  /* Prompt for a replacement word, and do the replacement
	   * if the user specifies one.
	   */
	  if ((status = getrepl (prompt)) == TRUE)
	    status = replace ();
	  if (info)
	    {
	      if (status == TRUE)
		eprintf ("%s replaced with %s", word, repl);
	      else
		eprintf ("No replacement done");
	    }
	  break;
	default:
	  eprintf ("Unrecognized ispell response: %s", buf);
	  break;
	}
    }
  return status;
}

/*
 * Check the word under the cursor for spelling,
 * and prompt the user for a replacement.
 */
int
checkword (REGION *r)
{
  int status;

  if (open_ispell () == FALSE)
    {
      eprintf ("Unable to open a pipe to ispell");
      return FALSE;
    }

  /* Get the word under the cursor (if any).  Then prompt
   * for a word to spell-check, using the cursor word as the default.
   */
  word[0] = '\0';
  getcursorword (word, sizeof (word), TRUE);
  if (word[0] == '\0')
    {
      eprintf("No word under cursor");
      return FALSE;
    }

  /* Move past the word, because lreplace expects that.
   */
  while (inalpha ())
    {
      if ((status = forwchar (TRUE, 1, KRANDOM)) != TRUE)
	return status;

      /* If we're keeping track of a region size, decrement
       * the size for each character we move past.
       */
      if (r != NULL)
	r->r_size--;
    }

  /* Ask ispell for the correct spelling, and prompt user
   * for a replacement.
   */
  return ask_ispell (r == NULL);
}

/*
 * Run ispell on the word under the cursor, or
 * if there is no word there, prompt for the word.
 */
int
spellword (int f, int n, int k)
{
  return checkword (NULL);
}

/*
 * Run ispell on the current marked region.
 */
int
spellregion (int f, int n, int k)
{
  REGION r;
  int status;
  int len;

  if ((status = getregion (&r)) != TRUE)
    return status;

  /* Save the current location.
   */
  clp = curwp->w_dot.p;
  cbo = curwp->w_dot.o;

  /* Scan through the region looking for words, and spell-check
   * each found word.
   */
  curwp->w_dot = r.r_pos;
  nrepl = 0;
  while (r.r_size > 0)
    {
      /* Find start of word.
       */
      while (r.r_size > 0)
	{
	  if (inalpha ())
	    break;
	  if (forwchar (TRUE, 1, KRANDOM) != TRUE)
	    break;
	  r.r_size--;
	}

      /* If we're not in a word, we must have reached the end
       * of the region or buffer, so terminate the outer loop.
       */
      if (!inalpha ())
	break;

      /* Spell-check the current word.
       */
      if ((status = checkword (&r)) == ABORT)
	break;
    }
  eprintf ("%d replacement%s done.", nrepl, nrepl == 1 ? "" : "s");

  /* Restore the current location.
   */
  curwp->w_dot.p = clp;
  len = wllength (clp);
  if (cbo > len)
    curwp->w_dot.o = len;
  else
    curwp->w_dot.o = cbo;
  curwp->w_flag |= WFHARD;
  if (!inprof)
    update ();
  return TRUE;
}
