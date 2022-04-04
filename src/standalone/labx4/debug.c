#include <stdio.h>

int main()
{
  char str[] = "sihtgubed";
  char *stri = &str[8];
  char *buf[9];
  char **bufi;
  char **bufend;
  bufi = buf;
  bufend = &buf[8]; //buf is of size 9 but index is max 8.
  
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
    printf("%c", **bufi);
    bufi++;
  }
}
