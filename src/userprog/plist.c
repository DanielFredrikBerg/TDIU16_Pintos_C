#include <stddef.h>

#include "plist.h"

struct p_info
{
  int id;
  int status;
  bool is_alive;
  int parent_id;
  bool status_needed;
};


void plist_init(struct p_list* m)
{
  for (unsigned i = 0; i < MAP_SIZE; i++)
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
    printf("\nError plist_insert: map is full!\n");
    return -1;
  }
  return counter; // fixa ogiltig fd (out of bounds)
}


value_p plist_find_process(struct p_list* m, key_t k)
{
  if (check_within_bounds(k))
  {
    return m->content[k];
  } 
  else 
  {
    printf("\nERROR plist_find: Key out of bounds!\n");
    return NULL;
  }
}


value_p plist_remove_process(struct p_list* m, key_t k)
{
  if (!check_within_bounds(k))
  {
    printf("\nERROR plist_remove: Key out of bounds!\n");
    return NULL;
  }
  else
  {
    if (m->content[k] != NULL)
    {
      value_p ret = m->content[k];
      m->content[k] = NULL;
      return ret;
    }
    else
    {
      puts("\nValue for key not allocated\n");
      return NULL;
    }
  }
}

//TODO daniel will provide 
void plist_print(struct p_list* m)
{
  int counter = 0;
  puts("---Process info table :");
  puts("ID  STATUS  IS_ALIVE  PARENT_ID  STATUS_NEEDED");
  while (counter < MAP_SIZE)
  {
    if (m->content[counter] != NULL)
    {
        printf("%d     %d       %d        %d        %d", 
        m->content[counter]->id, m->content[counter]->status,
        m->content[counter]->is_alive, m->content[counter]->parent_id,
        m->content[counter]->status_needed);
    }
  counter++;
  }
  puts("--------------------------");
}