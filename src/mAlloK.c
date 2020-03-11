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

#define SIZE_MIN_BLOCK (32 * PAGE_SIZE)

#define MIN_RESIDUAL_SIZE 500000000

#define CHUNK_SIZE      sizeof(chunk_t)

#define BLOCK_SIZE      sizeof(block_t)

#define _get_size(X)    (X->size_status & SIZE_MASK)

#define _get_status(X)  (X->size_status & STATUS_MASK)

#define _get_dirty(X)   (X->size_status & DIRTY_MASK)

#define  min(a, b)      ((a < b)? a: b)

#define  mmap_block(size)    (mmap(NULL, BLOCK_SIZE + CHUNK_SIZE + size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0))

#define  munmap_block(block)    (munmap(block, block->size + CHUNK_SIZE + BLOCK_SIZE))

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

// permet de verifier que la mémoire est aligné
static inline void check(void* ptr)
{
  if((size_t)ptr % 8 != 0){
    ////fprintf(stderr, "Adresse memoire pas alignee en %zu\n", (size_t)ptr);
    abort();
  }
}

static inline u_int64_t rdtsc (void)
{
  //64 bit variables
  u_int64_t a, d;

  __asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
  
  return (d << 32) | a;
}

// permet d'initialiser ou de modifier un block
static inline void set_block(block_t *block,             \
			     size_t   new_size,	                           \
			     chunk_t *new_stack,                           \
			     block_t* new_previous,                        \
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
			     size_t   new_size_status,	                  \
			     chunk_t *new_previous,	                      \
			     chunk_t *new_next,		                        \
			     chunk_t *new_previous_free,                  \
			     chunk_t *new_next_free,	                    \
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
}

// coupe un chunk donné en deux et retourne le nouveaux
static inline chunk_t *cut_chunk(chunk_t *chunk, size_t size)
{
    chunk_t *new_chunk = (void*)chunk + size + CHUNK_SIZE;

    if(chunk->next != NULL){
      chunk->next->previous = new_chunk;}

    set_chunk(new_chunk, (_get_size(chunk) - CHUNK_SIZE - size) | _get_dirty(chunk), chunk, chunk->next, NULL, NULL, chunk->block);

    set_chunk(chunk, size | STATUS_MASK | _get_dirty(chunk), chunk->previous, new_chunk, NULL, NULL, chunk->block);

    return new_chunk;
}

// permet de fusionner un chunk avec le chunk precedent
static inline chunk_t *merge_w_previous(chunk_t *chunk)
{
  if(chunk->previous != NULL){
    chunk->previous->next = chunk->next;
    if(chunk->next != NULL){
      chunk->next->previous = chunk->previous;}
    chunk->previous->size_status +=  (_get_size(chunk) + CHUNK_SIZE);
  }
  return chunk->previous;
}

// permet de fusionner un chunk avec le chunk suivant
static inline chunk_t *merge_w_next(chunk_t *chunk)
{
  if(chunk->next != NULL){
   if(chunk->next->next != NULL){
      chunk->next->next->previous = chunk;
    }
    chunk->size_status += (_get_size(chunk->next) + CHUNK_SIZE);
    chunk->next = chunk->next->next;
  }
  return chunk;
}


// permet d'ajouter un block a la liste
static inline block_t *add_block(block_t *block, size_t size)
{
  chunk_t *chunk = (void*)block + BLOCK_SIZE;

  // initialisation du block
  set_block(block, size, chunk, NULL, memory);

  memory = block;
  
  // si la liste est pas vide
  if(block->next != NULL){
    block->next->previous = block;
  }

  // initialisation du premier chunk
  set_chunk(chunk, size, NULL, NULL, NULL, NULL, block);

  // ajoue de la mémoire au compteur
  block_cpt+=block->size;

  return block;
}

// permet de supprimer un block particulier
static inline block_t* del_block(block_t *block)
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

  return block;
}

//recherche un chunk libre suffisament grand pour contenir la quantité de mémoire demandé
static inline  chunk_t* search_chunk(size_t size, char clean)
{
  chunk_t *ptr = memory_free_head;

  if(memory_free_tail == NULL || (memory_free_tail != NULL && _get_size(memory_free_tail) < size))
    return NULL;

  while(ptr != NULL){
    if(((clean == 1 && _get_dirty(ptr) == 0) || clean == 0) &&  _get_size(ptr) >= size){
      break;
    }
    ptr = ptr->next_free;
  }

  return ptr;
}

