#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

#define NEW_VERSION
#ifndef N
	#define N 1000
#endif

#ifndef WARN
	#define WARN 1
#endif

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
  size_t size;					// taille du block

  chunk_t *stack;				// liste de chunk du block

  block_t *previous;			// pointeur vers le block precedent
  block_t *next;				// pointeur vers le block suivant
};

// structure d'un chunk
struct chunk_t
{
  size_t size_status;			// taille et status du chunk

  chunk_t *previous;			// pointeur vers le chunk precedent
  chunk_t *next;				// pointeur vers le chunk suivant
  
  chunk_t *previous_free;		// pointeur vers le chunk libre precedent
  chunk_t *next_free;			// pointeur vers le chunk libre suivant

  block_t *block;				// pointeur vers le block du chunk
};

// taille minimum d'un mmap
#ifndef SIZE_MIN_BLOCK
	#define SIZE_MIN_BLOCK (128*1024)
#endif

// strategie d'allocation
#define BEST_FIT 1
#define WORST_FIT 2
#define FIRST_FIT 3

#ifndef STRATEGY
	#define STRATEGY FIRST_FIT
#endif

// liste de block
static block_t *memory = NULL;
static chunk_t *memory_free = NULL;

// compteur de mémoire alloué
static size_t allocated_memory = 0;

// min macro
#define min(a,b) ((a < b) ? a : b)

// initialisation mutex
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

////////////////////////////////////////////////////////////////////////////

// ajout d'un chunk libre
static inline void add_free_chunk(chunk_t *chunk)
{
	if(memory_free != NULL){
		memory_free->previous_free = chunk;}

	chunk->next_free = memory_free;
	chunk->previous_free = NULL;
	memory_free = chunk;
}

// suppression d'un chunk libre
static inline void del_free_chunk(chunk_t *chunk)
{
	if(memory_free == chunk){
		memory_free = chunk->next_free;
	}
	if(chunk->next_free != NULL){
		chunk->next_free->previous_free = chunk->previous_free;
	}
	if(chunk->previous_free != NULL){
		chunk->previous_free->next_free = chunk->next_free;
	}
}

// permet d'ajouter un block a la liste
static block_t * add_block(size_t size)
{
  // allocation de la mémoire
  void *ptr = mmap(0, sizeof(block_t) + sizeof(chunk_t) + size, PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  
  // ajout de allocated memory
  allocated_memory += size;

  // cast en chunk et en block
  block_t *block = ptr;
  chunk_t *chunk = ptr+sizeof(block_t);

  // initialisation du block
  block->size = size;
  block->next = memory;
  block->previous = NULL;

  block->stack = chunk;

  memory = block;
  
  // si la liste est pas vide et sinon
  if(block->next != NULL){
    block->next->previous = block;
  }

  // initialisation du premier chunk
  chunk->size_status = size;
  chunk->previous_free = NULL;
  chunk->next_free = NULL;
  chunk->next = NULL;
  chunk->previous = NULL;
  chunk->block = block;

  add_free_chunk(chunk);

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

  allocated_memory -= block->size;

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

      del_free_chunk(chunk);

      return chunk;
    }else{
      chunk->size_status -= size + sizeof(chunk_t);
      void *new_address = (void*)chunk + chunk->size_status + sizeof(chunk_t);
      chunk_t *new_chunk = new_address;
      new_chunk->next = chunk->next;

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
      del_free_chunk(chunk->previous);

      chunk->previous->next = chunk->next;
      if(chunk->next != NULL){
        chunk->next->previous = chunk->previous;}
      chunk->previous->size_status += (chunk->size_status + sizeof(chunk_t));
      chunk = chunk->previous;
    }

    if(chunk->next != NULL && chunk->next->size_status >> 63 == 0){
      del_free_chunk(chunk->next);

      if(chunk->next->next != NULL){
        chunk->next->next->previous = chunk;
      }
      chunk->size_status += chunk->next->size_status + sizeof(chunk_t);
      chunk->next = chunk->next->next;
    }
  }

  if(chunk->previous == NULL && chunk->next == NULL){
    del_block(chunk->block);
  }else{
  	add_free_chunk(chunk);
  }

  return;
}

