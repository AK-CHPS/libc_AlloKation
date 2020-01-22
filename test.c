#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <assert.h>

typedef struct block block_t;
typedef struct chunk chunk_t;

struct block{
	size_t size;
	block_t *next;
	chunk_t *list;
};

struct chunk
{
	size_t previous_size;
	size_t size;
	char allocated;
	chunk_t *next;	
};

int main(int argc, char const *argv[])
{
	printf("taille d'un block %ld octets\n", sizeof(block_t));
		printf("\ttaille d'un size_t %ld\n", sizeof(size_t));
		printf("\ttaille d'un block_t* %ld\n", sizeof(block_t*));
		printf("\ttaille d'un chunk_t* %ld\n\n", sizeof(chunk_t*));

	printf("taille d'un chunk %ld octets\n", sizeof(chunk_t));
		printf("\ttaille d'un size_t %ld\n", sizeof(size_t));
		printf("\ttaille d'un size_t %ld\n", sizeof(size_t));
		printf("\ttaille d'un chunk_t* %ld\n", sizeof(chunk_t*));

	printf("\n\n\n");



	block_t *mem = (block_t*)mmap(0, 512, PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	mem->size = 512-sizeof(block_t);
	mem->next = NULL;
	mem->list = (chunk_t*)mem+sizeof(block_t);

	chunk_t *node1 = mem->list;

	node1->previous_size = 0;
	node1->size = (mem->size/2)-sizeof(chunk_t);
	node1->allocated = 1;
	node1->next = (&(*node1)) +sizeof(chunk_t)+(*node1).size;
	
	chunk_t *node2 = node1->next;	
	mem->list->next->previous_size = mem->list->size;
	mem->list->next->size = (mem->size/2)-sizeof(chunk_t);
	mem->list->next->allocated = 0;
	mem->list->next->next = NULL; 

	printf("addresse de mem = %lld\n", mem);
	printf("\tmem->size = %zu octets\n", (*mem).size);
	printf("\tmem->next = %lld\n", (*mem).next);
	printf("\tmem->list = %lld\n\n", (*mem).list);
	printf("\taddresse de node1 = %lld\n", &(*node1));
	printf("\t\tnode1->previous_size = %zu octets\n", (*node1).previous_size);
	printf("\t\tnode1->size = %zu\n", (*node1).size);
	printf("\t\tnode1->allocated = %d\n", (*node1).allocated);
	printf("\t\tnode1->next = %lld\n\n", (*node1).next);
	printf("\taddresse de node2 = %lld\n", &(*node2));
	printf("\t\tnode2->previous_size = %zu octets\n", (*node2).previous_size);
	printf("\t\tnode2->size = %zu\n", (*node2).size);
	printf("\t\tnode2->allocated = %d\n", (*node2).allocated);
	printf("\t\tnode2->next = %p\n\n", (*node2).next);

	return 0;
}