// allocation d'un chunk propre ou non de taille size
static inline chunk_t *alloc_chunk(size_t size, char clean)
{
  chunk_t *chunk = search_chunk(size, clean);

  if(chunk == NULL && size > SIZE_MIN_BLOCK){
      block_t *block = add_block(mmap_block(size),size);
      block->stack->size_status |= STATUS_MASK;
      return block->stack;    
  }else{
    if(chunk == NULL){
      block_t *block = add_block(mmap_block(SIZE_MIN_BLOCK),SIZE_MIN_BLOCK);
      chunk = block->stack;
    }else{
      del_free_chunk(chunk);
    }
    if(! (CHUNK_SIZE + size > _get_size(chunk) || (_get_size(chunk) - CHUNK_SIZE - size) < WORD_SIZE)){
      add_free_chunk(cut_chunk(chunk,size));
    }

    chunk->size_status |= STATUS_MASK;
    return chunk;
  }

  return NULL;
}

// libération d'un chunk
static inline void free_chunk(chunk_t *chunk)
{
  if(chunk->previous != NULL && _get_status(chunk->previous) == 0){
    del_free_chunk(chunk->previous);

    chunk = merge_w_previous(chunk);
  }
 
  if(chunk->next != NULL &&  _get_status(chunk->next) == 0){
    del_free_chunk(chunk->next);

    chunk = merge_w_next(chunk);
  }

  chunk->size_status = _get_size(chunk) | DIRTY_MASK;

  if(chunk->previous == NULL && chunk->next == NULL && block_cpt > MIN_RESIDUAL_SIZE){ 
    del_block(chunk->block);
    munmap_block(chunk->block);
  }else{
    add_free_chunk(chunk);
  }
}

void *malloc(size_t size)
{
  pthread_mutex_lock(&mutex);
  
  //fprintf(stderr, "Entree malloc %zu\n", size);

  void *to_return = NULL;

  if(size != 0){
    size = roundTo(size,WORD_SIZE);
    to_return = alloc_chunk(size,0)+1;
  }

  //fprintf(stderr, "Sortie malloc %zu\n", (size_t)to_return);

  pthread_mutex_unlock(&mutex);

  return to_return;
}

void *calloc(size_t nmemb, size_t size)
{
  pthread_mutex_lock(&mutex);
  
  //fprintf(stderr, "Entree calloc\n");

  void * to_return = NULL;

  if(size != 0 && nmemb != 0){
    size_t total_size = roundTo(size*nmemb,WORD_SIZE);

    to_return = alloc_chunk(total_size,1)+1;
  }

  //fprintf(stderr, "Sortie calloc %zu\n", (size_t)to_return);
  pthread_mutex_unlock(&mutex);

  return to_return;
}

void free(void *ptr)
{
  pthread_mutex_lock(&mutex);
  //fprintf(stderr, "Entree free %zu\n", (size_t)ptr);
  
  if(ptr != NULL){
    free_chunk(ptr-CHUNK_SIZE);
  }

  //fprintf(stderr, "Sortie free %zu\n", (size_t)ptr);
  pthread_mutex_unlock(&mutex);
}

void *realloc(void *ptr, size_t size)
{
  pthread_mutex_lock(&mutex);
  //fprintf(stderr, "Entree realloc\n");

  void *to_return = NULL;

  if(size != 0){
    size = roundTo(size,WORD_SIZE);
    chunk_t *old_chunk = (ptr != NULL) ? ptr - CHUNK_SIZE : NULL;
    size_t old_size = (ptr != NULL) ? _get_size(old_chunk) : 0;


    if(ptr != NULL && size < old_size){
      if(old_size - size < SIZE_MIN_BLOCK){
        to_return = ptr;
      }else{
        add_free_chunk(cut_chunk(old_chunk,size));
        to_return = old_chunk + 1;
      }
    }else if(ptr != NULL && old_chunk->next != NULL && _get_status(old_chunk->next) == 0 &&   \
            _get_size(old_chunk) + _get_size(old_chunk->next) + CHUNK_SIZE >= size){
      del_free_chunk(old_chunk->next);

      merge_w_next(old_chunk);
      
      to_return = old_chunk + 1;
    }
    #if HAVE_MREMAP
    else if(ptr != NULL && old_chunk->previous == NULL && old_chunk->next == NULL && HAVE_MREMAP){
      block_cpt += size - old_size;
      void *old_block_adr = ptr-CHUNK_SIZE-BLOCK_SIZE;
      size_t old_block_size = old_size + BLOCK_SIZE + CHUNK_SIZE;
      size_t new_block_size = size + CHUNK_SIZE + BLOCK_SIZE;

      del_block(old_block_adr);

      block_t *new_block = add_block(mremap(old_block_adr, old_block_size, new_block_size, MREMAP_MAYMOVE),size);

      new_block->stack->size_status |= STATUS_MASK;

      to_return = new_block->stack + 1;
    }
    #endif
    else{
      to_return = alloc_chunk(size,0)+1;
      if(ptr != NULL){
        memcpy(to_return, ptr, old_size);
        free_chunk(old_chunk);}
    }
  }else if(ptr != NULL){
    free_chunk(ptr-CHUNK_SIZE);
  }

  //fprintf(stderr, "Sortie realloc %zu\n", (size_t)to_return);
  pthread_mutex_unlock(&mutex);

  return to_return;
}