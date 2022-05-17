#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "devices/disk.h"
#include "threads/synch.h"

/* dir_remove not thread safe??????????????????????
 * lock inode_remove()? 
 *
 * DONE 
 * 1. Katalogen är tom. Två processer lägger till filen ”kim.txt” samtidigt. Är det 
 * efteråt garanterat attkatalogen innehåller endast en fil ”kim.txt”? -- NEJ
 * 
 * -- directory.c --dir_add 
 * ANSWER:
 *  No, it is not guaranteed, two files could be created
 * SOLUTION:
 *  Add a lock, I am guessing global(DONE). Could it be possible to make it work with a local
 * lock? In struct dir?
 *
 * 
 * ?????????
 * 2. Katalogen innehåller en fil ”kim.txt”. Två processer tar bort ”kim.txt”, 
 * och en process lägger samtidigt till ”kam.txt”. Är det efteråt garanterat att 
 * katalogen innehåller endast fil ”kam.txt”? -- JA ? NO?
 * inode_remove("kim.txt")
 * inode_remove("kim.txt")
 * inode_open("kam.txt")
 * 
 * ANSWER:
 *  Yes there would only be one kam.txt however it would not be a terrible idea to lock
 *  inode_remove.
 * 
 * 
 * DONE
 * 3. Systemets globala inode-lista är tom. Tre processer öppnar samtidigt filen ”kim.txt”. 
 * Är det garanterat att inode-listan sedan innehåller endast en cachad referens 
 * till filen, med open_cnt lika med 3? -- NEJ
 * 
 * inode.c
 * open_inodes = global list of inode references
 * inode_open() 3 times at the same time
 * ANSWER:
 *  No it is not guaranteed that the that open_inodes will contain only one reference to
 *  kim.txt. If the three processes call inode_open() at the same time it could result in three
 *  different references to kim.txt in open_inodes.
 * SOLUTION:
 *  Lock the open_inodes array that way we can look through, add inodes as we please knowing
 *  that no other threads can access this list. If the inode already exists release the lock
 *  and call inode_reopen(). Inode_reopen has a critical variable in open count. Here we 
 *  use the local lock of the inode to protect it while making changes to the count.
 *  If the inode is not already opened read from disk and create the inode, release the lock.
 *
 *  
 * DONE
 * 4. Systemets globala inode-lista innehåller en referens till ”kim.txt” med open_cnt 
 * lika med 1. En process stänger filen samtidigt som en annan process öppnar filen. 
 * Är det garanterat att inode-listan efteråt innehåller samma information? -- NEJ
 * 
 * open_inodes(["kim.txt"])
 * inode_open("kim.txt")
 * inode_close("kim.txt")
 * 
 * ANSWER:
 *  No, if inode_close() start first it will realise the open_couter is zero so it will delete
 *  the inode. Meanwhile inode_open who just got a reference to the file will try to increase
 *  the open_counter in inode_reopen() because it thinks the file is already opened
 *  resulting in a segmentaion fault because the inode has already been freed.
 * 
 * SOLUTION:
 *  Lock in inode_reopen() protecting open_counter. This lock should be local to the current 
 *  inode as to not tank performance.
 *  Lock also in inode_close, protect open_couter. Unsure about list_remove (&inode->elem)??????? 
 * 
 * 
 * DONE
 * 5. Free-map innehåller två sekvenser med 5 lediga block. Två processer skapar 
 * samtidigt två filer som behöver 5 lediga block. Är det efteråt garanterat att 
 * filerna har fått var sin sekvens lediga block?
 * 
 * *free_map_file
 * free_map_allocate()
 * free_map_allocate()
 * 
 * ANSWER:
 *  NO, If the processes are created simultaneously it's not guaranteed the each process
 * will allocate 2x5 continous blocks for its own files.
 * SOLUTION:
 *  Lock the free_map_file so that each process can allocate continious blocks.
 * 
 * 
 * DONE
 * 6. Katalogen innehåller en fil ”kim.txt”. Systemets globala inode-lista innehåller 
 * en referens till samma fil med open_cnt lika med 1. Free-map har 5 block markerade 
 * som upptagna. En process tar bort filen ”kim.txt” samtidigt som en annan process 
 * stänger filen ”kim.txt”. Är det efteråt garanterat att inode-listan är tom, att 
 * free-map har 5 nya lediga block, och att katalogen är tom?
 * 
 * dir = kim.txt
 * open_inodes = kim.txt(open_count = 1)
 * free_map_file(5 blocks marked taken by kim.txt)
 * ????dir_remove(kim.txt) / inode_remove(kim.txt)
 * ????inode_close(kim.txt)
 * 
 * ?????ANSWER:
 *  No, seeing as dir_remove() could run paralell to inode_close() the inode could be freed
 *  in dir_remove() causing inode_close() to give an error when doing --.
 * Solution:
 *  Sync dir_remove() protect the dir which in turn protects the inode.
 * Old Solution:
 *  NEJ -  Utan en koordinerad ordning är det inte garanterat att alla aktioner alltid
 *  sker i en viss ordning.
 * 
 * 
 * 
 * 7. Katalogen innehåller en fil ”kim.txt”. En process försöker öppna filen samtidigt 
 * som en annan process tar bort filen ”kim.txt” och skapar sedan en ny fil ”kam.txt”. 
 * Är det efteråt garanterat att den första processen antingen lyckades öppna filen 
 * ”kim.txt”, eller att den misslyckades? Eller kan det bli så att den råkar 
 * öppna ”kam.txt” i stället?
 * 
 * 
 * open_inodes["kim.txt(1)"]
 * inode_open("kim.txt")
 * inode_close("kim.txt")
 * inode_open("kam.txt")
 * 
 * 8. Liknande frågor skall du själv ställa dig i relation till din process-lista 
 * och till din(a) fil-list(or).
 * // global process_map lock?
 * // local per process lock?
 * 
 * 
 */ 

