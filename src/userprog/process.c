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


/* This function is called at boot time (threads/init.c) to initialize
 * the process subsystem. */
void process_init(void)
{
  // Borde denna vara här eller i thread_init? Varje trår borde ha sin egen process_map?
  plist_init(&process_map);


  debug("\n# get reckt nub");
  struct p_info *process_info = malloc(sizeof(struct p_info));
  int process_id = plist_add_process(&process_map, process_info);

  process_info->status=-1;
  process_info->is_alive=true;
  process_info->parent_id=-1;
  process_info->status_needed=true;
  sema_init(&(process_info->sema), 0);
  
  debug("# !!!!!!!!!!!!!!!!!!!!!!!!!!Process_id:%d\n", process_id);
  process_info->id=process_id;

}

/* This function is currently never called. As thread_exit does not
 * have an exit status parameter, this could be used to handle that
 * instead. Note however that all cleanup after a process must be done
 * in process_cleanup, and that process_cleanup are already called
 * from thread_exit - do not call cleanup twice! */
void process_exit(int status) 
// Meddela förälder vilken statuskod processen avslutades.
{
  int id = thread_current()->process_info->id;
  struct p_info *process_info = plist_find_process(&process_map, id);
  process_info->status = status;
  process_info->is_alive = false;
  // int id = thread_current()->process_info.id;
  // struct p_info *process_info = plist_find_process(&process_map, id);
  // process_info->status = status;
  // process_info->is_alive = false;

  debug("# ---------------- PROCESS %d EXIT with STATUS: %d", process_info->id, status);
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
  //int return_value; <- Might need later? (From lab08)
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


  debug("# FIRST %s#%d: process_execute(\"%s\") ENTERED\n",
        thread_current()->name,
        thread_current()->tid,
        command_line);

  /* COPY command line out of parent process memory */
  arguments.command_line = malloc(command_line_size);
  strlcpy(arguments.command_line, command_line, command_line_size);

  // Current threads process id becomes parent id for child process here.
  arguments.parent_id = thread_current()->process_info->id;
  
  strlcpy_first_word (debug_name, command_line, 64);
  
       
  thread_id = thread_create (debug_name, PRI_DEFAULT,
                             (thread_func*)start_process, &arguments);
  

    debug("# %s#%d: Before sema down, thread_id=%d\n",
        thread_current()->name,
        thread_current()->tid,
        thread_id);
   
    // Hack -> ask for approval!
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


  

  //process_wait(process_id);
debug("# %s#%d: After sema down -> sema is %d AND Process_id is |%d|\n",
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

  debug("# LAST %s#%d: process_execute(\"%s\") RETURNS %d\n",
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
  debug("# start_process");
  /* The last argument passed to thread_create is received here... */
  struct intr_frame if_;
  bool success;

  char file_name[64];
  strlcpy_first_word (file_name, parameters->command_line, 64);
  
  debug("# SECOND %s#%d: start_process(\"%s\") ENTERED\n",
        thread_current()->name,
        thread_current()->tid,
        parameters->command_line);
  
  /* Initialize interrupt frame and load executable. */
  memset (&if_, 0, sizeof if_);
  if_.gs = if_.fs = if_.es = if_.ds = if_.ss = SEL_UDSEG;
  if_.cs = SEL_UCSEG;
  if_.eflags = FLAG_IF | FLAG_MBS;

  success = load (file_name, &if_.eip, &if_.esp);

  debug("# %s#%d: start_process(...): load returned %d\n",
        thread_current()->name,
        thread_current()->tid,
        success);
        
  // malloc i start_process
  struct p_info *process_info = (struct p_info*)malloc(sizeof(struct p_info));
  process_info->status=-1;
  process_info->is_alive=true;
  process_info->parent_id=parameters->parent_id;
  process_info->status_needed=true;
  sema_init(&(process_info->sema), 0);
  
   
  int process_id = plist_add_process(&process_map, process_info);
  printf("########################Process_id:%d\n", process_id);
  process_info->id=process_id;
  parameters->child_id = process_id;
  process_print_list();
  
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
  debug("# Sema Up\n");
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
     debug("# $$$$$$$$$$$$$$ load failed, exiting thread.");
     
    

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

  debug("%s#%d: process_wait(%d) ENTERED\n",
        cur->name, cur->tid, child_id);
  /* Yes! You need to do something good here ! */

  // Find child process.
  value_p child_process = plist_find_process(&process_map, child_id);

  // 1. Barn måste finnas för att kunna väntas på.
  process_print_list();
  if(child_process == NULL)
  {
    printf("# [[[[[[]]]]]] ERROR: Child of process:%d not found!\n", cur->process_info->id);
    return -1;
  }

  // 2. Förälderns ID (nuvarande process ID) måste överensstämma med barnets parent_id
  if(cur->process_info->id != child_process->parent_id)
  {
    printf("# [[[[[[]]]]]] ERROR: Process:%d does not have child:%d !\n", cur->process_info->id, child_process->id);
    return -1;
  }

  sema_down(&child_process->sema);

  child_process->status_needed = false;
  // delete child here 
  // Returnera barnprocessens exit status.
  status = child_process->status;

  debug("%s#%d: process_wait(%d) RETURNS %d\n",
        cur->name, cur->tid, child_id, status);
  
  return status;
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
  int status = cur->process_info->status;
  
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
  /* WHAT NEEDS TO BE DONE HERE:
  *  1. Need to close all files related to current process. DONE
  *  2. Need to remove this process (id) from the process list if it's in there. DONE
  *  3. Need to set this process status to false as it exits in the process table. MAYBE DONE
  *  4. May not remove process if parent is alive. DONE
  *  5. When parent process exits -> set parent_alive to false on all child processes. DONE
  *  6. Status needed = false när föräldern inte lever eller när den egna processen
  *     måste stänga men föräldern lever (waited_on=true)
  *  7. Om barn vill avsluta innan förälder så måste status_needed sättas
  *     till false så att processen tas bort när föräldern tas bort.
  */
  // Get process id for this process.
  int this_PID = cur->process_info->id;

  // Get pointer to current process info in process table.
  struct p_info *this_process = plist_find_process(&process_map, this_PID);

  // Check if process was found in process table.
  if( this_process != NULL )
  {
    // Close all related files to process. I.e. process file table to null. Don't know that it works.
    map_init(&thread_current()->container);

    // Set status_needed=false on all children of this_process.
    // status_need false
    // ta bort barnet
    plist_orphan_my_children( &process_map, this_process->id );
    //plist_remove_my_children( &process_map, this_process->id );

    // remove process if parent is dead or status_needed is false
    if( this_process->status_needed == false) 
    {
      plist_remove_process(&process_map, this_PID);
    } else {
      sema_up(&this_process->sema);
    }
  }
  
  debug("%s#%d: process_cleanup() DONE with status %d\n",
        cur->name, cur->tid, status);
  debug("# %d", this_process->id);

  if(this_process->id == 2)
  {
    debug("# Process 2 has finished cleanup it should still be in the list");
    process_print_list();
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

