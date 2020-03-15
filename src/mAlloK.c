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
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <limits.h>
#include <dlfcn.h>

////////////////////////////////////////////////////////////////////////////

#define PAGE_SIZE                       sysconf(_SC_PAGESIZE)

#define WORD_SIZE                       __WORDSIZE/8

#define STATUS_MASK                     0x8000000000000000

#define DIRTY_MASK                      0x4000000000000000

#define SIZE_MASK                       0x3fffffffffffffff

#define SIZE_MIN_CHUNK                  (256 * 1024)

#define MIN_RESIDUAL_SIZE               (500000 * 1024)

#define LIMIT_CALLOC                    512

#define REALLOC_MIN                     (64 * WORD_SIZE + CHUNK_SIZE)

#define CHUNK_SIZE                      sizeof(chunk_t)

#define get_size(X)                     (X->size_status & SIZE_MASK)

#define get_status(X)                   (X->size_status & STATUS_MASK)

#define get_dirty(X)                    (X->size_status & DIRTY_MASK)

#define set_size(X, size)               (X->size_status = size | get_status(X) | get_dirty(X))

#define set_status(X, status)           (X->size_status = get_size(X) | status | get_dirty(X))

#define set_dirty(X, dirty)             (X->size_status = get_size(X) | get_status(X) | dirty)

#define ptr_to_chunk(ptr)               ((void*)ptr - CHUNK_SIZE)

#define chunk_to_ptr(chunk)             ((void*)chunk + CHUNK_SIZE)

#define min(a, b)                       ((a < b)? a: b)

#define mmap_chunk(size)                (mmap(NULL, CHUNK_SIZE + size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0))

#define munmap_chunk(chunk)             (munmap(chunk, get_size(chunk) + CHUNK_SIZE))

#define mremap_chunk(chunk, size)       (mremap(chunk, get_size(chunk) + CHUNK_SIZE, size + CHUNK_SIZE, MREMAP_MAYMOVE))

#define roundTo(value,roundto)          ((value + (roundto - 1)) & ~(roundto - 1))

#define f(X)                            ((X < 512) ? (X >> 3) : (log2_fast(X)+55))

#define size_small_enough(chunk, size)  (!(CHUNK_SIZE + size > get_size(chunk) || (get_size(chunk) - CHUNK_SIZE - size) < WORD_SIZE))

#define only_chunk(chunk)               (chunk != NULL && chunk->previous == NULL && chunk->next == NULL)

#define next_is_free(chunk)             (chunk != NULL && chunk->next != NULL && get_status(chunk->next) == 0)

// permet d'initialiser ou de modifier un chunk
#define set_chunk(chunk, new_size_status, new_previous, new_next, new_previous_free, new_next_free)             \
(                                                                                                               \
  chunk->size_status = new_size_status,                                                                         \
  chunk->previous = new_previous,                                                                               \
  chunk->next = new_next,                                                                                       \
  chunk->previous_free = new_previous_free,                                                                     \
  chunk->next_free = new_next_free                                                                              \
)

////////////////////////////////////////////////////////////////////////////
//                          Structures de données                         //
////////////////////////////////////////////////////////////////////////////

// structure d'un chunk
typedef struct chunk_t chunk_t;

// structure d'un chunk
struct chunk_t
{
  size_t size_status;         // taille et status du chunk

  chunk_t *previous;          // pointeur vers le chunk precedent
  chunk_t *next;              // pointeur vers le chunk suivant
  
  chunk_t *previous_free;     // pointeur vers le chunk libre precedent
  chunk_t *next_free;         // pointeur vers le chunk libre suivant
};

// table de hachage
static chunk_t *table[128] = {};

// initialisation mutex
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

////////////////////////////////////////////////////////////////////////////
//                        Gestion des chunks libres                       //
////////////////////////////////////////////////////////////////////////////

// permet de calculer le log2 de x
static size_t as_size_t(const double x) {
    return *(size_t*)&x;
}