#if STRATEGY == BEST_FIT

	// retourne le plus petit block suffisament grand pour contenir la quantité de mémoire demandé
	static inline chunk_t* search_chunk(size_t size)
	{
		chunk_t *to_return = NULL, *ptr = memory_free;
		size_t chunk_size = 0xFFFFFFFFFFFFFFFF;

		while(ptr != NULL){
			if((ptr->size_status & size_mask) < chunk_size && (ptr->size_status & size_mask) >= size){
				to_return = ptr;
				chunk_size = (ptr->size_status & size_mask);}
			ptr = ptr->next_free;
		}

		return to_return;
	}

#elif STRATEGY == WORST_FIT

	// retourne le plus gros block suffisament grand pour contenir la quantité de mémoire demandé
	static inline chunk_t* search_chunk(size_t size)
	{
		chunk_t *to_return = NULL, *ptr = memory_free;
		size_t chunk_size = 0;

		while(ptr != NULL){
			if((ptr->size_status & size_mask) > chunk_size && (ptr->size_status & size_mask) >= size){
				to_return = ptr;
				chunk_size = (ptr->size_status & size_mask);}
			ptr = ptr->next_free;
		}

		return to_return;
	}

#else

	// retourne le premier block suffisament grand pour contenir la quantité de mémoire demandé
	static inline chunk_t* search_chunk(size_t size)
	{
		chunk_t *ptr = memory_free;

		while(ptr != NULL){
			if((ptr->size_status & size_mask) >= size){
				return ptr;}
			ptr = ptr->next_free;
		}

		return NULL;
	}

#endif

////////////////////////////////////////////////////////////////////////////

void *my_malloc(size_t size)
{
  pthread_mutex_lock(&mutex);

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

    return alloc_chunk(block->stack,size)+1;
  }

  pthread_mutex_unlock(&mutex);

  return NULL;
}

void my_free(void *ptr)
{
  pthread_mutex_lock(&mutex);

  free_chunk(ptr-sizeof(chunk_t));

  pthread_mutex_unlock(&mutex);
}

void *my_calloc(size_t nmemb, size_t size)
{
  pthread_mutex_lock(&mutex);

  void *adr = my_malloc(nmemb*size);

  adr = memset(adr, 0, nmemb*size);

  pthread_mutex_unlock(&mutex);
  
  return adr;
}

void *my_realloc(void *ptr, size_t size)
{
  pthread_mutex_lock(&mutex);

  chunk_t *old_chunk = ptr - sizeof(chunk_t);

  size_t old_size = old_chunk->size_status & size_mask;

  void *new_ptr = my_malloc(size);

  memcpy(new_ptr, ptr, min(size,old_size));

  free_chunk(old_chunk);

  pthread_mutex_unlock(&mutex);

  return new_ptr;
}

size_t rand_a_b(size_t a, size_t b)
{
	return rand()%(b-a)+a;
}

static inline u_int64_t rdtsc (void)
{
  //64 bit variables
  u_int64_t a, d;

  __asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
  
  return (d << 32) | a;
}

static void print_memory()
{
	static size_t instant = 0;

	FILE *f_alloc = fopen("dat/allocated_memory.dat","a");
	FILE *f_free = fopen("dat/free_memory.dat","a");
	FILE *f_list_free = fopen("dat/free_list_memory.dat","a");

	block_t *block_ptr = memory;
	while(block_ptr != NULL){
		chunk_t *chunk_ptr = block_ptr->stack;
		while(chunk_ptr != NULL){
			void *original_ptr = chunk_ptr;
			for(void* ptr = chunk_ptr; ptr < (original_ptr+sizeof(chunk_t)+(chunk_ptr->size_status & size_mask)); ptr+=512){
				if(chunk_ptr->size_status & status_mask){
					fprintf(f_alloc, "%zu %p\n ", instant, ptr);
				}else{
					fprintf(f_free, "%zu %p\n ", instant, ptr);
				}
			}
			chunk_ptr = chunk_ptr->next;
		}
		block_ptr = block_ptr->next;
	}

	chunk_t *chunk_ptr = memory_free;
	while(chunk_ptr != NULL){
		void *original_ptr = chunk_ptr;
		for(void* ptr = chunk_ptr; ptr < (original_ptr+sizeof(chunk_t)+(chunk_ptr->size_status & size_mask)); ptr+=512){
			fprintf(f_list_free, "%zu %p\n ", instant, ptr);
		}	

		chunk_ptr = chunk_ptr->next_free;
	}

	fclose(f_free);
	fclose(f_alloc);
	fclose(f_list_free);

	instant++;
}

