#include <stdio.h>
#include <syscall-nr.h>
#include "userprog/syscall.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include <inttypes.h>

/* header files you probably need, they are not used yet */
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "threads/vaddr.h"
#include "threads/init.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include "devices/input.h"
#include "devices/timer.h" // new import
#include "plist.h"

#define DBG(format, ...) printf(format "\n", ##__VA_ARGS__)

typedef int pid_t;

static void syscall_handler (struct intr_frame *);
struct map* file_table;

static
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

  // stop process from executing in kernel space as a user program
  if (!is_user_vaddr(start))
    return false;

  
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

static
bool verify_variable_length(char* start)
{
  const char* walker = start;
  unsigned starting_page = pg_no((const void*)start);

  //Check if first page is valid.
  if (pagedir_get_page(thread_current()->pagedir, walker) == NULL) {
    return false;
  }

  // If starting in the middle of a page we won't have
  // to check if page is valid again, but somehow know
  // when we go over to the next page.
  unsigned current_page = pg_no((const char*)walker);
  while(current_page == starting_page)
  {
    if(*walker == '\0')
    {
      return true;
    }
    walker++;
    current_page = pg_no((const char*)walker);
  }

  // Statement run once every page.
  // Return false if the new page is NULL.
  while ( pagedir_get_page(thread_current()->pagedir, walker) != NULL )
  {
    // Check each address in page if its the end \0
    for(int i=0; i<PGSIZE; i++)
    {
      if(*walker == '\0')
      {
        return true;
      }
      walker++;
    }
  } 
  return false;
}


void syscall_init (void)
{
  //DBG("_________SYSCALL INITIATED row: %d, file: %s", __LINE__, __FILE__);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  //map_init(file_table);
}


/* This array defined the number of arguments each syscall expects.
   For example, if you want to find out the number of arguments for
   the read system call you shall write:

   int sys_read_arg_count = argc[ SYS_READ ];

   All system calls have a name such as SYS_READ defined as an enum
   type, see `lib/syscall-nr.h'. Use them instead of numbers.
 */
const int argc[] = {
  /* basic calls */
  0, 1, 1, 1, 2, 1, 1, 1, 3, 3, 2, 1, 1,
  /* not implemented */
  2, 1,    1, 1, 2, 1, 1,
  /* extended, you may need to change the order of these two (plist, sleep) */
  0, 1
};


static
int syscall_write(struct thread* thread, int fd, char* buffer, int buffer_length)
{
  if(fd == STDIN_FILENO) 
      {
        return -1;
      }
    else if (fd == STDOUT_FILENO)
    {
      // write to screen
      putbuf(buffer, buffer_length);
      return buffer_length;
    }
    else 
    {
      // fd is a file
      struct file *file_ptr = map_find(&thread->container, fd);
      if (file_ptr)
      {
        // write from buffer to file_ptr
        return file_write(file_ptr, buffer, buffer_length);
      } else
      {
        return -1;
      }
    }
}


static
int syscall_read(struct thread* thread, int fd, char* buffer, int buffer_length)
{
  if(fd == STDOUT_FILENO) 
  {
    return -1;
  }
  else if(fd == STDIN_FILENO) 
  {
    // read from keyboard
    for(int i = 0; i < buffer_length; i++)
    {
      char c = (char)input_getc();
      if(c == '\r')
      {
        c = '\n';
      }
      *(buffer+i) = c;
      putbuf(buffer+i, 1);          
    }
    return buffer_length;
  } 
  else
  {
    // read from file
    struct file *file_ptr = map_find(&thread->container, fd);
    
    if(file_ptr)
    {
      return file_read(file_ptr, buffer, buffer_length);
    }
    else 
    {
      // file does not exists;
      return -1;
    }
  }
}


static
int syscall_open(struct thread* thread, const char* file_name)
{
  struct file* file_ptr = filesys_open(file_name);
  if (file_ptr)
  {
    // Process opens file.
    int fd = map_insert(&thread->container, file_ptr);
    if (fd == -1) {
      // max files open
      file_close(file_ptr);
    }
    return fd;
  }
  else
  {
    // file does not exist
    return -1;
  }
  file_close(file_ptr);
  return -1;
}


static
void syscall_close(struct thread* thread, int fd)
{
  struct file *file_ptr = map_find(&thread->container, fd);

  if(file_ptr)
  {
    map_remove(&thread->container, fd);
    file_close(file_ptr);
  }
}


