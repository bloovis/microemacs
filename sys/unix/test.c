#include <signal.h>

int
main ()
{
  register int pid;
  register int wpid;
  register int (*oqsig) ();
  oqsig = signal (SIGQUIT, SIG_IGN);
}
