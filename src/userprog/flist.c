#include <stddef.h>
#include "flist.h"
//#include "../filesys/file.h"
#include <stdlib.h>

void map_init(struct map *m)
{
  printf("Map size: %d\n", (int)sizeof(m->content) / 8);
  for (unsigned i = BEGIN; i < MAP_SIZE; i++)
  {
    m->content[i] = NULL;
  }
}

key_t map_insert(struct map *m, value_t v)
{
  value_t *walker = m->content+BEGIN;
  int counter = BEGIN;
  while (*walker != NULL)
  {
    walker++;
    counter++;
  }
  *walker = v;
  if(!check_within_bounds(counter))
  {
    printf("Error map_insert: map is full!");
    return -1;
  }
  return counter; // fixa ogiltig fd (out of bounds)
}

void map_print(struct map *m)
{
  int counter = BEGIN;
  while (counter < MAP_SIZE)
  {
    if (m->content[counter] != NULL)
    {
      printf("%d->%p\n", counter, m->content[counter]);
    }
    counter++;
  }
}

// NULL returned if not found
value_t map_find(struct map *m, key_t k)
{
  if (check_within_bounds(k))
  {
    return m->content[k];
  } 
  else 
  {
    printf("\nERROR map_find: Key out of bounds!\n");
    return NULL;
  }

}

// fungerar som map_find men tar bort värdet ur samlingen.
value_t map_remove(struct map *m, key_t k)
{
  if (!check_within_bounds(k))
  {
    printf("ERROR map_remove: Key out of bounds!\n");
    return NULL;
  }
  else
  {
    if (m->content[k] != NULL)
    {
      value_t ret = m->content[k];
      m->content[k] = NULL;
      return ret;
    }
    else
    {
      puts("Value for key not allocated\n");
      return NULL;
    }
  }
}


key_t map_contains_value(struct map *m, value_t target)
{
  for (unsigned i = BEGIN; i < MAP_SIZE; i++)
  {
    if (m->content[i] == target)
    {
      //printf("Currently comparing %d == %d", m->content[i], target);
      return i;
    }
  }
  return -1;
}

/* Anropa exec för varje association i map.
 * aux skickas med som inparameter till funktionen *exec representerar. */
void map_for_each(struct map *m,
                  void (*exec)(key_t k, value_t v, int aux),
                  int aux)
{
  for (unsigned i = BEGIN; i < MAP_SIZE; i++)
  {
    if (m->content[i] != NULL)
    {
      (*exec)(i, m->content[i], aux);
    }
  }
}

// Foreach with conditional
void map_remove_if(struct map *m,
                   bool (*cond)(key_t k, value_t v, int aux),
                   int aux)
{
  for (unsigned i = BEGIN; i < MAP_SIZE; i++)
  {
    (*cond)(i, m->content[i], aux);
  }
}

bool check_within_bounds(key_t k)
{
  // Fix this  NULL comparison. 
  // if(k != NULL)
  // {
    //printf("%s", (char*)k);
  bool status = -1 < k && k < MAP_SIZE;
  if (!status)
  {
    puts("Map key out of bounds");
  }

  
  return status;
}

