#include <stddef.h>

#include "plist.h"




void plist_init(struct p_list* plist)
{
  for (unsigned i = 0; i < MAP_SIZE; i++)
  {
    plist->content[i] = NULL;
  }
  lock_init(&plist->plist_lock);
}


key_t plist_add_process(struct p_list* plist, value_p new_process)
{
  lock_acquire(&plist->plist_lock);
  for (int i = 0; i < MAP_SIZE; i++)
  {
    if(plist->content[i] == NULL)
    {
      plist->content[i] = new_process;
      lock_release(&plist->plist_lock);
      return i;
    }
  }
  lock_release(&plist->plist_lock);
  return -1;
}


value_p plist_find_process(struct p_list* plist, key_t process_id)
{
  lock_acquire(&plist->plist_lock);
  if (check_within_bounds(process_id))
  {
    lock_release(&plist->plist_lock);
    return plist->content[process_id];
  } 
  else 
  {
    debug("#ERROR plist_find: Key out of bounds!\n");
    lock_release(&plist->plist_lock);
    return NULL;
  }
}


value_p plist_remove_process(struct p_list* plist, key_t process_id)
{
  lock_acquire(&plist->plist_lock);
  if (!check_within_bounds(process_id))
  {
    debug("#ERROR plist_remove: Key out of bounds!\n");
    lock_release(&plist->plist_lock);
    return NULL;
  }
  else
  {
    if (plist->content[process_id] != NULL)
    {
      value_p ret = plist->content[process_id];
      plist->content[process_id] = NULL;
      lock_release(&plist->plist_lock);
      return ret;
    }
    else
    {
      debug("#Value for key not allocated\n");
      lock_release(&plist->plist_lock);
      return NULL;
    }
  }
}

//TODO daniel has provided
void plist_print(struct p_list* m)
{
  int counter = 0;
  puts("# -----------------------------------------------");
  puts("# --- Process info table -------------------------");
  puts("# ID  STATUS  IS_ALIVE  PARENT_ID  STATUS_NEEDED");
  while (counter < MAP_SIZE)
  {
    if (m->content[counter] != NULL)
    {
        printf("# %2d%8d%10d%11d%15d\n", 
        m->content[counter]->id, m->content[counter]->status,
        m->content[counter]->is_alive, m->content[counter]->parent_id,
        m->content[counter]->status_needed);
    }
  counter++;
  }
  puts("# ------------------------------------------------");
}


void plist_orphan_my_children(struct p_list* plist, key_t parent_id)
{
  lock_acquire(&plist->plist_lock);
  value_p process_list = plist->content;
  for(int i=0; i<MAP_SIZE-1; i++)
  {
    if(process_list[i].parent_id == parent_id)
    {
      process_list[i].status_needed = false;
    }
  }
  lock_release(&plist->plist_lock);
}
