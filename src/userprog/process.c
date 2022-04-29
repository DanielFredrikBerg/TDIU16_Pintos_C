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
  plist_init(&process_map);
}

/* This function is currently never called. As thread_exit does not
 * have an exit status parameter, this could be used to handle that
 * instead. Note however that all cleanup after a process must be done
 * in process_cleanup, and that process_cleanup are already called
 * from thread_exit - do not call cleanup twice! */
void process_exit(int status UNUSED) 
// Meddela förälder vilken statuskod processen avslutades.
{
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
  
  //lab09 Innan thread_create:
  //1. Addera processen här innan den nya skapats gör att föräldern har ansvar: NEJ.
  //2. Ja, Förälderns process id direkt tillgängligt.
  //3. Ja direkt(?)
  //4. Ja processen kommer alltid läggas till i processlistan före process_cleanup.
  //5. Ja, absolut. Tankegång -> process måste existera innan den kan börja exekvera.
  //   Process cleanup har alltid något att städa om något går fel.

  // lab09 Within thread_create:
  // 1. Yes, but the function shouldn't be changed.
  // 2. Förälderns process id kan lätt ordnas.
  // 3. assld.
  
  // lab09 Add process within start_process
  // 1. Ja då det är i skapelsen av sin egen tråd.
  // 2. Ja det går att ordna via struct parameters_to_process
  // 3. ja direkt då den skapas där.
  // 4. Vi tror process_cleanup kan köra innan tråden och processen startats vid ex fel.
  // 5. Ja, all data som kan behövas finns tillgänglig.

  // lab09 After thread_create
  // 1. Nej det gör förälder tråden.
  // 2. Ja, förälderns process id är tillgängligt direkt.
  // 3. Ja, eftersom thread och process inte är kopplade.
  // 4. Nej
  // 5. Ja? men processen läggs till i process_listan när processen redan startat/är klar?
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


  debug(" FIRST %s#%d: process_execute(\"%s\") ENTERED\n",
        thread_current()->name,
        thread_current()->tid,
        command_line);

  /* COPY command line out of parent process memory */
  arguments.command_line = malloc(command_line_size);
  strlcpy(arguments.command_line, command_line, command_line_size);

  // Current threads process id becomes parent id for child process here.
  arguments.parent_id = thread_current()->process_info.id;

  strlcpy_first_word (debug_name, command_line, 64);
  
       
  thread_id = thread_create (debug_name, PRI_DEFAULT,
                             (thread_func*)start_process, &arguments);
  
    debug("%s#%d: Before sema down, thread_id=%d\n",
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
    process_id = thread_id;
  }
  else 
  {
    process_id = -1;
  }
  

  //process_wait(process_id);
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

  debug("LAST %s#%d: process_execute(\"%s\") RETURNS %d\n",
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
  debug("start_process");
  /* The last argument passed to thread_create is received here... */
  struct intr_frame if_;
  bool success;

  char file_name[64];
  strlcpy_first_word (file_name, parameters->command_line, 64);
  
  debug("SECOND %s#%d: start_process(\"%s\") ENTERED\n",
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

  int process_id = plist_add_process(&process_map, &(thread_current()->process_info));
    // Stoppa in skapelse av processen här.
    thread_current()->process_info.id=process_id;
    thread_current()->process_info.status=-1;
    thread_current()->process_info.is_alive=true;
    thread_current()->process_info.parent_id=parameters->parent_id;
    thread_current()->process_info.status_needed=true;
    
   parameters->is_success = true;
   
  
   
  }

  
  debug("%s#%d: start_process(\"%s\") DONE\n",
        thread_current()->name,
        thread_current()->tid,
        parameters->command_line);
  debug("Sema Up\n");
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
     debug("$$$$$$$$$$$$$$ load failed, exiting thread.");
     
    

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
   initialized. You must make sure that nay data needed IS available
   or initialized to something sane, or else that any such situation
   is detected.
*/
  
void
process_cleanup (void) // nånstans här, stäng alla öppna filer.
{
  struct thread  *cur = thread_current ();
  uint32_t       *pd  = cur->pagedir;
  int status = -1;
  
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
    plist_remove_process(&process_map, thread_current()->process_info.id );
  debug("%s#%d: process_cleanup() DONE with status %d\n",
        cur->name, cur->tid, status);
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

