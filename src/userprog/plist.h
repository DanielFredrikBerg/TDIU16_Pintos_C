#ifndef _PLIST_H_
#define _PLIST_H_

typedef int key_t;
typedef struct process_info* value_t;
/* Place functions to handle a running process here (process list).
   
   plist.h : Your function declarations and documentation.
   plist.c : Your implementation.

   The following is strongly recommended:
*/
/*
   - A function that given process inforamtion (up to you to create)
     inserts this in a list of running processes and return an integer
     that can be used to find the information later on.
*/
key_t add_running_process(struct process_map* m, value_t pi_t);
/*
   - A function that given an integer (obtained from above function)
     FIND the process information in the list. Should return some
     failure code if no process matching the integer is in the list.
     Or, optionally, several functions to access any information of a
     particular process that you currently need.
*/
value_t find_running_process(struct process_map* m, key_t process_id);
/*
   - A function that given an integer REMOVE the process information
     from the list. Should only remove the information when no process
     or thread need it anymore, but must guarantee it is always
     removed EVENTUALLY.
*/
//TODO

/*- A function that print the entire content of the list in a nice,
     clean, readable format.*/
void print_process_list(struct process_map* m);


#endif
