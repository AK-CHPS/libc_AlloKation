#ifndef _libmAlloK_h
#define _libmAlloK_h

#include <stdlib.h>
#include <malloc.h>

extern void *mAlloK(size_t);
extern void freeAK(void *);
extern void *cAlloK(size_t, size_t);
extern void *reAlloK(void *, size_t);

#define malloc(size) (*mAlloK)(size)
#define free(ptr) (*freeAK)(ptr)
#define calloc(nmemb, size) (*cAlloK)(nmemb, size)
#define realloc(ptr, size) (*reAlloK)(ptr, size)

#endif
