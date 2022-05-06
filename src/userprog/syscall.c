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
pid_t syscall_exec(const char* command_line)
{
  return process_execute(command_line);
}


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
  int32_t syscall_num = *(esp);
  struct thread* current_thread = thread_current();
  int fd = esp[1];

  switch (  syscall_num )
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
      //TODO: implement syscall_exec - DONE.
      f->eax = process_execute((char*)esp[1]);
      break;
    }

    case SYS_WRITE: /* int fd, void *buffer, unsigned lenght */
    {
      f->eax = syscall_write(current_thread, fd, (char*)esp[2], esp[3]);
      break;
    }

    case SYS_READ:
    {
      f->eax = syscall_read(current_thread, fd, (char*)esp[2], esp[3]);
      break;
    }

    case SYS_CREATE:
    {
      f->eax = filesys_create((char*)esp[1], esp[2]);
      break;
    }

    case SYS_OPEN:
    {
      f->eax = syscall_open(current_thread, (char*)esp[1]);
      break;
    }

    case SYS_CLOSE:
    {
      syscall_close(current_thread, fd);
      break;
    }

    case SYS_REMOVE:
    {
      const char* file_name = (char*)esp[1];
      f->eax = filesys_remove(file_name);
      break;
    }

    case SYS_SEEK:
    {
      syscall_seek(current_thread, fd, esp[2]);
      break;
    }

    case SYS_TELL:
    {
      f->eax = syscall_tell(current_thread, fd);
      break;
    }

    case SYS_FILESIZE:
    {
      f->eax = syscall_filesize(current_thread, fd);
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
      process_wait(wait_on_child_id);
      break;
    }

    default:
    {
      printf ("______Executed an unknown system call!\n");
      printf ("___Stack top + 0: %d\n", esp[0]);
      printf ("___Stack top + 1: %d\n", esp[1]);
      thread_exit ();
    }
  }
}
