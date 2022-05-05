#ifndef _PLIST_H_
#define _PLIST_H_
#include "flist.h"
#include "threads/synch.h"

typedef int key_t;
typedef struct p_info* value_p;

struct p_list
{
  value_p content[MAP_SIZE];
};

struct p_info
{
  int id;
  int status;
  bool is_alive;
  int parent_id;
  bool status_needed; // false if parent_alive == false || waited_on == true //Daniel
  struct semaphore sema;
};

/* Place functions to handle a running process here (process list).
   
   plist.h : Your function declarations and documentation.
   plist.c : Your implementation.

   The following is strongly recommended:
*/

void plist_init(struct p_list* m);

/*
   - A function that given process inforamtion (up to you to create)
     inserts this in a list of running processes and return an integer
     that can be used to find the information later on.
*/
key_t plist_add_process(struct p_list* m, value_p pi_t);
/*
   - A function that given an integer (obtained from above function)
     FIND the process information in the list. Should return some
     failure code if no process matching the integer is in the list.
     Or, optionally, several functions to access any information of a
     particular process that you currently need.
*/
value_p plist_find_process(struct p_list* m, key_t process_id);
/*
   - A function that given an integer REMOVE the process information
     from the list. Should only remove the information when no process
     or thread need it anymore, but must guarantee it is always
     removed EVENTUALLY.
*/
value_p plist_remove_process(struct p_list* m, key_t process_id);

/*- A function that print the entire content of the list in a nice,
     clean, readable format.*/
void plist_print(struct p_list* m);


/* Anropa exec för varje association i p_list.
 * aux skickas med som inparameter till funktionen *exec representerar.
 */
void plist_orphan_my_children(struct p_list* m, key_t parent_id);

#endif
