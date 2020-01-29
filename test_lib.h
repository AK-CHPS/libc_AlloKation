#ifndef test_lib_h
#define test_lib_h

#include <malloc.h>

void* my_malloc(size_t s)
{
  printf("\nmalloc\n");
}

void my_free(void *pt)
{
  printf("\nfree\n");
}


void *(*malloc_ptr)(size_t) = my_malloc;
void (*free_ptr)(void *) = my_free;
#define malloc(s) (*malloc_ptr)(s)
#define free(s) (*free_ptr)(s)

#endif
