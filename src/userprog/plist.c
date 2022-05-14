#include <stddef.h>

#include "plist.h"




void plist_init(struct p_list* m)
{
  for (unsigned i = 0; i < PMAP_SIZE; i++)
  {
    m->content[i] = NULL;
  }
}


key_t plist_add_process(struct p_list* m, value_p pi_t)
{
  value_p *walker = m->content;
  int counter = 0;
  while (*walker != NULL)
  {
    walker++;
    counter++;
  }
  *walker = pi_t;
  if(!check_within_bounds(counter))
  {
    printf("# \nError plist_insert: map is full!\n");
    return -1;
  }
  
  return counter; 
}


value_p plist_find_process(struct p_list* m, key_t k)
{
  if (check_within_bounds(k))
  {
    return m->content[k];
  } 
  else 
  {
    printf("# \nERROR plist_find: Key out of bounds!");
    return NULL;
  }
}


value_p plist_remove_process(struct p_list* m, key_t k)
{
  if (!check_within_bounds(k))
  {
    printf("# \nERROR plist_remove: Key out of bounds!");
    return NULL;
  }
  else
  {
    if (m->content[k] != NULL)
    {
      if(k==1) puts("bam get reckt");
      value_p ret = m->content[k];
      m->content[k] = NULL;
      return ret;
    }
    else
    {
      debug("#\nValue for key not allocated id %d", k);
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
  while (counter < PMAP_SIZE)
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


void plist_orphan_my_children(struct p_list* m, key_t parent_id)
{
  value_p process_list = m->content;
  for(int i=0; i<PMAP_SIZE-1; i++)
  {
    if(process_list[i].parent_id == parent_id)
    {
      process_list[i].status_needed = false;
    }
  }
}


void plist_remove_my_children(struct p_list* m, key_t parent_id)
{
  value_p process_list = m->content;
  for(int i=0; i<PMAP_SIZE-1; i++)
  {
    if(process_list[i].parent_id == parent_id)
    {
      plist_remove_process(m, process_list[i].id);
    }
  }
}


