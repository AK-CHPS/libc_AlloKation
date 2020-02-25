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

// mask pour obtenir le status
static const size_t status_mask = 0x8000000000000000;
// mask pour obtenir la taille
static const size_t size_mask = 0x7fffffffffffffff;

// structure d'un block
typedef struct block_t block_t;

// structure d'un chunk
typedef struct chunk_t chunk_t;

// structre d'un block
struct block_t
{
  size_t size;
  void *first_address;
  void *last_address;
  chunk_t *head;
  chunk_t *tail;
  block_t *previous;
  block_t *next;
};

// structure d'un chunk
struct chunk_t
{
  size_t size_status;
  chunk_t *previous;
  chunk_t *next;
  block_t *block;
};

// taille minimum d'un mmap
#define SIZE_MIN_BLOCK (128*1024)

// liste de block
static block_t *memory = NULL;

// min macro
#define min(a,b) ((a < b) ? a : b)

////////////////////////////////////////////////////////////////////////////

// permet d'ajouter un block a la liste
static block_t * add_block(size_t size)
{
  // allocation de la mémoire
  void *ptr = mmap(0, sizeof(block_t) + sizeof(chunk_t) + size, PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  
  // cast en chunk et en block
  block_t *block = ptr;
  chunk_t *chunk = ptr+sizeof(block_t);

  // initialisation du block
  block->first_address = ptr+sizeof(block_t)+sizeof(chunk_t);
  block->last_address = ptr+sizeof(block_t) + sizeof(chunk_t) + size;
  block->size = size;
  block->next = memory;
  block->previous = NULL;
  block->head = chunk;
  block->tail = chunk;
  memory = block;
  
  // si la liste est pas vide et sinon
  if(block->next != NULL){
    block->next->previous = block;
  }

  // initialisation du premier chunk
  chunk->size_status = size;
  chunk->next = NULL;
  chunk->previous = NULL;
  chunk->block = block;

  return ptr;
}

// permet de supprimer un block particulier
static void del_block(block_t *block)
{
  if(memory == block){
    memory = block->next;
  }
  if(block->previous != NULL){
    block->previous->next = block->next;
  }
  if(block->next != NULL){
    block->next->previous = block->previous;
  }

  munmap(block,block->size+sizeof(chunk_t)+sizeof(block_t));

  return ;
}

////////////////////////////////////////////////////////////////////////////

// permet d'allouer un chunk
static chunk_t * alloc_chunk(chunk_t * chunk, size_t size)
{
  if(chunk != NULL && (chunk->size_status >> 63) == 0 && size <= chunk->size_status){
    if(chunk->size_status - size <= sizeof(chunk_t)){
      chunk->size_status |= status_mask;

      return chunk;
    }else{
      chunk->size_status -= size + sizeof(chunk_t);
      void *new_address = (void*)chunk + chunk->size_status + sizeof(chunk_t);
      chunk_t *new_chunk = new_address;
      new_chunk->next = chunk->next;

      if(chunk->block->tail == chunk){
        chunk->block->tail = new_chunk;
      }

      if(chunk->next != NULL){
        chunk->next->previous = new_chunk;
      }     
      
      chunk->next = new_chunk;
      new_chunk->previous = chunk;
      new_chunk->size_status = size + status_mask;
      new_chunk->block = chunk->block;

      return new_chunk;
    }
  }

  return NULL;
}

// permet de liberer un chunk 
static void free_chunk(chunk_t *chunk)
{
  if(chunk != NULL && chunk->size_status >> 63 != 0){
    chunk->size_status &= size_mask;

    if(chunk->previous != NULL && chunk->previous->size_status >> 63 == 0){
      if(chunk->block->tail == chunk){
        chunk->block->tail = chunk->previous;
      }

      chunk->previous->next = chunk->next;
      if(chunk->next != NULL){
        chunk->next->previous = chunk->previous;}
      chunk->previous->size_status += (chunk->size_status + sizeof(chunk_t));
      chunk = chunk->previous;
    }

    if(chunk->next != NULL && chunk->next->size_status >> 63 == 0){
      if(chunk->block->tail ==  chunk->next){
        chunk->block->tail = chunk;
      }

      if(chunk->next->next != NULL){
        chunk->next->next->previous = chunk;
      }
      chunk->size_status += chunk->next->size_status + sizeof(chunk_t);
      chunk->next = chunk->next->next;
    }
  }
  return;
}

// retourne le premier chunk libre de taille superieur ou égale à size
static chunk_t * search_chunk(size_t size)
{
  block_t *ptr = memory;

  while(ptr != NULL){
    if(((ptr->head->size_status & size_mask) >= size)  && (ptr->head->size_status >> 63 == 0)){
      return ptr->head;
    }else if(((ptr->tail->size_status & size_mask) >= size)  && (ptr->tail->size_status >> 63 == 0)){
      return ptr->tail;
    }
    ptr = ptr->next;
  }
  return NULL;
}

////////////////////////////////////////////////////////////////////////////

void *mAlloK(size_t size)
{
  chunk_t *ptr = search_chunk(size);

  if(ptr != NULL){
    return alloc_chunk(ptr,size)+1;
  }else{
    block_t *block;

    if(size < SIZE_MIN_BLOCK){
      block = add_block(SIZE_MIN_BLOCK);
    }else{
      block = add_block(size);
    }

    return alloc_chunk(block->head,size)+1;
  }

  return NULL;
}

void freeAK(void *ptr)
{
  free_chunk(ptr-sizeof(chunk_t));
}

void *cAlloK(size_t nmemb, size_t size)
{
  void *adr = mAlloK(nmemb*size);

  return memset(adr, 0, nmemb*size);
}

void *reAlloK(void *ptr, size_t size)
{
  chunk_t *old_chunk = ptr - sizeof(chunk_t);

  size_t old_size = old_chunk->size_status & size_mask;
  free_chunk(old_chunk);

  void *new_ptr = mAlloK(size);

  return memcpy(new_ptr, ptr, min(size,old_size));
}