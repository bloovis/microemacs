/*
 * Name:	MicroEMACS
 *		Spawn CLI; stop if C shell.
 * Version:	29
 * Last edit:	10-Feb-86
 * By:		rex::conroy
 *		decvax!decwrl!dec-rhea!dec-rex!conroy
 *
 * Spawn. New version, which
 * interracts with the job control stuff
 * in the 4.X BSD C shell.
 */
#include	"def.h"

#ifdef __hpux
/* Need this kludge to get termios.h to define POSIX functions */
#define _INCLUDE_POSIX_SOURCE
#endif

#include	<termios.h>
#include	<signal.h>
#include	<stdlib.h>
#include	<unistd.h>
#ifdef __linux__
#include	<sys/types.h>
#include	<sys/wait.h>
#endif

char *shellp = NULL;		/* Saved "SHELL" name.          */

extern struct termios oldtty;
extern struct termios newtty;

/*
 * Spawn the program, pass it some arguments.  If the program is NULL,
 * run the shell instead.
 */
int
spawn (char *program, char *args[])
{
  register int pid;
  register int wpid;
  register void (*oqsig) ();
  register void (*oisig) ();
  int status;
  int cshell = 0;
  char *p, *basename;

  if (program == NULL)
    {				/* Run the shell        */
      if (shellp == NULL)
	{
	  shellp = getenv ("SHELL");
	  if (shellp == NULL)
	    shellp = getenv ("shell");
	  if (shellp == NULL)
	    shellp = "/bin/sh";	/* Safer.               */
	}
    }

  ttcolor (CTEXT);
  ttnowindow ();

  /* Skip past the directory portion of the shell name,
   * so we can see if this is the C shell (or tcsh).
   */
  if (program == NULL)
    {
      for (p = basename = shellp; *p != '\0'; p++)
	if (*p == '/')
	  basename = p + 1;
      /*      printf("\n\nbase name is %s\n\n", basename); */
      cshell = strcmp (basename, "csh") == 0
        || strcmp (basename, "bash") == 0
	|| strcmp (basename, "tcsh") == 0;
    }
  if (cshell)
    {
      if (epresf != FALSE)
	{
	  ttmove (nrow - 1, 0);
	  tteeol ();
	  epresf = FALSE;
	}			/* Csh types a "\n"     */
      ttmove (nrow - 2, 0);	/* before "Stopped".    */
    }
  else
    {
      ttmove (nrow - 1, 0);
      if (epresf != FALSE)
	{
	  tteeol ();
	  epresf = FALSE;
	}
    }
  ttflush ();
  if (tcsetattr (0, TCSANOW, &oldtty) < 0)
    {
      eprintf ("IOCTL #1 to terminal failed");
      return (FALSE);
    }
  if (cshell)			/* C shell.             */
    kill (0, SIGTSTP);
  else
    {				/* Bourne shell.        */
      oqsig = signal (SIGQUIT, SIG_IGN);
      oisig = signal (SIGINT, SIG_IGN);
      if ((pid = fork ()) < 0)
	{
	  signal (SIGQUIT, oqsig);
	  signal (SIGINT, oisig);
	  eprintf ("Failed to create process");
	  return (FALSE);
	}
      if (pid == 0)
	{
	  if (program == NULL)
	    execl (shellp, "sh", "-i", NULL);
	  else
	    execvp (program, args);
	  _exit (0);		/* Should do better!    */
	}
      while ((wpid = wait (&status)) >= 0 && wpid != pid)
	;
      signal (SIGQUIT, oqsig);
      signal (SIGINT, oisig);
    }
  sgarbf = TRUE;		/* Force repaint.       */
  if (tcsetattr (0, TCSANOW, &newtty) < 0)
    {
      eprintf ("IOCTL #2 to terminal failed");
      return (FALSE);
    }
  return (TRUE);
}

/*
 * This code does a one of 2 different
 * things, depending on what version of the shell
 * you are using. If you are using the C shell, which
 * implies that you are using job control, then MicroEMACS
 * moves the cursor to a nice place and sends itself a
 * stop signal. If you are using the Bourne shell it runs
 * a subshell using fork/exec.  Bound to "C-C".
 */
int
spawncli (int f, int n, int k)
{
  return (spawn (NULL, NULL));
}

/*
 * Run the spell checkers.
 */
int
spellcheck (f, n, k)
{
  char *args[3];

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
