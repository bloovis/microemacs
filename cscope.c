#include "def.h"
#include <unistd.h>
#include <fcntl.h>

/* Uncomment this line to build test program. */
/* #define TEST 1 */

/*
 * Local variables.
 */
static FILE *cscope_input;
static FILE *cscope_output;

/*
 * Ignore the prompt characters from cscope.
 */
static void
ignore_prompt (void)
{
  if (fgetc (cscope_input) != '>' || fgetc (cscope_input) != '>' ||
      fgetc (cscope_input) != ' ')
    {
#if TEST
      printf ("bad prompt from cscope!\n");
#endif
    }
}

/*
 * Open a pipe to the cscope program, return TRUE if success.
 */
static int
open_cscope (void)
{
  int in_pipe[2];
  int out_pipe[2];

  if (pipe (in_pipe) != 0)
    {
#if TEST
      perror ("creating input pipe");
#endif
      return FALSE;
    }
  if (pipe (out_pipe) != 0)
    {
#if TEST
      perror ("creating input pipe");
#endif
      return FALSE;
    }

  if (fork () == 0)
    {
      int devnull;

      /* We're the child.  Redirect standard input to the input pipe, and
         standard output to the output pipe. */
      dup2 (out_pipe[0], 0);
      dup2 (in_pipe[1], 1);

      /* Close unneeded pipe handles. */
      close (out_pipe[1]);
      close (in_pipe[0]);

      /* Redirect stderr to /dev/null. */
      devnull = open ("/dev/null", O_WRONLY);
      if (devnull >= 0)
	dup2 (devnull, 2);

      /* Execute the cscope program. */
      execlp ("cscope", "cscope", "-l", "-k", NULL);
    }
  else
    {
      /* We're the parent.  Connect the two ends of pipe to line-buffered
         FILEs. */
      cscope_input = fdopen (in_pipe[0], "r");
      cscope_output = fdopen (out_pipe[1], "w");
      if (cscope_input == NULL || cscope_output == NULL)
	{
#if TEST
	  perror ("creating FILEs from pipes");
#endif
	  return FALSE;
	}

      setvbuf (cscope_input, (char *)NULL, _IOLBF, 0);
      setvbuf (cscope_output, (char *)NULL, _IOLBF, 0);

      /* Close unneeded pipe handles. */
      close (out_pipe[0]);
      close (in_pipe[1]);
    }

  return TRUE;
}

/*
 * Initiate a search to cscope, return the number of matches.  The
 * 'searchfield' parameter says which field of of the cscope entry screen
 * on which to enter the string ('0' is "Find this C symbol", '6' is
 * "Find this egrep pattern", etc.).
 */
static int
cscope_search (char search_field, const char *search_string)
{
  int nlines, result;
  char buf[64];

  /* Ignore the ">> " prompt, which should be waiting for us already. */
  ignore_prompt ();

  /* Issue the command Nsearchstring, where N is the field number.
   */
  fputc (search_field, cscope_output);
  fputs (search_string, cscope_output);
  fputc ('\n', cscope_output);
  fflush (cscope_output);
#if TEST
  printf ("sent command '0%s'\n", search_string);
#endif

  /* Read the first line, which is of the format 'cscope: n lines',
     and parse the 'n'. */
  if (fgets (buf, sizeof (buf), cscope_input) == NULL)
    return 0;
#if TEST
  printf ("result of query: '%s'\n", buf);
#endif
  result = sscanf (buf, "cscope: %d lines", &nlines);
  return result == 1 ? nlines : 0;
}

/*
 * Read the next match from the previously initiated search.
 * Return the file to 'filename', the name of the function containing
 * the string to 'where', and the line number to 'line_number'.
 * FIXME: we should do length-checking on the buffers.
 */
static void
next_match (char *filename, char *where, int *line_number)
{
  char buf[1024];
  int ret;

  if (fgets (buf, sizeof (buf), cscope_input) == NULL)
    return;
#if TEST
  printf ("read line: '%s'\n", buf);
#endif
  ret = sscanf (buf, "%s %s %d", filename, where, line_number);
#if TEST
  printf ("sscanf returned %d\n", ret);
#endif
}

/*
 * Issue a cscope search in the specified entry field for the string,
 * read back the resulting matches, and store them in the tags list.
 */
static int
prepcscope (char field, const char *string)
{
  int		n;
  char		filename[1024];
  char		where[1024];
  int		line;
  tagfile *	f;
  int		exact;

  /* Open a pipe to cscope if not already done.
   */
  if (cscope_input == NULL)
    if (open_cscope () == FALSE)
      {
	eprintf ("Unable to open a pipe to cscope");
	return FALSE;
      }

  /* Free up any existing tags from a previous search.
   */
  freetags (FALSE, 1, KRANDOM);

  /* Initiate a search to cscope, get back the number of matches.
   */
  n = cscope_search (field, string);

  /* Add each of the matches to the tag list.
   */
  while (n-- > 0)
    {
      /* Get the result of the next match.
       */
      next_match (filename, where, &line);

      /* Create a file entry for this file if not already in the list.
       */
      f = findtagfile (filename);
      if (f == NULL)
	{
	  eprintf ("Unable to create file structure");
	  return FALSE;
	}

      /* If the search string is the same as the name of he function where this
       * reference was found, this must be the definition of the function,
       * so put the tag at the head of the list instead of the end.
       */
      exact = strcmp (where, string) == 0;

      /* Add a tag entry to the list.
       */
      if (addtagref (string, f, line, 0L, exact) == NULL)
	{
	  eprintf ("Unable to create tag structure");
	  return FALSE;
	}
    }

  return TRUE;
}

/*
 * Prepare for scanning through the tags for the given C symbol.
 * This function is a callback called by searchtag (in tags.c)
 * just before it starts searching through the tag list.
 */
static int
prepref (const char *string)
{
  return prepcscope ('0', string);
}

/*
 * Search for a cscope reference.  All of the work involved in searching
 * the tag list is done in searchtag (tags.c), but the actual creation
 * of the tag list is done in prepref above.
 */
int
findcscope (f, n, k)
{
  return searchtag (f, n, prepref, "ref");
}

/*
 * Search for the next cscope reference.
 */
int
nextcscope (f, n, k)
{
  return searchtag (1, n, prepref, "ref");
}

/*
 * Prepare for scanning through the egrep searches for the given string.
 * This function is a callback called by searchtag (in tags.c)
 * just before it starts searching through the tag list.
 */
static int
prepgrep (const char *string)
{
  return prepcscope ('6', string);
}

/*
 * Search for an egrep reference.  All of the work involved in searching
 * the tag list is done in searchtag (tags.c), but the actual creation
 * of the tag list is done in prepgrep above.
 */
int
findgrep (f, n, k)
{
  return searchtag (f, n, prepgrep, "grep");
}

/*
 * test program
 */
#if TEST
int
main (int argc, char *argv[])
{
  int i;
  char filename[1024];
  char where[1024];
  int line;

  if (open_cscope () == FALSE)
    {
      printf ("unable to open pipe to cscope\n");
      return 1;
    }

  for (i = 1; i < argc; i++)
    {
      const char *search_string = argv[i];
      int n = cscope_search (search_string);
      printf ("%d matches for %s:\n", n, search_string);
      while (n-- > 0)
	{
	  next_match (filename, where, &line);
	  printf ("%s:%d in %s\n", filename, line, where);
	}
    }
  return 0;
}
#endif
