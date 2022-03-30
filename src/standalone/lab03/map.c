
#include <stdlib.h>
#include "map.h"

void map_init(struct map* m)
{
  printf("Map size: %d\n", (int)sizeof(m->content)/8 );
  for(int i=0; i<MAP_SIZE; i++)
    {
      m->content[i] = NULL;
    }
}


key_t map_insert(struct map* m, value_t v)
{
  value_t* walker = m->content;
  int counter = 0;
  while( *walker != NULL )
    {
      walker++;
      counter++;
    }
  *walker = v;
  return counter;
}

void map_print(struct map* m)
{
  int counter = 0;
  while( counter < MAP_SIZE )
    {
      if( m->content[counter] == NULL )
	{
	  puts("null");
	}
      else
	{
	  printf("%s\n", m->content[counter]);
	}
      counter++;
    }
}

// NULL returned if not found
value_t map_find(struct map* m, key_t k)
{
  if( k < 0 || MAP_SIZE < k )
    {
      // TODO: put pintos error code here
      
    }
  return m->content[k];
}

// fungerar som map_find men tar bort värdet ur samlingen.
value_t map_remove(struct map* m, key_t k)
{
  if(m->content[k] != NULL)
  {
    value_t ret = m->content[k];
    printf("BOFORE FREE: %s\n", m->content[k]);
    //free(m->content[k]);
    m->content[k] = NULL;
    printf("AFTER FREE: %s\n", m->content[k]);
    return ret; 
  }
  else
  {
    return NULL;
  }
}

/* Anropa exec för varje association i map.
 * aux skickas med som inparameter till funktionen *exec representerar. */
void map_for_each(struct map* m,
		  void (*exec)(key_t k, value_t v, int aux),
		  int aux)
{
  for(int i=0; i<MAP_SIZE; i++)
  {
    if(m->content[i] != NULL)
    {
      (*exec)( i, m->content[i], aux);
    }
  }
}

// Foreach with conditional
void map_remove_if(struct map* m,
		   bool (*cond)(key_t k, value_t v, int aux),
		   int aux)
{
  for(int i=0; i<MAP_SIZE; i++)
  {
    (*cond)(i, m->content[i], aux); 
  }
}

