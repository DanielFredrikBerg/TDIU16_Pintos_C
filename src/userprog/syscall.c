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

#define DBG(format, ...) printf(format "\n", ##__VA_ARGS__)

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


static void
syscall_handler (struct intr_frame *f)
{
  int32_t* esp = (int32_t*)f->esp;
  int32_t syscall_num = *(esp);
  /*
   * sys_call_id = esp[0]
   * fd = esp[1]
   * buffer = esp[2]
   * length = esp[3]
   */

  switch (  syscall_num /* retrive syscall number */ )
  {
    case SYS_EXIT:
    {
      printf("__EXIT STATUS = %d\n", *(esp+1));
      thread_exit();
      break;
    }


    case SYS_HALT:
    {
      puts("++INSIDE SYS_HALT\n");
      power_off();
      break;
      
    }


    case SYS_WRITE: /* int fd, void *buffer, unsigned lenght */
    {
      struct thread* current_thread = thread_current();
      int fd = esp[1];
      char* buffer = (char*)esp[2];
      int buffer_length = esp[3];
      //printf("\n------ FD = %d \n", fd);

      if(fd == STDIN_FILENO) 
      {
	      printf("ERROR SYS_WRITE: fd not STDOUT_FILENO\n");
        f->eax = -1;

      }
      else if (fd == STDOUT_FILENO)
      {
        putbuf(buffer, buffer_length); 
      }
      else 
      {
        struct file *file_ptr = map_find(&current_thread->container, fd);
        if (file_ptr)
        {
          // write from buffer to file_ptr
          f->eax = file_write(file_ptr, buffer, esp[3]);
        } else
        {
          f->eax = -1;
        }
      }
      break;
    }


    case SYS_READ:
    {
      struct thread* current_thread = thread_current();
      int fd = esp[1];
      char* buffer = (char*)esp[2];
      unsigned buffer_length = esp[3];

      if(fd == STDOUT_FILENO) 
      {
        f->eax = -1;
        break;
      }
      else if(fd == STDIN_FILENO) 
      {

        for(unsigned i = 0; i < buffer_length; i++)
        {
          char c = (char)input_getc();
          if(c == '\r')
          {
            c = '\n';
          }
          *(buffer+i) = c;
          putbuf(buffer+i, 1);          
        }
        f->eax = esp[3];
      } 
      else
      {
        struct file *file_ptr = map_find(&current_thread->container, fd);
        
        if(file_ptr)
        {
          f->eax = file_read(file_ptr, buffer, buffer_length);
        }
        else 
        {
          f->eax = -1;
        }

      }
      break;
    }


    case SYS_CREATE:
    {
      f->eax = filesys_create((char*)esp[1], esp[2]);
      break;
    }


    case SYS_OPEN:
    {
      struct thread* current_thread = thread_current();
      const char* file_name = (char*)esp[1];
      struct file* file_ptr = filesys_open(file_name);
      if (file_ptr == NULL)
      {
        printf("ERROR SYS_OPEN: File doe not exist\n");
        f->eax = -1;
      }
      else
      {
        // if fd is not open, -1 is returned. else the fd is returned
        int fd = map_contains_value(&current_thread->container, file_ptr);
        if (fd == -1) {
        printf("File exists but is not open.\nOpening and inserting file '%s' to process wide file table.\n",
	       file_name);
          fd = map_insert(&current_thread->container, file_ptr);
        }
        f->eax = fd;
      }
      break;
    }


    case SYS_CLOSE:
    {
      puts("------- INSIDE SYS_CLOSE");
      break;
    }


    case SYS_REMOVE:
    {
      puts("------- INSIDE SYS_REMOVE");
      break;
    }


    case SYS_SEEK:
    {
      puts("------- INSIDE SYS_SEEK");
      break;
    }


    case SYS_TELL:
    {
      puts("------- INSIDE SYS_TELL");
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
