#include <stdio.h>
#include "regexp.h"

void
main ()
{
  regexp *r = regcomp ("o.*ing");
  int i = regexec (r, "This is a Gosling display algorithm");
  printf ("Result is %d\n", i);
}

void
regerror (char *s)
{
  printf ("regerror: %s\n", s);
}
