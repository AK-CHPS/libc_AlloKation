#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <time.h>

#define NEW_VERSION
#ifndef N
	#define N 1000
#endif
/*********************************************************************************
										Data
*********************************************************************************/

//compteur de mmap
static u_int64_t mmap_cpt = 0;

//compteur de munmap
static u_int64_t munmap_cpt = 0;

// mask pour obtenir le status
static const u_int64_t status_mask = 0x8000000000000000;
// mask pour obtenir la taille
static const u_int64_t size_mask = 0x7fffffffffffffff;

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
	chunk_t *stack;
	block_t *previous;
	block_t *next;
};

// structure d'un chunk
struct chunk_t
{
	size_t size_status;
	chunk_t *previous;
	chunk_t *next;
};

/*********************************************************************************
**********************************************************************************
NOUVELLE VERSION / NOUVELLE VERSION / NOUVELLE VERSION / NOUVELLE VERSION / NOUVEL
**********************************************************************************
*********************************************************************************/
#ifdef NEW_VERSION

// taille minimum d'un mmap
#define SIZE_MIN_BLOCK (128*1024)

// liste de block
static block_t *head = NULL;
static block_t *tail = NULL;

/*********************************************************************************
									Prototypes
*********************************************************************************/

void *my_malloc(size_t size);

void my_free(void *ptr);

void *my_calloc(size_t nmemb, size_t size);

void *my_realloc(void *ptr, size_t size);

/*********************************************************************************
								Gestion de blocks
*********************************************************************************/

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
	block->next = head;
	block->previous = NULL;
	block->stack = chunk;
	head = block;
	
	// si la liste est pas vide et sinon
	if(block->next != NULL){
		block->next->previous = block;
	}else{
		tail = ptr;
	}

	// initialisation du premier chunk
	chunk->size_status = size;
	chunk->next = NULL;
	chunk->previous = NULL;

	return ptr;
}

