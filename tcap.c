#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if 0
#include	<sys/ioctl.h>
/* #include	<sys/filio.h> */
/* #include	<termios.h> */
#include <sgtty.h>
#else
#include <curses.h>
#include <term.h>
#endif

#ifdef  TIOCGWINSZ
/* #error    blorch; */
#endif

#define TCAPSLEN 1024

char tcapbuf[TCAPSLEN];
static char tcbuf[1024];
char *tgetstr ();

void
printesc (p)
     char *p;
{
  char c;
  while ((c = *p++) != '\0')
    if (c < ' ')
      printf ("^%c", c + '@');
    else
      printf ("%c", c);
}

void
main (argc, argv)
     int argc;
     char *argv[];
{
  char *tv_stype, *p, *t;
  int i;

/*	ttopen(); */
  if ((tv_stype = getenv ("TERM")) == NULL)	/* Don't want VAX C getenv() */
    {
      printf ("Environment variable TERM not defined!");
      return;
    }

  if ((tgetent (tcbuf, tv_stype)) != 1)
    {
      printf ("Unknown terminal type %s\n", tv_stype);
      return;
    }

  p = tcapbuf;
  for (i = 1; i < argc; i++)
    {
      t = tgetstr (argv[i], &p);
      if (t == NULL)
	printf ("%s not defined for this terminal\n", argv[i]);
      else
	{
	  printf ("%s: ", argv[i]);
	  printesc (t);
	  printf ("\n");
	}
      printf ("%s (numeric): %d\n", argv[i], tgetnum (argv[i]));
    }
}