static
void syscall_seek(struct thread* thread, int fd, int32_t new_pos)
{
  struct file *file_ptr = map_find(&thread->container, fd);

  if (file_ptr)
  {
    /* calling seek past filesize is defined to return the position of the end of file */
    int32_t max_len = file_length(file_ptr); 
    if (new_pos > max_len)
    {
      file_seek(file_ptr, max_len);
    }
    else
    {
      file_seek(file_ptr, new_pos);
    }
  }
}


static
int syscall_tell(struct thread* thread, int fd)
{
  struct file *file_ptr = map_find(&thread->container, fd);
  if(file_ptr)
  {
    return file_tell(file_ptr);
  } 
  else
  {
    return -1;
  } 
}


static
int syscall_filesize(struct thread* thread, int fd)
{
  struct file *file_ptr = map_find(&thread->container, fd);

  if(file_ptr)
  {
    return file_length(file_ptr);
  } 
  else
  {
    return -1;
  }
}

static
void syscall_plist(void)
{
  process_print_list(); 
}

static
void syscall_sleep(int millis)
{
  timer_msleep(millis);
}

static void
syscall_handler (struct intr_frame *f)
{

  int32_t* esp = (int32_t*)f->esp;
  struct thread* current_thread = thread_current();
  //int fd = esp[1];

  int esp_size = sizeof(*esp);
  

  // check if the esp stack is in valid page space  
  if (!verify_fix_length((void*)esp, esp_size)) {
    thread_exit();
  }

  // check that the system call is within defined sys_call range
  if (esp[0] < 0 || esp[0] > SYS_NUMBER_OF_CALLS) {
    thread_exit();
  }

  // the size of all given arguments shouldn't be larger than argc_size
  int argc_size = esp_size * argc[esp[0]];
  if (!verify_fix_length((void*)&esp[1], argc_size))
    thread_exit();
  

  switch ( esp[0] )
  {
    case SYS_EXIT:
    {
      int exit_status = esp[1];
      process_exit(exit_status);
      thread_exit();
      break;
    }

    case SYS_HALT:
    {
      power_off();
      break;
    }

    case SYS_EXEC:
    {
      // check the argument sent to the syscall so that it ends with a null terminating value
      // and be in valid space.
      if(!verify_variable_length((char*)esp[1]))
        thread_exit();

      f->eax = process_execute((char*)esp[1]);
      break;
    }

    case SYS_WRITE: /* int fd, void *buffer, unsigned lenght */
    {
      // check that we write within buffer space
      if(!verify_fix_length((char*)esp[2], esp[3]))
        thread_exit();

      f->eax = syscall_write(current_thread, esp[1], (char*)esp[2], esp[3]);
      break;
    }

    case SYS_READ:
    {
      // check that we read within buffer space
      if(!verify_fix_length((char*)esp[2], esp[3]))
        thread_exit();

      f->eax = syscall_read(current_thread, esp[1], (char*)esp[2], esp[3]);
      break;
    }

    case SYS_CREATE:
    {
      // check that name within initial_size of the name (esp[2])
      if(!verify_fix_length((char*)esp[1], esp[2]))
        thread_exit();


      f->eax = filesys_create((char*)esp[1], esp[2]);
      break;
    }

    case SYS_OPEN:
    {
      // check that file name has '\0' within a valid page. 
      if(!verify_variable_length((char*)esp[1]))
      {
        thread_exit();
      }
      f->eax = syscall_open(current_thread, (char*)esp[1]);
      break;
    }

    case SYS_CLOSE:
    {
      syscall_close(current_thread, esp[1]);
      break;
    }

    case SYS_REMOVE:
    {
      // check that file name has '\0' within a valid page.
      if(!verify_variable_length((char*)esp[1]))
        thread_exit();

      f->eax = filesys_remove((char*)esp[1]);
      break;
    }

    case SYS_SEEK:
    {
      syscall_seek(current_thread, esp[1], esp[2]);
      break;
    }

    case SYS_TELL:
    {
      f->eax = syscall_tell(current_thread, esp[1]);
      break;
    }

    case SYS_FILESIZE:
    {
      f->eax = syscall_filesize(current_thread, esp[1]);
      break;
    }

    case SYS_PLIST:
    {
      syscall_plist(); 
      break;
    }

    case SYS_SLEEP:
    {
      int milliseconds = esp[1];
      syscall_sleep(milliseconds); 
      break;
    }

    case SYS_WAIT:
    {
      int wait_on_child_id = esp[1];
      f->eax = process_wait(wait_on_child_id);
      break;
    }
    default:
    {
      printf ("Executed an unknown system call!\n");
      printf ("Stack top + 0: %d\n", esp[0]);
      printf ("Stack top + 1: %d\n", esp[1]);
      
      thread_exit ();
    }
  }
}