// permet de calculer le log2 de x
static size_t log2_fast(const size_t x) {
    return (size_t)((as_size_t((double)x)>>52))-1023;
}

// ajout d'un chunk libre
static void add_free_chunk(chunk_t *chunk)
{
  size_t index = f(get_size(chunk));

  chunk->next_free = table[index];

  chunk->previous_free = NULL;

  if(table[index] != NULL){
    table[index]->previous_free = chunk;
  }

  table[index] = chunk;
}

// suppression d'un chunk libre
static void del_free_chunk(chunk_t *chunk)
{
  size_t index = f(get_size(chunk));
  
  if(table[index] == chunk){
    table[index] = chunk->next_free;
  }
  if(chunk->next_free != NULL){
    chunk->next_free->previous_free = chunk->previous_free;
  }
  if(chunk->previous_free != NULL){
    chunk->previous_free->next_free = chunk->next_free;
  }
}

//recherche un chunk libre suffisament grand pour contenir la quantité de mémoire demandé
static  chunk_t* search_chunk(const size_t size, char clean)
{
  size_t index = f(size);

  while(index < 128){
    chunk_t *chunk_ptr = table[index];
    while(chunk_ptr != NULL){
        if(((clean == 1 && get_dirty(chunk_ptr) == 0) || clean == 0) && get_size(chunk_ptr) >= size){
          return chunk_ptr;
        }
      chunk_ptr = chunk_ptr->next_free;
    }
    index++;
  }

  return NULL;
}

////////////////////////////////////////////////////////////////////////////
//                            Gestion des blocks                          //
////////////////////////////////////////////////////////////////////////////

// coupe un chunk donné en deux et retourne le nouveaux
static chunk_t *cut_chunk(chunk_t *chunk, const size_t size)
{
  chunk_t *new_chunk = (void*)chunk + size + CHUNK_SIZE;

  if(chunk->next != NULL){
    chunk->next->previous = new_chunk;}

  set_chunk(new_chunk, (get_size(chunk) - CHUNK_SIZE - size) | get_dirty(chunk), chunk, chunk->next, NULL, NULL);

  set_chunk(chunk, size | STATUS_MASK | get_dirty(chunk), chunk->previous, new_chunk, NULL, NULL);

  return new_chunk;
}

// permet de fusionner un chunk avec le chunk precedent
static chunk_t *merge_w_previous(chunk_t *chunk)
{
  chunk->previous->next = chunk->next;
  if(chunk->next != NULL){
    chunk->next->previous = chunk->previous;}

  chunk->previous->size_status +=  (get_size(chunk) + CHUNK_SIZE);

  return chunk->previous;
}

// permet de fusionner un chunk avec le chunk suivant
static chunk_t *merge_w_next(chunk_t *chunk)
{
  if(chunk->next->next != NULL){
    chunk->next->next->previous = chunk;}

  chunk->size_status += (get_size(chunk->next) + CHUNK_SIZE);
  chunk->next = chunk->next->next;

  return chunk;
}

////////////////////////////////////////////////////////////////////////////
//                     Allocation et liberation de chunks                 //
////////////////////////////////////////////////////////////////////////////

static chunk_t* alloc_chunk(chunk_t * chunk, const size_t size)
{
  if(chunk == NULL && size > SIZE_MIN_CHUNK){
    chunk = mmap_chunk(size);
    set_chunk(chunk, size | DIRTY_MASK | STATUS_MASK, NULL, NULL, NULL, NULL);
  }else{
    if(chunk == NULL){
      chunk = mmap_chunk(SIZE_MIN_CHUNK);
      set_chunk(chunk, SIZE_MIN_CHUNK, NULL, NULL, NULL, NULL);
    }else{
      del_free_chunk(chunk);
    }

    if(size_small_enough(chunk, size))  
      add_free_chunk(cut_chunk(chunk, size));

    chunk->size_status |= STATUS_MASK;
  }

  return chunk;
}

