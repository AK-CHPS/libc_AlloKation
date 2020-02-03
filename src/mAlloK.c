#include "mAlloK.h"
#include <stddef.h>
#include <stdio.h>

void *mAlloK(size_t size)
{
  printf("test : malloc succefully replaced by mAlloK\n");

  return NULL;
}

void freeAK(void *ptr)
{
  printf("test : free succefully replaced by freeAK\n");
}

void *cAlloK(size_t nmemb, size_t size)
{
  printf("test : calloc succefully replaced by cAlloK\n");

  return NULL;
}

void *reAlloK(void *ptr, size_t size)
{
  printf("test : realloc succefully replaced by reAlloK\n");

  return NULL;
}
  
