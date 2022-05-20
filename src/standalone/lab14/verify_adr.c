#include <stdlib.h>
#include "pagedir.h"
#include "thread.h"
#include <stdio.h>

/* verfy_*_length are intended to be used in a system call that accept
 * parameters containing suspicious (user mode) adresses. The
 * operating system (executing the system call in kernel mode) must not
 * be fooled into using (reading or writing) addresses not available
 * to the user mode process performing the system call.
 *
 * In pagedir.h you can find some supporting functions that will help
 * you dermining if a logic address can be translated into a physical
 * addrerss using the process pagetable. A single translation is
 * costly. Work out a way to perform as few translations as
 * possible.
 *
 * Recommended compilation command:
 *
 *  gcc -Wall -Wextra -std=gnu99 -pedantic -m32 -g pagedir.o verify_adr.c
 */

/* Verify all addresses from and including 'start' up to but excluding
 * (start+length). */
bool verify_fix_length(void* start, unsigned length)
{
  /*
  Get the last address with the help of length.
  Get the page for the last address.
  Find out which page the first address resides in.
  Check if the current page is valid.
  Check if the first and last page are the same page.
  If invalid return false.
  Move forward to the next page and check if current page is valid until lastpage.
  Return true.
  */

  int last_page_number = pg_no(start+length-1);
  const void* walker = start;
  
  // Return true if the first_page_number == last_page_number and the
  // page is valid.
  if( (int) pg_no(walker) == last_page_number 
      && pagedir_get_page(thread_current()->pagedir, walker) != NULL)
  {
    return true;
  }

  do
  {
    if( pagedir_get_page(thread_current()->pagedir, walker) == NULL )
    {
      return false;
    }
    walker = walker + PGSIZE;
  } while ((int) pg_no(walker) <= last_page_number);
  
  return true;
}

/* Verify all addresses from and including 'start' up to and including
 * the address first containg a null-character ('\0'). (The way
 * C-strings are stored.)
 */
bool verify_variable_length(char* start)
{
  const void* walker = start;
  // Statement run once every page.
  while ( pagedir_get_page(thread_current()->pagedir, walker) == NULL )
  {
    // Check each address in page if its the end \0
    for(int i=0; i<PGSIZE; i++)
    {
      if(is_end_of_string((char*)walker))
      {
        return true;
      }
      walker++;
    }
  } 
  return false;

}

/* Definition of test cases. */
struct test_case_t
{
  void* start;
  unsigned length;
};

#define TEST_CASE_COUNT 6

const struct test_case_t test_case[TEST_CASE_COUNT] =
{
  {(void*)100, 100}, /* one full page */
  {(void*)199, 102},
  {(void*)101, 98},
  {(void*)250, 190},
  {(void*)250, 200},
  {(void*)250, 210}
};

/* This main program will evalutate your solution. */
int main(int argc, char* argv[])
{
  int i;
  bool result;

  if ( argc == 2 )
  {
    simulator_set_pagefault_time( atoi(argv[1]) );
  }
  thread_init();

  /* Test the algorithm with a given intervall (a buffer). */
  for (i = 0; i < TEST_CASE_COUNT; ++i)
  {
    start_evaluate_algorithm(test_case[i].start, test_case[i].length);
    result = verify_fix_length(test_case[i].start, test_case[i].length);
    evaluate(result);
    end_evaluate_algorithm();
  }

  /* Test the algorithm with a C-string (start address with
   * terminating null-character).
   */
  for (i = 0; i < TEST_CASE_COUNT; ++i)
  {
    start_evaluate_algorithm(test_case[i].start, test_case[i].length);
    result = verify_variable_length(test_case[i].start);
    evaluate(result);
    end_evaluate_algorithm();
  }
  return 0;
}