// libération d'un chunk
static void free_chunk(chunk_t *chunk)
{
  if(chunk->previous != NULL && get_status(chunk->previous) == 0){
    del_free_chunk(chunk->previous);

    chunk = merge_w_previous(chunk);
  }
 
  if(chunk->next != NULL &&  get_status(chunk->next) == 0){
    del_free_chunk(chunk->next);

    chunk = merge_w_next(chunk);
  }

  chunk->size_status = get_size(chunk) | DIRTY_MASK;

  if(only_chunk(chunk)){
    munmap_chunk(chunk);
  }else{
    add_free_chunk(chunk);
  }
}

////////////////////////////////////////////////////////////////////////////
//                            Fonctions Standards                         //
////////////////////////////////////////////////////////////////////////////

void *malloc(size_t size)
{
  pthread_mutex_lock(&mutex);
  
  void *to_return = NULL;

  if(size != 0){
    size = roundTo(size,WORD_SIZE);
    to_return = alloc_chunk(search_chunk(size,0),size) + 1;    
  }  

  pthread_mutex_unlock(&mutex);

  return to_return;
}

void *calloc(size_t nmemb, size_t size)
{
  pthread_mutex_lock(&mutex);
  
  void * to_return = NULL;

  if(size != 0 && nmemb != 0){
    size_t total_size = roundTo(size*nmemb,WORD_SIZE);

    chunk_t *chunk = NULL;

    if((chunk = search_chunk(total_size,1)) != NULL){
      to_return = chunk_to_ptr(alloc_chunk(chunk, total_size));
    }else if(total_size < LIMIT_CALLOC && (chunk = search_chunk(total_size,0)) != NULL){
      to_return = memset(chunk_to_ptr(alloc_chunk(chunk, total_size)), 0, total_size);
    }else{
      chunk_t *chunk = (SIZE_MIN_CHUNK > total_size) ? mmap_chunk(SIZE_MIN_CHUNK) : mmap_chunk(total_size);

      if(size_small_enough(chunk, total_size))  
        add_free_chunk(cut_chunk(chunk, total_size));

      set_status(chunk, STATUS_MASK);

      to_return = chunk_to_ptr(chunk);
    }
  }

  pthread_mutex_unlock(&mutex);

  return to_return;
}

void free(void *ptr)
{
  pthread_mutex_lock(&mutex);
  
  if(ptr != NULL){
    free_chunk(ptr_to_chunk(ptr));
  }

  pthread_mutex_unlock(&mutex);
}

void *realloc(void *ptr, size_t size)
{
  pthread_mutex_lock(&mutex);

  void *to_return = ptr;

  if(size != 0){
    size = roundTo(size,WORD_SIZE);
    chunk_t *old_chunk = (ptr != NULL) ? ptr_to_chunk(ptr) : NULL;
    size_t old_size = (ptr != NULL) ? get_size(old_chunk) : 0;


    if(ptr != NULL && size < old_size){
      if(old_size - size >= REALLOC_MIN)  add_free_chunk(cut_chunk(old_chunk,size));
    }else if(next_is_free(old_chunk) &&  get_size(old_chunk) + get_size(old_chunk->next) + CHUNK_SIZE >= size){
      del_free_chunk(old_chunk->next);

      merge_w_next(old_chunk);

      if(size_small_enough(old_chunk, size))  
        add_free_chunk(cut_chunk(old_chunk, size));
    }
    #if HAVE_MREMAP    
    else if(only_chunk(old_chunk) && HAVE_MREMAP){
      chunk_t *new_chunk = mremap_chunk(old_chunk, size);

      new_chunk->size_status = size + DIRTY_MASK + STATUS_MASK;

      to_return = chunk_to_ptr(new_chunk);
    }
    #endif
    else{
      to_return = alloc_chunk(search_chunk(size,0),size)+1;
      if(ptr != NULL){
        memcpy(to_return, ptr, old_size);
        free_chunk(old_chunk);}
    }
  }else if(ptr != NULL){
    free_chunk(ptr-CHUNK_SIZE);
  }

  pthread_mutex_unlock(&mutex);

  return to_return;
}