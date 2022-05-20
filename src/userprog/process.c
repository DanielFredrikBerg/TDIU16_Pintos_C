#include <debug.h>
#include <stdio.h>
#include <string.h>

#include "userprog/gdt.h"      /* SEL_* constants */
#include "userprog/process.h"
#include "userprog/load.h"
#include "userprog/pagedir.h"  /* pagedir_activate etc. */
#include "userprog/tss.h"      /* tss_update */
#include "filesys/file.h"
#include "threads/flags.h"     /* FLAG_* constants */
#include "threads/thread.h"
#include "threads/vaddr.h"     /* PHYS_BASE */
#include "threads/interrupt.h" /* if_ */
#include "threads/init.h"      /* power_off() */

/* Headers not yet used that you may need for various reasons. */
#include "threads/synch.h"
#include "threads/malloc.h"
#include "lib/kernel/list.h"

#include "userprog/flist.h"
#include "userprog/plist.h"

/* HACK defines code you must remove and implement in a proper way */
#define HACK
struct p_list process_map;
struct lock process_map_lock;


/* This function is called at boot time (threads/init.c) to initialize
 * the process subsystem. */
void process_init(void)
{
  // Borde denna vara här eller i thread_init? Varje trår borde ha sin egen process_map?
  plist_init(&process_map);
  lock_init(&process_map_lock);
}


/* This function is currently never called. As thread_exit does not
 * have an exit status parameter, this could be used to handle that
 * instead. Note however that all cleanup after a process must be done
 * in process_cleanup, and that process_cleanup are already called
 * from thread_exit - do not call cleanup twice! */
void process_exit(int status) 
{
  int id = thread_current()->id_in_process_map;
  // no need to lock map here because there is no risk for the prcoess to 
  // dissapear from the process map.
  // 1. process can be removed only after this function is called
  // 2. parent cant directly remove this from the process_map, it can only set the
  // status_needed to false.
  struct p_info *process_info = plist_find_process(&process_map, id);
  process_info->status = status;
  process_info->is_alive = false;
}


/* Print a list of all running processes. The list shall include all
 * relevant debug information in a clean, readable format. */
void process_print_list()
{
  plist_print(&process_map);
}


struct parameters_to_start_process
{
  bool is_success;
  char* command_line;
  struct semaphore sema;
  int parent_id;
  int child_id;
};

static void
start_process(struct parameters_to_start_process* parameters) NO_RETURN;

/* Starts a new proccess by creating a new thread to run it. The
   process is loaded from the file specified in the COMMAND_LINE and
   started with the arguments on the COMMAND_LINE. The new thread may
   be scheduled (and may even exit) before process_execute() returns.
   Returns the new process's thread id, or TID_ERROR if the thread
   cannot be created. */

  /* SCHEDULES function `start_process' to run (LATER) */
  // Do we get thread_id as soon as process has been started or only
  // after the thread has completed some of its work?

int
process_execute (const char *command_line) 
{
  char debug_name[64];
  int command_line_size = strlen(command_line) + 1;
  tid_t thread_id = -1;
  int  process_id = -1;

  /* LOCAL variable will cease existence when function return! */
  struct parameters_to_start_process arguments;

  sema_init(&(arguments.sema), 0);


  debug("#%s#%d: process_execute(\"%s\") ENTERED\n",
        thread_current()->name,
        thread_current()->tid,
        command_line);

  /* COPY command line out of parent process memory */
  arguments.command_line = malloc(command_line_size);
  strlcpy(arguments.command_line, command_line, command_line_size);

  // Current threads process id becomes parent id for child process here.
  arguments.parent_id = thread_current()->id_in_process_map;
  
  strlcpy_first_word (debug_name, command_line, 64);
  
       
  thread_id = thread_create (debug_name, PRI_DEFAULT,
                             (thread_func*)start_process, &arguments);
   
  if (thread_id == -1)
  {
    sema_up(&(arguments.sema));
  }
  sema_down(&(arguments.sema));
  
  
  if(arguments.is_success)
  {
    process_id = arguments.child_id;
  }
  else 
  {
    process_id = -1;
  }

debug("%s#%d: After sema down -> sema is %d AND Process_id is |%d|\n",
        thread_current()->name,
        thread_current()->tid,
        arguments.sema.value,
        process_id
      );

  /* AVOID bad stuff by turning off. YOU will fix this! */
  //power_off();
  
  // Kommando för att exekvera testprogrammet
  //pintos -p ../examples/sumargv -a sumargv -v -k --fs-disk=2 -- -f -q run 'sumargv 1 2 3'

  /* WHICH thread may still be using this right now? */
  // Måste ske efter printen i start_process.
  free(arguments.command_line);

  debug("%s#%d: process_execute(\"%s\") RETURNS %d\n",
        thread_current()->name,
        thread_current()->tid,
        command_line, process_id);

  /* MUST be -1 if `load' in `start_process' return false */
  return process_id;
}

