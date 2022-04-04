#define _DEFAULT_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(void)
{
  const char *dupstr = "sihtgubed";
  char *str = strdup(dupstr);
  char *stri = &str[8];
  char *buf[9];
  char **bufi;
  char **bufend;
  bufi = buf;
  bufend = &buf[9];
  
  while (bufi != bufend){
    *bufi = stri;
    bufi++;
    stri--;
  }
  
  while (bufi != buf){
    *(*bufi) -= 32;
    bufi--;
  }
  
  while (bufi != bufend){
    putchar(**bufi);
    bufi++;
  }

  putchar('\n');

  free(str);

  return 0;
}
