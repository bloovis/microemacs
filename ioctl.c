#include <stdio.h>

int
ioctl (int handle, int num, void *ptr)
{
  printf ("ioctl(%d, %d, %p)\n");
}