/* ASM version of the code to set up the main stack. */
void *setup_main_stack_asm(const char *command_line, void *esp);


/* A thread function that loads a user process and starts it
   running. */
static void
start_process (struct parameters_to_start_process* parameters)
{
  /* The last argument passed to thread_create is received here... */
  struct intr_frame if_;
  bool success;

  char file_name[64];
  strlcpy_first_word (file_name, parameters->command_line, 64);
  
  debug("%s#%d: start_process(\"%s\") ENTERED\n",
        thread_current()->name,
        thread_current()->tid,
        parameters->command_line);
  
  /* Initialize interrupt frame and load executable. */
  memset (&if_, 0, sizeof if_);
  if_.gs = if_.fs = if_.es = if_.ds = if_.ss = SEL_UDSEG;
  if_.cs = SEL_UCSEG;
  if_.eflags = FLAG_IF | FLAG_MBS;

  success = load (file_name, &if_.eip, &if_.esp);

  debug("%s#%d: start_process(...): load returned %d\n",
        thread_current()->name,
        thread_current()->tid,
        success);

  // Process information has to be declared here because if load fails
  // thread_exit() will be called which calls in turn process_cleanup()

  struct p_info *process_info = malloc(sizeof(struct p_info));
  // process_map has to be locked here because if current process A gets preempted
  // inside plist_add_process when it found a place to add a new proces.
  // and another new process B gets to run plist_add_process(). When it will be 
  // A's turn to run it will think that the spot it previousy found is still empty
  // which is not, and it will write over.
  lock_acquire(&process_map_lock);
  int process_id = plist_add_process(&process_map, process_info);
  lock_release(&process_map_lock);

  process_info->status=-1;
  process_info->is_alive=true;
  process_info->parent_id=parameters->parent_id;
  process_info->status_needed=true;
  sema_init(&(process_info->sema), 0);
  process_info->id=process_id;
  thread_current()->id_in_process_map = process_id;    
  parameters->child_id = process_id; 
  
  if (success)
  {
    /* We managed to load the new program to a process, and have
       allocated memory for a process stack. The stack top is in
       if_.esp, now we must prepare and place the arguments to main on
       the stack. */

    /* A temporary solution is to modify the stack pointer to
       "pretend" the arguments are present on the stack. A normal
       C-function expects the stack to contain, in order, the return
       address, the first argument, the second argument etc. */
    
    // if_.esp -= 12; /* this is a very rudimentary solution */

    /* This uses a "reference" solution in assembler that you
       can replace with C-code if you wish. */
    if_.esp = setup_main_stack_asm(parameters->command_line, if_.esp);

    /* The stack and stack pointer should be setup correct just before
       the process start, so this is the place to dump stack content
       for debug purposes. Disable the dump when it works. */
    
//    dump_stack ( PHYS_BASE + 15, PHYS_BASE - if_.esp + 16 );


      // Stoppa in skapelse av processen här.
    // ta hand om 0 processen efter wait.
    parameters->is_success = true;
  }

  debug("%s#%d: start_process(\"%s\") DONE\n",
        thread_current()->name,
        thread_current()->tid,
        parameters->command_line);

  if(!success)
    { 
      parameters->is_success = false;
    }
  sema_up(&(parameters->sema));

  /* If load fail, quit. Load may fail for several reasons.
     Some simple examples:
     - File doeas not exist
     - File do not contain a valid program
     - Not enough memory
  */
  if ( ! success )
  {
    parameters->is_success = false;
    thread_exit ();
  }
  
  /* Start the user process by simulating a return from an interrupt,
     implemented by intr_exit (in threads/intr-stubs.S). Because
     intr_exit takes all of its arguments on the stack in the form of
     a `struct intr_frame', we just point the stack pointer (%esp) to
     our stack frame and jump to it. */
  asm volatile ("movl %0, %%esp; jmp intr_exit" : : "g" (&if_) : "memory");
  NOT_REACHED ();
}

/* Wait for process `child_id' to die and then return its exit
   status. If it was terminated by the kernel (i.e. killed due to an
   exception), return -1. If `child_id' is invalid or if it was not a
   child of the calling process, or if process_wait() has already been
   successfully called for the given `child_id', return -1
   immediately, without waiting.

   This function will be implemented last, after a communication
   mechanism between parent and child is established. */
