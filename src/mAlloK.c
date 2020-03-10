//#include "malloc.h"
#include <stddef.h>
//#include <stdio.h>

#ifdef linux
  #define _GNU_SOURCE
  #define HAVE_MREMAP 1
#else
  #define HAVE_MREMAP 0
#endif

///////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <limits.h>

////////////////////////////////////////////////////////////////////////////

#define PAGE_SIZE sysconf(_SC_PAGESIZE)

#define WORD_SIZE __WORDSIZE/8

#define STATUS_MASK 0x8000000000000000

#define DIRTY_MASK 0x4000000000000000

#define SIZE_MASK 0x3fffffffffffffff

#ifndef SIZE_MIN_BLOCK
   #define SIZE_MIN_BLOCK (32 * PAGE_SIZE) // 32 * PAGE_SIZE ?
#endif

#ifndef MIN_RESIDUAL_SIZE
   #define MIN_RESIDUAL_SIZE 500000000 // 32 * PAGE_SIZE * PAGE_SIZE = 500Mo
#endif

#define _get_size(X)  (X->size_status & SIZE_MASK)

#define _get_status(X)  (X->size_status & STATUS_MASK)

#define _get_dirty(X) (X->size_status & DIRTY_MASK)

////////////////////////////////////////////////////////////////////////////

// structure d'un block
typedef struct block_t block_t;

// structure d'un chunk
typedef struct chunk_t chunk_t;

// structure d'un block
struct block_t
{
  size_t size;                // taille du block

  chunk_t *stack;             // liste de chunk du block

  block_t *previous;          // pointeur vers le block precedent
  block_t *next;              // pointeur vers le block suivant
};

// structure d'un chunk
struct chunk_t
{
  size_t size_status;         // taille et status du chunk

  chunk_t *previous;          // pointeur vers le chunk precedent
  chunk_t *next;              // pointeur vers le chunk suivant
  
  chunk_t *previous_free;     // pointeur vers le chunk libre precedent
  chunk_t *next_free;         // pointeur vers le chunk libre suivant

  block_t *block;             // pointeur vers le block du chunk
};

// liste de block représentant la mémoire
static block_t *memory = NULL;

// liste de chunk représentant la mémoire libre
static chunk_t *memory_free_head = NULL;

static chunk_t *memory_free_tail = NULL;

// quantité de mémoire allouée
static size_t allocated_memory = 0;

// quantité de mémoire alloué dans les block
static size_t block_cpt = 0;

// initialisation mutex
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

////////////////////////////////////////////////////////////////////////////

// permet d'arrondir au multiple de roundTo superieur
static inline size_t roundTo(size_t value, size_t roundTo)
{
    return (value + (roundTo - 1)) & ~(roundTo - 1);
}

// permet d'initialiser ou de modifier un block
static inline void set_block(block_t *block,            \
			     size_t   new_size,	        \
			     chunk_t *new_stack,        \
			     block_t* new_previous,     \
			     block_t *new_next          )
{
  block->size = new_size;
  block->stack = new_stack;
  block->previous = new_previous;
  block->next = new_next;

  return ;
}

//permet d'initialiser ou de modifier un chunk
static inline void set_chunk(chunk_t *chunk,            \
			     size_t   new_size_status,	\
			     chunk_t *new_previous,	\
			     chunk_t *new_next,		\
			     chunk_t *new_previous_free,\
			     chunk_t *new_next_free,	\
			     block_t *new_block         )
{
  chunk->size_status = new_size_status;
  chunk->previous = new_previous;
  chunk->next = new_next;
  chunk->previous_free = new_previous_free;
  chunk->next_free = new_next_free;
  chunk->block = new_block;

  return;
}


// ajout d'un chunk libre
static inline void add_free_chunk(chunk_t *chunk)
{
    if(memory_free_head == NULL){
        memory_free_head = chunk; 
        memory_free_tail = chunk;                    
    }else if( _get_size(memory_free_head) >= _get_size(chunk)) { 
        chunk->next_free = memory_free_head;
        memory_free_head->previous_free = chunk;
        memory_free_head = chunk;
    }else{
        chunk_t *current = memory_free_head; 

        while(current->next_free != NULL && _get_size(current->next_free) < _get_size(chunk)){ 
            current = current->next_free;}
  
        chunk->next_free = current->next_free; 

        if(current->next_free != NULL) 
            chunk->next_free->previous_free = chunk; 

        if(current == memory_free_tail)
          memory_free_tail = chunk;
  
        current->next_free = chunk; 
        chunk->previous_free = current; 
    }

    return ;
}

// suppression d'un chunk libre
static inline void del_free_chunk(chunk_t *chunk)
{
  if(memory_free_head == chunk){
    memory_free_head = chunk->next_free;
  }
  if(memory_free_tail == chunk){
    memory_free_tail = chunk->previous_free;
  }
  if(chunk->next_free != NULL){
    chunk->next_free->previous_free = chunk->previous_free;
  }
  if(chunk->previous_free != NULL){
    chunk->previous_free->next_free = chunk->next_free;
  }

  return ;
}

