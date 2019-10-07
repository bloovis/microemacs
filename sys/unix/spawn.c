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
#if defined(__linux__) || defined(CYGWIN) || defined(__FreeBSD__) || defined(__APPLE__)
#include	<sys/types.h>
#include	<sys/wait.h>
#include	<fcntl.h>
#endif

extern struct termios oldtty;
extern struct termios newtty;

static int saw_sigpipe;


/*
 * Get the name of the parent process, and return a pointer to a
 * static buffer containing the first 31 characters of the name,
 * or NULL if we can't get the parent name for some reason.
 * We do this by getting the PID of the parent, then reading
 * the file /proc/PID/comm.
 */
static const char *
getparent (void)
{
  static char parent[32];
  pid_t ppid;
  char procfile[32];
  int fd;
  ssize_t actual;
  char *p;

  if (parent[0] != '\0')
    return parent;
  ppid = getppid ();
  snprintf (procfile, sizeof (procfile), "/proc/%d/comm", ppid);
  fd = open (procfile, O_RDONLY);
  if (fd < 0)
    return NULL;
  actual = read (fd, parent, sizeof (parent) - 1);
  if (actual == -1)
    return NULL;
  parent[actual] = '\0';

  /* Zap the newline terminator.
   */
  for (p = parent; p < &parent[actual]; p++)
    if (*p == '\n')
      *p = '\0';
  return parent;
}

/*
 * Get the shell name from the SHELL or shell environment variable.
 * If no such variable exists, assume /bin/sh.
 */
static const char *
getshell (void)
{
  static const char *shellp = NULL;	/* Saved "SHELL" name.          */

  if (shellp == NULL)
    {
      shellp = getenv ("SHELL");
      if (shellp == NULL)
	shellp = getenv ("shell");
      if (shellp == NULL)
	shellp = "/bin/sh";		/* Safer.               */
    }
  return shellp;
}

/*
 * Determine if the parent process is a shell that supports
 * job control signals.
 */
static int
jcok (void)
{
  const char *parent;
  static const char *jcshells[3] = {"bash", "csh", "tcsh"};
  int i, n;

  /* Compare the parent process name against a list of known shells
   * that support job control.
   */
  parent = getparent ();
  if (parent == NULL)
    return FALSE;
  n = sizeof (jcshells) / sizeof (jcshells[0]);
  for (i = 0; i < n; i++)
    if (strcmp (parent, jcshells[i]) == 0)
      return TRUE;
  return FALSE;
}

/*
 * Spawn the program, pass it some arguments.  If the program is NULL,
 * run the shell instead.
 */
int
spawn (char *program, const char *args[])
{
  register int pid;
  register int wpid;
  register void (*oqsig) ();
  register void (*oisig) ();
  int status;
  int jobcontrol = FALSE;
  const char *shellp = NULL;

  /* If program is NULL, run the shell.  Determine what shell
   * we are using, and whether we can use job control to return
   * to the shell temporarily.
   */
  if (program == NULL)
    {
      shellp = getshell ();
      jobcontrol = jcok ();
    }

  ttcolor (CTEXT);
  ttnowindow ();

  if (jobcontrol)
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
  if (ttold () == FALSE)
    {
      eprintf ("Unable to restore terminal state");
      return (FALSE);
    }
  if (jobcontrol)			/* C shell or bash */
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
	    execvp (program, (char * const *) args);
	  _exit (0);		/* Should do better!    */
	}
      while ((wpid = wait (&status)) >= 0 && wpid != pid)
	;
      signal (SIGQUIT, oqsig);
      signal (SIGINT, oisig);
    }
  if (ttnew () == FALSE)
    {
      eprintf ("Unable to reinitialize terminal state");
      return (FALSE);
    }
  erefresh (FALSE, 1, 0);	/* Force repaint.       */
  return (TRUE);
}

/*
 * This code does a one of 2 different
 * things, depending on what version of the shell
 * you are using. If you are using the C shell or Bash, which
 * implies that you are using job control, then MicroEMACS
 * moves the cursor to a nice place and sends itself a
 * stop signal. If you are using another shell it runs
 * a subshell using fork/exec.  Bound to "C-C".
 */
int
spawncli (int f, int n, int k)
{
  return (spawn (NULL, NULL));
}

/*
 * Signal handler for SIGPIPE, which can occur if cscope terminates
 * abnormally (e.g. if the current directory has no source files).
 */
static void
sigpipe_handler (int signum)
{
   saw_sigpipe = 1;
}

/*
 * Open a two-way pipe to the specified program, store the
 * input FILE pointer to *infile, store the output FILE pointer
 * to *outfile, and return TRUE if success.
 */
int
openpipe (const char *program, const char *args[],
          FILE **infile, FILE **outfile)
{
  int in_pipe[2];
  int out_pipe[2];

  saw_sigpipe = 0;
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
       * standard output to the output pipe.
       */
      dup2 (out_pipe[0], 0);
      dup2 (in_pipe[1], 1);

      /* Close unneeded pipe handles. */
      close (out_pipe[1]);
      close (in_pipe[0]);

      /* Redirect stderr to /dev/null. */
      devnull = open ("/dev/null", O_WRONLY);
      if (devnull >= 0)
	dup2 (devnull, 2);

      /* Execute the program. */
      execvp (program, (char **)args);
    }
  else
    {
      /* We're the parent.  Connect the two ends of pipe to line-buffered
       * FILEs.
       */
      *infile = fdopen (in_pipe[0], "r");
      *outfile = fdopen (out_pipe[1], "w");
      if (*infile == NULL || *outfile == NULL)
	{
#if TEST
	  perror ("creating FILEs from pipes");
#endif
	  return FALSE;
	}

      setvbuf (*infile, (char *)NULL, _IOLBF, 0);
      setvbuf (*outfile, (char *)NULL, _IOLBF, 0);

      /* Close unneeded pipe handles. */
      close (out_pipe[0]);
      close (in_pipe[1]);

      /* Set up a signal handler for SIGPIPE to prevent ourselves
       * from exiting should the child process bomb for some reason.
       */
      signal (SIGPIPE, sigpipe_handler);
    }

  return TRUE;
}
