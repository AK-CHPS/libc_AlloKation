#include <libmAlloK.h>

int main(int argc, char *argv[])
{
  void *A = malloc(10);
  void *B = calloc(10, sizeof(char));
  A = realloc(A, 20);
  free(A);
  free(B);
  
  return 0;
}
