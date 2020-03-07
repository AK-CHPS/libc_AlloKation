#define _GNU_SOURCE
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


#define NEW_VERSION
#ifndef N
	#define N 1000
#endif

#ifndef WARN
	#define WARN 1
#endif

#ifndef PERSO
	#define PERSO 1
#endif

#ifndef M
	#define M 1000
#endif

////////////////////////////////////////////////////////////////////////////

// mask pour obtenir le status
static const size_t status_mask = 0x8000000000000000;
// mask pour le dirty bit
static const size_t dirty_mask = 0x4000000000000000;
// mask pour obtenir la taille
static const size_t size_mask = 0x3fffffffffffffff;

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

#ifndef MIN_RESIDUAL_SIZE
	#define MIN_RESIDUAL_SIZE 500000000
#endif

// liste de block représentant la mémoire
static block_t *memory = NULL;

// liste de chunk représentant la mémoire libre
static chunk_t *memory_free_head = NULL;
static chunk_t *memory_free_tail = NULL;

// quantité de mémoire allouée
static size_t allocated_memory = 0;

// quantité de mémoire alloué dans les block
static size_t block_cpt = 0;

// min macro
#define min(a,b) ((a < b) ? a : b)

// initialisation mutex
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

////////////////////////////////////////////////////////////////////////////

size_t rand_a_b(size_t a, size_t b)
{
	return rand()%(b-a)+a;
}

// permet d'arrondir au multiple de roundTo supperieur
static size_t roundTo(size_t value, size_t roundTo)
{
    return (value + (roundTo - 1)) & ~(roundTo - 1);
}

