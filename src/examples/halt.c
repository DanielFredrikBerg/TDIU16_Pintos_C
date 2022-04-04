/* halt.c

   Simple program to test whether running a user program works.
 	
   Just invokes a system call that shuts down the OS. */

#include <syscall.h>
#include <stdio.h>

#define DBG(format, ...) printf(format "\n", ##__VA_ARGS__)

int
main (void)
{
  //DBG("# Before halt call row %d in %s", __LINE__, __FILE__);
  exit(3);
  //halt ();
  /* not reached */
}