/* The disk that contains the file system. */
struct disk *filesys_disk;

static void do_format (void);

/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
void
filesys_init (bool format) 
{
  filesys_disk = disk_get (0, 1);
  if (filesys_disk == NULL)
    PANIC ("hd0:1 (hdb) not present, file system initialization failed");

  inode_init ();
  free_map_init ();

  if (format) 
    do_format ();

  free_map_open ();
}

/* Shuts down the file system module, writing any unwritten data
   to disk. */
void
filesys_done (void) 
{
  free_map_close ();
}

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
bool
filesys_create (const char *name, off_t initial_size) 
{
  disk_sector_t inode_sector = 0;
  struct dir *dir = dir_open_root ();
  bool success = (dir != NULL
                  && free_map_allocate (1, &inode_sector)
                  && inode_create (inode_sector, initial_size)
                  && dir_add (dir, name, inode_sector));
  if (!success && inode_sector != 0) 
    free_map_release (inode_sector, 1);
  dir_close (dir);
  
  return success;
}

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
struct file *
filesys_open (const char *name)
{
  struct dir *dir = dir_open_root ();
  struct inode *inode = NULL;
  struct file *file = NULL;

  if (dir != NULL)
    dir_lookup (dir, name, &inode);
  dir_close (dir);

  file = file_open (inode);
  
  return file;
}

void
filesys_close (struct file *file)
{
  file_close(file);
}

/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *name) 
{
  struct dir *dir = dir_open_root ();
  bool success = dir != NULL && dir_remove (dir, name);
  dir_close (dir); 
  
  return success;
}

/* Formats the file system. */
static void
do_format (void)
{
  printf ("Formatting file system...");
  free_map_create ();
  if (!dir_create (ROOT_DIR_SECTOR, 16))
    PANIC ("root directory creation failed");
  free_map_close ();
  printf ("done.\n");
}
