/* do not forget the guard against multiple includes */
#pragma once
#include <stdbool.h>
#include <stdio.h>

#define PANIC(message) do { printf("PANIC: %s\n", message); exit(1); } while(0)
// Symbolic constant for size of map
#define MAP_SIZE 128

typedef char* value_t;
typedef int key_t;

struct map
{
  value_t content[MAP_SIZE];
};

// Constructor
void map_init(struct map* m);

void map_print(struct map* m);

key_t map_insert(struct map* m, value_t v);

// NULL returned if not found
value_t map_find(struct map* m, key_t k);

// fungerar som map_find men tar bort värdet ur samlingen.
value_t map_remove(struct map* m, key_t k);

/* Anropa exec för varje association i map.
 * aux skickas med som inparameter till funktionen *exec representerar.
 */
void map_for_each(struct map* m,
		  void (*exec)(key_t k, value_t v, int aux),
		  int aux);

// Fungerar som map_for_each men exekveras baserat på om cond är sann eller ej
void map_remove_if(struct map* m,
		   bool (*cond)(key_t k, value_t v, int aux),
		   int aux);

 