// ajout d'un chunk libre
static inline void add_free_chunk(chunk_t *chunk)
{
	if((u_int64_t)chunk % 8 != 0)
		printf("PAS ALLIGNE\n");

    if(memory_free_head == NULL){
        memory_free_head = chunk; 
        memory_free_tail = chunk;                    
    }else if((memory_free_head->size_status & size_mask) >= (chunk->size_status & size_mask)) { 
        chunk->next_free = memory_free_head;
        memory_free_head->previous_free = chunk;
        memory_free_head = chunk;
    }else{
        chunk_t *current = memory_free_head; 

        while(current->next_free != NULL && (current->next_free->size_status & size_mask) < (chunk->size_status & size_mask)){ 
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

// permet d'ajouter un block a la liste
static block_t * add_block(size_t size)
{
  // allocation de la mémoire
  void *ptr = mmap(0, sizeof(block_t) + sizeof(chunk_t) + size, PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
 
  if((u_int64_t)ptr % 4096 != 0){
	printf("PAS ALLIGNE\n");
  }

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
  
  // si la liste est pas vid
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

  allocated_memory -= block->size;

  block_cpt-=block->size;

  munmap(block,block->size+sizeof(chunk_t)+sizeof(block_t));

  return ;
}

////////////////////////////////////////////////////////////////////////////

// permet d'allouer un chunk
static chunk_t * alloc_chunk(chunk_t * chunk, size_t size)
{
    del_free_chunk(chunk);
 
    if(chunk->size_status - size <= sizeof(chunk_t)){
      chunk->size_status += status_mask;
      chunk->size_status |= dirty_mask;
    }else{
      chunk_t *new_chunk = (void*)chunk + size + sizeof(chunk_t);

      if(chunk->next != NULL){
      	chunk->next->previous = new_chunk;}
      new_chunk->next = chunk->next;
      chunk->next = new_chunk;
      new_chunk->previous = chunk;

      new_chunk->size_status = (chunk->size_status & size_mask) - (size + sizeof(chunk_t));
      chunk->size_status = size;
      chunk->size_status |= status_mask;
      chunk->size_status |= dirty_mask;
      new_chunk->block = chunk->block;
      new_chunk->next_free = NULL;
      new_chunk->previous_free = NULL;

      add_free_chunk(new_chunk); 
    }

    return chunk;
}

// permet de liberer un chunk 
static void free_chunk(chunk_t *chunk)
{
  chunk->size_status &= size_mask;

  if(chunk->previous != NULL && chunk->previous->size_status & status_mask == 0){
    del_free_chunk(chunk->previous);

   chunk->previous->next = chunk->next;
    if(chunk->next != NULL){
      chunk->next->previous = chunk->previous;}
    chunk->previous->size_status += (chunk->size_status & size_mask) + sizeof(chunk_t);
    chunk->previous->size_status |= (chunk->size_status & dirty_mask);
    chunk = chunk->previous;
  }

   if(chunk->next != NULL && chunk->next->size_status & status_mask == 0){
    del_free_chunk(chunk->next);

   if(chunk->next->next != NULL){
      chunk->next->next->previous = chunk;
    }
    chunk->size_status += (chunk->next->size_status & size_mask) + sizeof(chunk_t);
    chunk->size_status |= (chunk->next->size_status & dirty_mask);
    chunk->next = chunk->next->next;
  }

  if(chunk->previous == NULL && chunk->next == NULL && block_cpt > MIN_RESIDUAL_SIZE){
    del_block(chunk->block);
  }else{
	add_free_chunk(chunk);
  }

  return;
}

//recherche un chunk libre suffisament grand pour contenir la quantité de mémoire demandé
static inline chunk_t* search_chunk(size_t size, char clean)
{
	chunk_t *to_return = NULL, *ptr = memory_free_head;
	size_t chunk_size = 0xFFFFFFFFFFFFFFFF;

	if(memory_free_tail == NULL || (memory_free_tail != NULL && (memory_free_tail->size_status & size_mask) < size))
		return NULL;

	while(ptr != NULL){
		if(((clean == 1 && (ptr->size_status & dirty_mask) == 0) || clean == 0) && \
		  	(ptr->size_status & size_mask) < chunk_size && \
		  	(ptr->size_status & size_mask) >= size){
				to_return = ptr;
				chunk_size = (ptr->size_status & size_mask);}
		ptr = ptr->next_free;
	}

	return to_return;
}

////////////////////////////////////////////////////////////////////////////

void *my_malloc(size_t size)
{
  pthread_mutex_lock(&mutex);

  size = roundTo(size,8);

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

void my_free(void *ptr)
{
  pthread_mutex_lock(&mutex);

  free_chunk(ptr-sizeof(chunk_t));

  pthread_mutex_unlock(&mutex);
}

void *my_calloc(size_t nmemb, size_t size)
{
  	pthread_mutex_lock(&mutex);
	
	size_t total_size = roundTo(size*nmemb,8);

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

void *my_realloc(void *ptr, size_t size)
{
  pthread_mutex_lock(&mutex);

  size = roundTo(size,8);

  chunk_t *old_chunk = ptr - sizeof(chunk_t);

  size_t old_size = old_chunk->size_status & size_mask;

  	if(size < old_size){
	  	if(old_size - size < SIZE_MIN_BLOCK){
	  		return ptr;
	  	}else{
	  		old_chunk->size_status -= status_mask;
	  		add_free_chunk(old_chunk);
	  		return alloc_chunk(old_chunk,size)+1;
	  	}
  	}else if(old_chunk->next != NULL && (old_chunk->size_status & status_mask) == 0 && \
  	(old_chunk->size_status & size_mask) + (old_chunk->next->size_status & size_mask) + sizeof(chunk_t) >= size){
	  	del_free_chunk(old_chunk->next);
	  		
	  	if(old_chunk->next->next != NULL){
	  		old_chunk->next->previous = old_chunk;}
	  	old_chunk->next = old_chunk->next->next;
	  	old_chunk->size_status += (old_chunk->next->size_status & size_mask) + sizeof(chunk_t);
	  		
	  	return ptr;
  	}else if(old_chunk->previous == NULL && old_chunk->next == NULL){
  		void *old_addr = ptr-sizeof(chunk_t)-sizeof(block_t);

  		void *new_addr = mremap(old_addr, old_size + sizeof(block_t) + sizeof(chunk_t), size + sizeof(chunk_t) + sizeof(block_t), MREMAP_MAYMOVE);

  		assert(new_addr != NULL);

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
  		new_chunk->size_status = size + status_mask + dirty_mask;
  		new_chunk->next = new_chunk->previous = NULL;
  		new_chunk->next_free = new_chunk->previous_free = NULL;
  		new_chunk->block = new_block;

  		return new_addr + sizeof(block_t) + sizeof(chunk_t);
  	}else{
		void *new_ptr = my_malloc(size);

		memcpy(new_ptr, ptr, old_size);
		
		free_chunk(old_chunk);

		return new_ptr;
  	}

  	pthread_mutex_unlock(&mutex);

  return NULL;
}

void __attribute__((constructor)) constructor()
{
	add_block(SIZE_MIN_BLOCK);
} 

void __attribute__((destructor)) destructor()
{
	block_t *ptr = memory, *old_ptr = NULL;

	while(ptr != NULL){
		old_ptr = ptr;
		ptr = ptr->next;
		del_block(old_ptr);
	}
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
			
			for(void* ptr = chunk_ptr; ptr < (original_ptr+sizeof(chunk_t)+(chunk_ptr->size_status & size_mask)); ptr+=1024){
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

	chunk_t *chunk_ptr = memory_free_head;
	while(chunk_ptr != NULL){
		void *original_ptr = chunk_ptr;
		for(void* ptr = chunk_ptr; ptr < (original_ptr+sizeof(chunk_t)+(chunk_ptr->size_status & size_mask)); ptr+=1024){
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
		size = rand_a_b(M-(0.4*M),M+(0.4*M));

		// Soit malloc, realloc, calloc 
		if(rand_a_b(0,2) == 0){
			if(WARN) printf("\tMalloc de %zu octets\n", size*sizeof(int));
			// malloc
			start = rdtsc();
			#if PERSO == 1
				tab[i] = my_malloc(sizeof(int)*size);
			#else
				tab[i] = malloc(sizeof(int)*size);
			#endif
			stop = rdtsc();
			malloc_sum += stop - start;
			malloc_cpt += sizeof(int)*size;
			used_memory += size*sizeof(int);
			if(WARN) printf("\tFin Malloc\n\n");
		}else{
			if(WARN) printf("\tCalloc de %zu octets\n", size*sizeof(int));
			// calloc
			start = rdtsc();
			#if PERSO == 1
				tab[i] = my_calloc(size, sizeof(int));
			#else
				tab[i] = calloc(size, sizeof(int));
			#endif
			stop = rdtsc();
			calloc_sum += stop - start;
			calloc_cpt += sizeof(int)*size;
			used_memory += size*sizeof(int);
			if(WARN) printf("\tFin Calloc\n\n");
		}

		if(rand_a_b(0,5) == 0){
			used_memory -= size*sizeof(int);
			size = rand_a_b(M-(0.4*M),M+(0.4*M));
			if(WARN) printf("\tRealloc de %zu octets\n", size*sizeof(int));
			// realloc
			start = rdtsc();
			#if PERSO == 1
				tab[i] = my_realloc(tab[i], sizeof(int) * size);
			#else
				tab[i] = realloc(tab[i], sizeof(int) * size);
			#endif
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
		for(int j = 0; j < i; j++){
			size = tab[j][0];
			int res = 0;
			for(int z = 1; z < size; z++){
				start = rdtsc();
				res = tab[j][z];
				stop = rdtsc();
				read_sum += stop - start;
				read_cpt += sizeof(int);
				if(res != z){
					printf("Problème itération %d\n", j);
				}

				start = rdtsc();
				tab[j][z] = z;
				stop = rdtsc();
				write_sum += stop - start;
				write_cpt+= sizeof(int);
			}
		}

		size = tab[i][0];
		int res = 0;
		for(int z = 1; z < size; z++){
			start = rdtsc();
			tab[i][z] = z;
			stop = rdtsc();
			write_sum += stop - start;
			write_cpt+= sizeof(int);
		} 		
		if(WARN) printf("\tFin lectures et ecritures\n\n");

		#if PERSO == 1
			print_memory();
		#endif
	}

	// liberation de la memoire
	for(int i = 0; i < N; i++){
		if(tab[i] != NULL){
			if(WARN) printf("\tFree de %zu octets\n", tab[i][0] * sizeof(int));
			free_cpt += tab[i][0] * sizeof(int);
			start = rdtsc();
			#if PERSO == 1
				my_free(tab[i]);
			#else
				free(tab[i]);
			#endif
			stop = rdtsc();
			free_sum += stop - start;
			if(WARN) printf("\tFin Free\n\n");
		}
	}

	printf("%lf\t%lf\t%lf\t%lf\t%lf\t%lf\n",														\
			(double) malloc_cpt/malloc_sum, 														\
			(double) calloc_cpt/calloc_sum,															\
			(double) realloc_cpt/realloc_sum,														\
			(double) free_cpt/free_sum,																\
			(double) write_cpt/write_sum,															\
			(double) read_cpt/read_sum);	


	//system("gnuplot plot.sh");

	return 0;
}