// permet de supprimer un block particulier
static void del_block(block_t *block)
{
	if(head == block){
		head = block->next;
	}
	if(tail == block){
		tail = block->previous;
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

// permet d'afficher la pile de chaque block
static int print_stack(block_t *block)
{
	chunk_t *ptr = block->stack;
	while(ptr != NULL){
		printf("\tAddresse du chunk precedent: %zu\n", ptr->previous);
		printf("\tAddresse du chunk: %zu\n", ptr);
		printf("\tTaille du chunk: %zu\n", ptr->size_status & size_mask);
		printf("\tAllocation du chunk: %zu\n", ptr->size_status >> 63);
		printf("\tAddresse du chunk suivant: %zu\n\n", ptr->next);
		ptr = ptr->next;
	}
	return 0;
}

// permet d'afficher la memoire alloué
static int print_memory()
{
	block_t *ptr = head;
	while(ptr != NULL){
		printf("\n-------------------------\n");
		printf("Addresse du block precedent: %zu\n", ptr->previous);
		printf("Addresse du block: %zu\n", ptr);
		printf("First addresse: %zu\n", ptr->first_address);
		printf("Last addresse: %zu\n", ptr->last_address);
		printf("Taille du block: %zu\n", ptr->size);
		printf("Premier chunk de la pile: %zu\n", ptr->stack);
		printf("Addresse du prochain block: %zu\n\n", ptr->next);
		print_stack(ptr);
		ptr = ptr->next;
	}

	return 0;
}

/*********************************************************************************
								Gestion de chunk
*********************************************************************************/

// permet d'allouer un chunk
static chunk_t * alloc_chunk(chunk_t * chunk, size_t size)
{
	if(chunk != NULL && chunk->size_status >> 63 != 1 && size <= chunk->size_status){
		if(chunk->size_status-sizeof(chunk_t)-size < sizeof(chunk_t)){
			chunk->size_status |= status_mask;

			return chunk;
		}else{
			chunk->size_status -= (sizeof(chunk_t)+size);
			void *new_address = (void*)chunk + chunk->size_status +  sizeof(chunk_t);
			chunk_t *new_chunk = new_address;
			new_chunk->next = chunk->next;

			if(chunk->next != NULL){
				chunk->next->previous = new_chunk;
			}			
			
			chunk->next = new_chunk;
			new_chunk->previous = chunk;
			new_chunk->size_status = size + status_mask;

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
			chunk->previous->next = chunk->next;
			if(chunk->next != NULL){
				chunk->next->previous = chunk->previous;}
			chunk->previous->size_status += (chunk->size_status + sizeof(chunk_t));
			chunk = chunk->previous;
		}

		if(chunk->next != NULL && chunk->next->size_status >> 63 == 0){
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
	return NULL;
}

/*********************************************************************************
							Fonctions standards
*********************************************************************************/

void *my_malloc(size_t size)
{
	return NULL;
}

void my_free(void *ptr)
{
	return;
}

void *my_calloc(size_t nmemb, size_t size)
{
	return NULL;
}

void *my_realloc(void *ptr, size_t size)
{
	return NULL;
}

/*********************************************************************************
					Chargement et dechargment preventif
*********************************************************************************/

// alloue un premier chunk dans le doute :) 
__attribute__((constructor)) void my_constructor(void) 
{

}

// supprime la mémoire dans le doute :) 
__attribute__((destructor)) void my_destructor(void) 
{

}

#endif


/*********************************************************************************
**********************************************************************************
ANCIENNE VERSION / ANCIENNE VERSION / ANCIENNE VERSION / ANCIENNE VERSION / ANCIEN
**********************************************************************************
*********************************************************************************/
#ifdef OLD_VERSION

/*********************************************************************************
									Hach Table
*********************************************************************************/

/* Hach table for size */
static chunk_t *size_tab[218] = {};

/* Fonction de hachage permettant d'acceder aux chunks en fonction de leurs tailles*/
static inline int size_hachage(size_t size)
{
	if(size >= 512){
		return log2(size+1)*7; /* OK */
	}else{
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

	if(tmp == ptr){
		size_tab[index] = tmp->tab_next;
		tmp->tab_next = NULL;
		return;}
	while(tmp->tab_next != ptr && tmp != NULL){
		tmp = tmp->tab_next;}
	if(tmp != NULL){
		chunk_t *old_next = tmp->tab_next;
		tmp->tab_next = tmp->tab_next->tab_next;
		old_next->tab_next = NULL;
	}
}

static void print_tab()
{
	printf("------------------ Print Tab --------------------\n");
	for(int i = 0; i < 218; i++){
		chunk_t *ptr = size_tab[i];
		if(ptr != NULL)	printf("size_tab[%d]: \n", i);
		while(ptr != NULL){
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

	for(int i = index; i < 218; i++){
		chunk_t *ptr = size_tab[i];
		while(ptr != NULL){
			if((ptr->size_status & status_mask) == 0){
				return ptr;}
			ptr = ptr->tab_next;
		}
	}
	return NULL;
}

/*********************************************************************************
								Block Managment
**********************************************************************************/

static chunk_t *add_block(size_t size)
{
	chunk_t *chunk = mmap(0, sizeof(chunk_t)+size, PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	chunk->prev = NULL;
	chunk->size_status = size;
	chunk->next = NULL;
	mmap_cpt++;
	return chunk;
}

static void del_block(chunk_t *chunk)
{
	munmap(chunk, sizeof(chunk)+ (chunk->size_status & size_mask));
	munmap_cpt++;
}

/*********************************************************************************
								Chunk Managment
**********************************************************************************/

void *my_malloc(size_t size)
{
	chunk_t *ptr = search_size(size+sizeof(chunk_t));

	if(ptr == NULL){
		ptr = add_block(size);
		ptr->size_status += status_mask;
		add_chunk(ptr);
		return ptr+1;
	}else{
		if(ptr->size_status - (sizeof(chunk_t)+size) < sizeof(chunk_t)){
			ptr->size_status |= status_mask;
			return ptr+1;
		}else{
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

void my_free(void *ptr)
{
	chunk_t *chunk = ptr-sizeof(chunk_t);

	chunk->size_status -= status_mask;
	del_chunk(chunk);

	if(chunk->prev != NULL && (chunk->prev->size_status & status_mask == 0)){
		del_chunk(chunk);
		del_chunk(chunk->prev);
		chunk->prev->next = chunk->next;
		chunk->prev->size_status += chunk->size_status + sizeof(chunk_t);
		add_chunk(chunk->prev);
		chunk = chunk->prev;
	}
	if(chunk->next != NULL && (chunk->next->size_status & status_mask == 0)){
		del_chunk(chunk);
		del_chunk(chunk->next);
		chunk->next = chunk->next->next;
		chunk->size_status += chunk->next->size_status + sizeof(chunk_t);
		add_chunk(chunk);
	}
}

void *my_calloc(size_t nmemb, size_t size)
{
	void *ptr = my_malloc(size*nmemb);
	memset(ptr, 0, size*nmemb);
	return ptr;
}

void *my_realloc(void *ptr, size_t size)
{
	void *old_ptr = ptr;
	chunk_t *old_chunk = ptr-sizeof(chunk_t);
	size_t old_size = old_chunk->size_status & size_mask;
	
	my_free(ptr);
	ptr = my_malloc(size);
	if(ptr != old_ptr)
		memcpy(ptr,old_ptr,old_size);

	return ptr;
}

#endif

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

int main(int argc, char const *argv[])
{
	srand(time(NULL));

	printf("taille d'un block: %zu\n", sizeof(block_t));
	printf("taille d'un chunk: %zu\n\n", sizeof(chunk_t));

	size_t size = 0, free_cpt = 0, allocation_sum = 0, free_sum = 0;
	u_int64_t start = 0, stop = 0, cpt = 0;

	for(int i = 0; cpt < N; i++){
		size = rand_a_b(2000,100000);
		block_t *block = add_block(size);
		size_t nb_chunk = rand_a_b(2,11);
		chunk_t *t[N] = {};
		for(int j = 0; j < nb_chunk-1; j++){
			start = rdtsc();
			t[j] = alloc_chunk(block->stack,size/(nb_chunk+1));}
			stop = rdtsc();
			allocation_sum += stop - start;
		for(int j = 0; j < nb_chunk-1; j++){
			if(rand_a_b(0,2)){
				start = rdtsc();
				free_chunk(t[j]);
				stop = rdtsc();
				free_sum += stop - start;
				free_cpt++;}}
		del_block(block);
		cpt += nb_chunk;
	}


	printf("Nombre de cycle moyen par allocation d'un chunk = %lf\n", (double)allocation_sum/N);
	printf("Nombre de cycle moyen par free d'un chunk = %lf\n\n", (double)free_sum/free_cpt);

	size = free_cpt = allocation_sum = free_sum = 0;
	start = 0, stop = 0, cpt = 0;

	block_t *t[N] = {};

	for(int i = 0; i < N; i++){
		size = rand_a_b(1,100000);
		start = rdtsc();
		t[i] = add_block(size);
		stop = rdtsc();
		allocation_sum += stop-start;
		if(rand_a_b(0,2)){
			start = rdtsc();
			del_block(t[i]);
			stop = rdtsc();
			free_sum += stop-start;
			free_cpt++;
		}
	}

	printf("Nombre de cycle moyen par allocation d'un block = %lf\n", (double)allocation_sum/N);
	printf("Nombre de cycle moyen par free d'un block = %lf\n", (double)free_sum/free_cpt);

	return 0;
}