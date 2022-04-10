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

void syscall_init (void)
{
  //DBG("_________SYSCALL INITIATED row: %d, file: %s", __LINE__, __FILE__);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
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
      puts("++INSIDE SYS_EXIT");
      
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
    case SYS_WRITE:
    {
      puts("----SYS WRITE");
      if(esp[1] == STDIN_FILENO) 
      {
        f->eax = -1;
        break;
      }

      if (esp[1] == STDOUT_FILENO)
      {
        putbuf((char*)esp[2], esp[3]); 
      }
      f->eax = esp[3];
      //return f->eax;
      break;
    }
    case SYS_READ:
    {
      puts("----SYS READ");
      if(esp[1] == STDOUT_FILENO) 
      {
        f->eax = -1;
        break;
      }
      if(esp[1] == STDIN_FILENO) 
      {
        char buffer[esp[3]]; //
        for(int i = 0; i < esp[3]; i++)
        {
          buffer[i] = input_getc();
          if(buffer[i] == '\r')
          {
            buffer[i] = '\n';
          }
        }

        putbuf((char*)&buffer, 1); 

        esp[2] = (int32_t)buffer;

        f->eax = esp[3];
      }
      // kinda works
      // if(esp[1] == STDIN_FILENO) 
      // {
      //   uint8_t buffer[esp[3]]; //
      //   for(int i = 0; i < esp[3]; i++)
      //   {
      //     buffer[i] = input_getc();
          
      //     if(buffer[i] == '\r')
      //     {
      //       buffer[i] = '\n';
      //     }
      //     printf("%c", buffer[i]);

      //     //putbuf((char*)buffer[i], 1); 
      //   }
      //   esp[2] = (int32_t)buffer;

      //   f->eax = esp[3];
      // }
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
