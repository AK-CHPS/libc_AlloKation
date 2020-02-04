//#include "mAlloK.h"
#include <stddef.h>
//#include <stdio.h>
///////////////////////////////////////////////////////////////////////////
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

////////////////////////////////////////////////////////////////////////////
static const u_int64_t status_mask = 0x8000000000000000;
static const u_int64_t size_mask = 0x7fffffffffffffff;
typedef struct chunk_t
{
	struct chunk_t *prev;
	struct chunk_t *next;
	struct chunk_t *tab_next;
	size_t size_status;
}chunk_t;
static chunk_t *size_tab[218] = {};
////////////////////////////////////////////////////////////////////////////
static void add_chunk(chunk_t *ptr);
static void del_chunk(chunk_t *ptr);
static void print_tab();
static chunk_t *search_size(size_t size);
static chunk_t *add_block(size_t size);
static void del_block(chunk_t *chunk);
////////////////////////////////////////////////////////////////////////////

void *mAlloK(size_t size)
{
  printf("\n OUR MALLOC !!!!!!\n");
  chunk_t *ptr = search_size(size+sizeof(chunk_t));
  
  if(ptr == NULL)
    {
      ptr = add_block(size);
      ptr->size_status |= status_mask;
      add_chunk(ptr);
      return ptr+1;
    }
  else
    {
      if(ptr->size_status - (sizeof(chunk_t)+size) < sizeof(chunk_t))
	{
	  ptr->size_status |= status_mask;
	  return ptr+1;
	}
      else
	{
	  del_chunk(ptr);
	  chunk_t *new_chunk = (chunk_t*)((void*)ptr + size + sizeof(chunk_t));
	  new_chunk->size_status = ptr->size_status - size - sizeof(chunk_t);
	  new_chunk->prev = ptr;
	  new_chunk->next = ptr->next;
	  ptr->next = new_chunk;
	  ptr->size_status = size | status_mask;
	  add_chunk(ptr);
	  add_chunk(new_chunk);
	  return ptr+1;
	}
    }
  
  return NULL;
}

void freeAK(void *ptr)
{
  printf("\n OUR FREE !!!!!!\n");
  chunk_t *chunk = ptr-sizeof(chunk_t);

  chunk->size_status &= size_mask;
  del_chunk(chunk);

  if(chunk->prev != NULL && (chunk->prev->size_status & status_mask))
    {
      del_chunk(chunk);
      del_chunk(chunk->prev);
      chunk->prev->next = chunk->next;
      chunk->prev->size_status += chunk->size_status + sizeof(chunk_t);
      add_chunk(chunk->prev);
      chunk = chunk->prev;
    }
  if(chunk->next != NULL && (chunk->next->size_status & status_mask))
    {
      del_chunk(chunk);
      del_chunk(chunk->next);
      chunk->next = chunk->next->next;
      chunk->size_status += chunk->next->size_status + sizeof(chunk_t);
      add_chunk(chunk);
    }
}

void *cAlloK(size_t nmemb, size_t size)
{
  printf("\n OUR CALLOC !!!!!!\n");
  void *ptr = mAlloK(size*nmemb);
  memset(ptr, 0, size*nmemb);
  return ptr;
}

void *reAlloK(void *ptr, size_t size)
{
  printf("\n OUR REALLOC !!!!!!\n");
  void *old_ptr = ptr;
  chunk_t *old_chunk = ptr-sizeof(chunk_t);
  size_t old_size = old_chunk->size_status & size_mask;
	
  freeAK(ptr);
  ptr = mAlloK(size);
  if(ptr != old_ptr)
    memcpy(ptr,old_ptr,old_size);

  return ptr;
}
  




/////////////////////////////////////////////////////////////////////

static inline int size_hachage(size_t size)
{
  if(size >= 512)
    {
      return log2(size+1)*7; /* OK */
    }
  else
    {
      return size>>3;	/* OK */
    }
}

static void add_chunk(chunk_t *ptr)
{
  const int index = size_hachage(ptr->size_status & size_mask);
  ptr->tab_next = size_tab[index];
  size_tab[index] = ptr;
}

static void del_chunk(chunk_t *ptr)
{
  const int index = size_hachage(ptr->size_status & size_mask);

  chunk_t *tmp = size_tab[index];

  if(tmp == ptr)
    {
      size_tab[index] = tmp->tab_next;
      tmp->tab_next = NULL;
      return;
    }
  while(tmp->tab_next != ptr && tmp != NULL)
    {
      tmp = tmp->tab_next;
    }
  if(tmp != NULL)
    {
      chunk_t *old_next = tmp->tab_next;
      tmp->tab_next = tmp->tab_next->tab_next;
      old_next->tab_next = NULL;
    }
}

static void print_tab()
{
  printf("------------------ Print Tab --------------------\n");
  for(int i = 0; i < 218; i++)
    {
      chunk_t *ptr = size_tab[i];
      if(ptr != NULL)
	printf("size_tab[%d]: \n", i);
      
      while(ptr != NULL)
	{
	  printf("\t\t|\n");
	  printf("previous address: %zu\n", ptr->prev);
	  printf("address: %zu\n", ptr);
	  printf("next address: %zu\n", ptr->next);
	  printf("size : %zu status: %zu\n", ptr->size_status & size_mask, ptr->size_status & status_mask);
	  ptr = ptr->tab_next;
	}
  }
}


static chunk_t *search_size(size_t size)
{
  const int index = size_hachage(size);
  
  for(int i = index; i < 218; i++)
    {
      chunk_t *ptr = size_tab[i];
      while(ptr != NULL)
	{
	  if((ptr->size_status & status_mask) == 0)
	    {
	      return ptr;
	    }
	  ptr = ptr->tab_next;
	}
    }
  return NULL;
}

static chunk_t *add_block(size_t size)
{
  chunk_t *chunk = mmap(0, sizeof(chunk_t)+size, PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  chunk->prev = NULL;
  chunk->size_status = size;
  chunk->next = NULL;
  return chunk;
}

static void del_block(chunk_t *chunk)
{
  munmap(chunk, sizeof(chunk)+ (chunk->size_status & size_mask));
}