int
process_wait (int child_id) 
{
  int status = -1;
  struct thread *cur = thread_current ();
  struct p_info *process_info = plist_find_process(&process_map, cur->id_in_process_map);

  debug("%s#%d: process_wait(%d) ENTERED\n",
        cur->name, cur->tid, child_id);
  /* Yes! You need to do something good here ! */

  // Find child process.
  value_p child_process = plist_find_process(&process_map, child_id);

  // 1. Barn måste finnas för att kunna väntas på.
  if(child_process == NULL)
  {
    return -1;
  }

  //2. Förälderns ID (nuvarande process ID) måste överensstämma med barnets parent_id
  // if status needed == false we have already waited on it
  if(process_info->id != child_process->parent_id || child_process->status_needed == false)
  {
    return -1;
  }
  
  sema_down(&child_process->sema);

  // Returnera barnprocessens exit status.
  status = child_process->status;
  child_process->status_needed = false;

  return status;
}


/*
 * Iterates though the process map and removes processes that have
 * exited and their status is not needed.
 * Example for when this is needed:
 * Assume two processes, P1 and P2, P2 is a child processes of P1.
 * P2 exits(72) we can't delete it because status needed is still required
 * P1 exits(1) and we can detelete P1 because it's not needed.
 * However, P2 will not to deleted from the list even though it's dead and it's status is
 * not needed.
 */
static void
process_cull(void)
{ 
  // lock because compilator may think this is a single 
  // thread program and compile out free
  lock_acquire(&process_map_lock);
  for(int i=0; i<MAP_SIZE-1; i++)
  {
    struct p_info *cur_process = plist_find_process(&process_map, i);
    if(cur_process != NULL && cur_process->is_alive == false 
       && cur_process->status_needed == false)
    {
      value_p process = plist_remove_process(&process_map, cur_process->id);
      free(process);
    }
  }
  lock_release(&process_map_lock);

}



/* Free the current process's resources. This function is called
   automatically from thread_exit() to make sure cleanup of any
   process resources is always done. That is correct behaviour. But
   know that thread_exit() is called at many places inside the kernel,
   mostly in case of some unrecoverable error in a thread.

   In such case it may happen that some data is not yet available, or
   initialized. You must make sure that any data needed IS available
   or initialized to something sane, or else that any such situation
   is detected.
*/
  
void
process_cleanup (void) // nånstans här, stäng alla öppna filer. DONE
{

  struct thread  *cur = thread_current ();
  uint32_t       *pd  = cur->pagedir;
  // Current status of process in here becomes exit_status.
  struct p_info *this_process = plist_find_process(&process_map, cur->id_in_process_map);
  int status = this_process->status;
  
  debug("%s#%d: process_cleanup() ENTERED\n", cur->name, cur->tid);
  
  /* Later tests DEPEND on this output to work correct. You will have
   * to find the actual exit status in your process list. It is
   * important to do this printf BEFORE you tell the parent process
   * that you exit.  (Since the parent may be the main() function,
   * that may sometimes poweroff as soon as process_wait() returns,
   * possibly before the printf is completed.)
   */
  printf("%s: exit(%d)\n", thread_name(), status);
  
  /* Destroy the current process's page directory and switch back
     to the kernel-only page directory. */
  if (pd != NULL) 
    {
      /* Correct ordering here is crucial.  We must set
         cur->pagedir to NULL before switching page directories,
         so that a timer interrupt can't switch back to the
         process page directory.  We must activate the base page
         directory before destroying the process's page
         directory, or our active page directory will be one
         that's been freed (and cleared). */
      cur->pagedir = NULL;
      pagedir_activate (NULL);
      pagedir_destroy (pd);
    }  

  // Check if process was found in process table.
  if( this_process != NULL )
  {
    // Close all related files to process. I.e. process file table to null. Don't know that it works.
    for (int i = 0; i < MAP_SIZE; i++) {
      struct file* file = map_find(&thread_current()->container, i);
      file_close(file);
    }

    // Set status_needed=false on all children of this_process.
    plist_orphan_my_children( &process_map, this_process->id );

    // remove process if parent is dead or status_needed is false
    if( !this_process->status_needed == false) 
    {
      sema_up(&this_process->sema);
    }
    process_cull();
  }
}



/* Sets up the CPU for running user code in the current
   thread.
   This function is called on every context switch. */
void
process_activate (void)
{
  struct thread *t = thread_current ();

  /* Activate thread's page tables. */
  pagedir_activate (t->pagedir);

  /* Set thread's kernel stack for use in processing
     interrupts. */
  tss_update ();
}

