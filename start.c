#include <stdio.h>
#include <stdlib.h>  

  void Start(void*);

  int main(int argc, char **argv) {
      int * heap=(int*)malloc(sizeof(int)*10000);
      Start(heap);
      free(heap);
      return 0;
  }
