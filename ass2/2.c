#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[]) {
  int total_letters = 0;
  for(int word=0; word<argc; word++) {
    char* c = argv[word];
    int letters = 0;
    while( *c != '\0' ) {
      printf("%c",*c);
      letters++;
      c++;
    }
    
    int t = 20-strlen(argv[word]);
    printf("%*d\n", t, letters);
    total_letters += strlen(argv[word]);
  }
  
  printf("Total length%8d\n", total_letters); 
  printf("Average length%6.2f\n", (float)total_letters/argc);
  
  return 0;
  
}
