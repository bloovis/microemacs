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
static const char *guesses[10];	/* guesses returned by ispell */
static int nguesses;		/* number of guesses */

/*
 * Return TRUE if the character at dot
 * is a letter.
 */
static int
inalpha (void)
{
  if (curwp->w_dot.o == wllength (curwp->w_dot.p))
    return (FALSE);
  if (ISALPHA (wlgetc (curwp->w_dot.p, curwp->w_dot.o)) != FALSE)
    return (TRUE);
  return (FALSE);
}

/*
 * Run ispell on the entire buffer.
 */
int
spellcheck (int f, int n, int k)
{
  const char *args[3];

  /* Save the file if it's been changed.
   */
  if ((curbp->b_flag & BFCHG) != 0)
    if (filesave (f, n, KRANDOM) == FALSE)
      return FALSE;

  /* Run ispell on the file.
   */
  args[0] = "ispell";
  args[1] = curbp->b_fname;
  args[2] = NULL;
  if (spawn ("ispell", args) == FALSE)
    return FALSE;

  return readin (curbp->b_fname);
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

  /* Read the prompt string from ispell.
   */
  if (fgets (buf, sizeof (buf), ispell_input) == NULL)
    return FALSE;
  else
    return TRUE;
}

/*
 * Replace the string 'old' with 'new' at dot. We have
 * to move forward past the old word because lreplace expects
 * dot to be after, not before, the string being replaced.
 */
static int
replace (const char *old, const char *new)
{
  int oldlen = uslen ((const uchar *) old);
  int status;

  while (inalpha ())
    {
      if ((status = forwchar (TRUE, 1, KRANDOM)) != TRUE)
	return status;
    }
  return lreplace (oldlen, new, FALSE);
}

/*
 * Prompt for a replacement word.  If the user specifies a replacement
 * successfully, copy the replacement to repl and return TRUE.
 * Otherwise return FALSE
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
	  ctrlg (FALSE, 0, KRANDOM);
	case 'q':
	case 'Q':
	  status = FALSE;
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
		  strncpy (repl, guesses[n], sizeof (repl));
		  status = TRUE;
		  done = TRUE;
		}
	    }
	  break;
	}
    }
  if (status == TRUE)
    eprintf ("%s replaced with %s", word, repl);
  else
    eprintf ("No replacement done");
  return status;
}


/*
 * Ask ispell to check a word, and if it is not correct,
 * prompt the user with the suggestions from ispell.
 */
static int
ask_ispell (void)
{
  char *s;
  char prompt[256];
  int status;
  int chars;
  int i;

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
	  eprintf ("%s is spelled correctly", word);
	  status = TRUE;
	  break;
	case '#':
	  if ((status = ereply ("No matches. Replace with: ", repl, sizeof (repl))) != TRUE)
	    break;
	  status = replace (word, repl);
	  break;
	case '&':
	case '?':
	  s = &buf[1];
	  if (sscanf (s, "%*s %*d %*d: %n", &chars) != 0)
	    break;
	  s += chars;

	  /* Break up the guesses line into individual words.
	   */
	  nguesses = 0;
	  for (i = 0; i < 10; i++)
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
	  strcpy (prompt, "SPC=ignore,A=accept,R=replace,Q=quit");
	  for (i = 0; i < nguesses; i++)
	    {
	      char n[2];

	      n[0] = i + '0';
	      n[1] = '\0';
	      strcat (prompt, ",");
	      strcat (prompt, n);
	      strcat (prompt, "=");
	      strcat (prompt, guesses[i]);
	    }
	  if ((status = getrepl (prompt)) != TRUE)
	    break;
	  status = replace (word, repl);
	  break;
	default:
	  eprintf ("Unrecognized ispell response: %s", buf);
	  break;
	}
    }
  return status;
}

/*
 * Run ispell on the word under the cursor, or
 * if there is no word there, prompt for the word.
 */
int
spellword (int f, int n, int k)
{
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

  /* Ask ispell for the correct spelling, and prompt user
   * for a replacement.
   */
  return ask_ispell ();
}