// permet d'ajouter un block a la liste
static block_t *add_block(size_t size)
{
  // allocation de la mémoire
  void *ptr = mmap(NULL, sizeof(block_t) + sizeof(chunk_t) + size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  // cast en chunk et en block
  block_t *block = ptr;
  chunk_t *chunk = ptr + sizeof(block_t);

  // initialisation du block
  set_block(block, size, chunk, NULL, memory);

  memory = block;
  
  // si la liste est pas vide
  if(block->next != NULL){
    block->next->previous = block;
  }

  // initialisation du premier chunk
  set_chunk(chunk, size, NULL, NULL, NULL, NULL, block);

  add_free_chunk(chunk);

  // ajoue de la mémoire au compteur
  block_cpt+=block->size;

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

  // supression de la mémoire au compteur
  block_cpt -= block->size;

  munmap(block, block->size+sizeof(chunk_t)+sizeof(block_t));

  return ;
}

////////////////////////////////////////////////////////////////////////////

// permet d'allouer un chunk
static chunk_t *alloc_chunk(chunk_t *chunk, size_t size)
{
  del_free_chunk(chunk);
 
  if(_get_size(chunk) - size <= sizeof(chunk_t)){
    chunk->size_status |= STATUS_MASK;
    chunk->size_status |= DIRTY_MASK;
  }else{
    chunk_t *new_chunk = (void*)chunk + size + sizeof(chunk_t);

    if(chunk->next != NULL){
      chunk->next->previous = new_chunk;}

  set_chunk(new_chunk, _get_size(chunk) - (size + sizeof(chunk_t)) | _get_dirty(chunk), chunk, chunk->next, NULL, NULL, chunk->block);

  add_free_chunk(new_chunk); 

  set_chunk(chunk, size | STATUS_MASK | DIRTY_MASK, chunk->previous, new_chunk, chunk->previous_free, chunk->next_free, chunk->block);
  }

    return chunk;
}

// permet de liberer un chunk 
static void free_chunk(chunk_t *chunk)
{
  chunk->size_status &= SIZE_MASK;

  if(chunk->previous != NULL && _get_status(chunk->previous) == 0){
    del_free_chunk(chunk->previous);

   chunk->previous->next = chunk->next;
    if(chunk->next != NULL){
      chunk->next->previous = chunk->previous;}
    chunk->previous->size_status +=  _get_size(chunk) + sizeof(chunk_t);
    chunk = chunk->previous;
  }
 
  if(chunk->next != NULL &&  _get_status(chunk->next) == 0){
    del_free_chunk(chunk->next);

   if(chunk->next->next != NULL){
      chunk->next->next->previous = chunk;
    }
    chunk->size_status += _get_size(chunk->next) + sizeof(chunk_t);
    chunk->next = chunk->next->next;
  }

  chunk->size_status |= DIRTY_MASK;

  if(chunk->previous == NULL && chunk->next == NULL && block_cpt > MIN_RESIDUAL_SIZE){
    del_block(chunk->block);
  }else{
    add_free_chunk(chunk);
  }

  return ;
}

//recherche un chunk libre suffisament grand pour contenir la quantité de mémoire demandé
static inline chunk_t* search_chunk(size_t size, char clean)
{
  chunk_t *ptr = memory_free_head;

  if(memory_free_tail == NULL || (memory_free_tail != NULL && _get_size(memory_free_tail) < size))
    return NULL;

  while(ptr != NULL){
    if(((clean == 1 && _get_dirty(ptr) == 0) || clean == 0) &&  _get_size(ptr) >= size){
      return ptr;}
    ptr = ptr->next_free;
  }

  return NULL;
}

////////////////////////////////////////////////////////////////////////////

void *malloc(size_t size)
{
  pthread_mutex_lock(&mutex);

  size = roundTo(size,WORD_SIZE);

  if(size == 0){
    return NULL;}

  chunk_t *ptr = search_chunk(size, 0);

  if(ptr != NULL){
    return alloc_chunk(ptr, size)+1;
  }else{
    block_t *block;

    if(size < SIZE_MIN_BLOCK){
      block = add_block(SIZE_MIN_BLOCK);
    }else{
      block = add_block(size);
    }

    return alloc_chunk(block->stack, size)+1;
  }

  pthread_mutex_unlock(&mutex);

  return NULL;
}

void free(void *ptr)
{
  pthread_mutex_lock(&mutex);

  free_chunk(ptr-sizeof(chunk_t));

  pthread_mutex_unlock(&mutex);
}

void *calloc(size_t nmemb, size_t size)
{
  pthread_mutex_lock(&mutex);

  if(size == 0){
    return NULL;}

  size_t total_size = roundTo(size*nmemb,WORD_SIZE);

  chunk_t *ptr = search_chunk(total_size, 1);

  if(ptr != NULL){
    return alloc_chunk(ptr, total_size)+1;
  }else{
      block_t *block;

      if(total_size < SIZE_MIN_BLOCK){
        block = add_block(SIZE_MIN_BLOCK);
      }else{
        block = add_block(total_size);
      }

      return alloc_chunk(block->stack, total_size)+1;
  }

    pthread_mutex_unlock(&mutex);
  
  return NULL;
}

void *realloc(void *ptr, size_t size)
{
  pthread_mutex_lock(&mutex);

  if(size == 0){
    return NULL;}

  size = roundTo(size,WORD_SIZE);

  chunk_t *old_chunk = ptr - sizeof(chunk_t);

  size_t old_size = _get_size(old_chunk);

    if(size < old_size){
      if(old_size - size < SIZE_MIN_BLOCK){
        return ptr;
      }else{
        old_chunk->size_status &= DIRTY_MASK + SIZE_MASK;
        add_free_chunk(old_chunk);

        return alloc_chunk(old_chunk,size)+1;
      }
    }else if(old_chunk->next != NULL && _get_status(old_chunk->next) == 0 && \
          _get_size(old_chunk) + _get_size(old_chunk->next) + sizeof(chunk_t) >= size){

      del_free_chunk(old_chunk->next);

      if(old_chunk->next->next != NULL){
        old_chunk->next->next->previous = old_chunk;}

      old_chunk->size_status = _get_size(old_chunk) + _get_size(old_chunk->next) + sizeof(chunk_t);
      old_chunk->size_status |= _get_dirty(old_chunk->next);
      old_chunk->next = old_chunk->next->next;

      add_free_chunk(old_chunk);

      return alloc_chunk(old_chunk,size)+1;
    }

    #if HAVE_MREMAP

      else if(old_chunk->previous == NULL && old_chunk->next == NULL && HAVE_MREMAP){
        void *old_addr = ptr-sizeof(chunk_t)-sizeof(block_t);

        block_cpt -= old_size;

        void *new_addr = mremap(old_addr, old_size + sizeof(block_t) + sizeof(chunk_t), size + sizeof(chunk_t) + sizeof(block_t), MREMAP_MAYMOVE);

        block_cpt += size;

        block_t *new_block = new_addr;
        new_block->size = size;
        new_block->stack = new_addr + sizeof(block_t);

        if(new_block->previous != NULL){
          new_block->previous->next = new_addr;
        }
        if(new_block->next != NULL){
          new_block->next->previous = new_addr;
        }
        if(old_addr == memory){
          memory = new_addr;
        }

        chunk_t *new_chunk = new_addr + sizeof(block_t);

        new_chunk->size_status = size | STATUS_MASK | DIRTY_MASK;
        new_chunk->next = new_chunk->previous = NULL;
        new_chunk->next_free = new_chunk->previous_free = NULL;
        new_chunk->block = new_block;

        return new_addr + sizeof(block_t) + sizeof(chunk_t);
      }

    #endif

    else{
      void *new_ptr = malloc(size);

      memcpy(new_ptr, ptr, old_size);
    
      free_chunk(old_chunk);

      return new_ptr;
    }

    pthread_mutex_unlock(&mutex);

  return NULL;
}

void print_memory(const char *allocated_file, const char *free_file)
{
  static size_t instant = 0;

  FILE *f_alloc = fopen(allocated_file,"a");
  FILE *f_free = fopen(free_file,"a");

  block_t *block_ptr = memory;
  while(block_ptr != NULL){
    chunk_t *chunk_ptr = block_ptr->stack;
    while(chunk_ptr != NULL){
      void *original_ptr = chunk_ptr;
      
      for(void* ptr = chunk_ptr; ptr < (original_ptr + sizeof(chunk_t)+ _get_size(chunk_ptr)); ptr+=1024){
        if(_get_status(chunk_ptr)){
          fprintf(f_alloc, "%zu %p\n ", instant, ptr);
        }else{
          fprintf(f_free, "%zu %p\n ", instant, ptr);
        }

      } 
      chunk_ptr = chunk_ptr->next;
    }
    block_ptr = block_ptr->next;
  }

  fclose(f_free);
  fclose(f_alloc);

  instant++;
}

/* void __attribute__((constructor)) constructor() */
/* { */
/*   add_block(SIZE_MIN_BLOCK); */
/* } */

/* void __attribute__((destructor)) destructor() */
/* { */
/*   block_t *ptr = memory, *old_ptr = NULL; */

/*   while(ptr != NULL){ */
/*     old_ptr = ptr; */
/*     ptr = ptr->next; */
/*     del_block(old_ptr); */
/*   } */
/* } */
