#include "./headers/preload.h"
#include <sys/types.h>

void *malloc(size_t size)
{
  return mAlloK(size);
}

void *calloc(size_t nmemb, size_t size)
{
  return cAlloK(nmemb, size);
}

void free(void *ptr)
{
  return freeAK(ptr);
}

void *realloc(void *ptr, size_t size)
{
  return reAlloK(ptr, size);
}
