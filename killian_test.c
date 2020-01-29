#include <test_lib.h>

#include <stdlib.h>
#include <stdio.h>


int main(int argc, char const *argv[])
{

  int *A = malloc(4);

  free(A);
  
  return 0;
}