int main(int argc, char const *argv[])
{
	srand(time(NULL));

	remove("dat/allocated_memory.dat");
	remove("dat/free_memory.dat");
	remove("dat/free_list_memory.dat");

	size_t size = 0;
	u_int64_t start = 0, stop = 0;
	u_int64_t malloc_sum = 0, malloc_cpt = 0;
	u_int64_t calloc_sum = 0, calloc_cpt = 0;
	u_int64_t realloc_sum = 0, realloc_cpt = 0;
	u_int64_t free_sum = 0, free_cpt = 0;
	u_int64_t write_sum = 0, write_cpt = 0;
	u_int64_t read_sum = 0, read_cpt = 0;
	size_t used_memory = 0;
	int *tab[N] = {};

	for(int i = 0; i < N; i++){
		if(WARN) printf("Iteration n° %d\n", i);

		// choix de la taille
		size = rand_a_b(1,3000);

		// Soit malloc, realloc, calloc 
		if(rand_a_b(0,2) == 0){
			if(WARN) printf("\tMalloc de %zu octets\n", size*sizeof(int));
			// malloc
			start = rdtsc();
			tab[i] = my_malloc(sizeof(int)*size);
			stop = rdtsc();
			malloc_sum += stop - start;
			malloc_cpt += sizeof(int)*size;
			used_memory += size*sizeof(int);
			if(WARN) printf("\tFin Malloc\n\n");
		}else{
			if(WARN) printf("\tCalloc de %zu octets\n", size*sizeof(int));
			// calloc
			start = rdtsc();
			tab[i] = my_calloc(size, sizeof(int));
			stop = rdtsc();
			calloc_sum += stop - start;
			calloc_cpt += sizeof(int)*size;
			used_memory += size*sizeof(int);
			if(WARN) printf("\tFin Calloc\n\n");
		}

		if(rand_a_b(0,5) == 0){
			used_memory -= size*sizeof(int);
			size = rand_a_b(1,3000);
			if(WARN) printf("\tRealloc de %zu octets\n", size*sizeof(int));
			// realloc
			start = rdtsc();
			tab[i] = my_realloc(tab[i], sizeof(int) * size);
			stop = rdtsc();
			realloc_sum += stop - start;
			realloc_cpt += sizeof(int)*size;
			used_memory += size*sizeof(int);		
			if(WARN) printf("\tFin Realloc\n\n");
		}

		// taille du tableau dans la premiere case
		tab[i][0] = size;

		// ecriture et lecture dans le tableau
		if(WARN) printf("\tDebut lectures et ecritures\n");
		for(int j = 0; j <= i; j++){
			size = tab[j][0];
			for(int z = 1; z < size; z++){
				start = rdtsc();
				tab[j][z] = z;
				stop = rdtsc();
				write_sum += stop - start;
				write_cpt+= sizeof(int);

				start = rdtsc();
				tab[j][z];
				stop = rdtsc();
				read_sum += stop - start;
				read_cpt += sizeof(int);

			} 
		}
		if(WARN) printf("\tFin lectures et ecritures\n\n");

		print_memory();
	}

	size_t final_allocated_memory = allocated_memory;

	// liberation de la memoire
	for(int i = 0; i < N; i++){
		if(tab[i] != NULL){
			if(WARN) printf("\tFree de %zu octets\n", tab[i][0] * sizeof(int));
			free_cpt += tab[i][0] * sizeof(int);
			start = rdtsc();
			my_free(tab[i]);
			stop = rdtsc();
			free_sum += stop - start;
			if(WARN) printf("\tFin Free\n\n");
		}
	}

	printf("Malloc\t\tCalloc\t\tRealloc\t\tFree\t\tWrite\t\tRead en octets/cycle\n");
	printf("%lf\t%lf\t%lf\t%lf\t%lf\t%lf\n",														\
			(double) malloc_cpt/malloc_sum, 														\
			(double) calloc_cpt/calloc_sum,															\
			(double) realloc_cpt/realloc_sum,														\
			(double) free_cpt/free_sum,																\
			(double) write_cpt/write_sum,															\
			(double) read_cpt/read_sum);	

	printf("Allocated memory = %zu bytes\n", final_allocated_memory);
	printf("Used memory = %zu bytes\n", used_memory);
	printf("Ratio = %lf \n", (double)used_memory/final_allocated_memory);

	system("gnuplot plot.sh");

	return 0;
}