#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>

/*********************** Introduction **************************

Most C and C++ compilers already provide a heap memory-manager as part of the standard library, 
so you don't need to do anything at all in order to avoid hitting the OS with every request.

If you want to improve performance, there are a number of improved allocators around that you can simply 
link with and go. e.g. Hoard, which wheaties mentioned in a now-deleted answer (which actually was quite 
good -- wheaties, why'd you delete it?).

If you want to write your own heap manager as a learning exercise, here are the basic things it needs to do:

    - Request a big block of memory from the OS
    - Keep a linked list of the free blocks
    - When an allocation request comes in:
        - search the list for a block that's big enough for the requested size plus some book-keeping variables stored alongside.
        - split off a big enough chunk of the block for the current request, put the rest back in the free list
        - if no block is big enough, go back to the OS and ask for another big chunk
    - When a deallocation request comes in
        - read the header to find out the size
        - add the newly freed block onto the free list
        - optionally, see if the memory immediately following is also listed on the free list, and combine 
        both adjacent blocks into one bigger one (called coalescing the heap)

***************************************************************/

/************************************************************
*						  CONSTANTES						*
************************************************************/
#define MSIZE 4096000000000

/************************************************************
*					Strucutres de données					*
************************************************************/

typedef struct mspace mspace_t;
typedef struct chunk chunk_t;

/* Liste de blocs mémoires obtenus avec mmap, chaque bloc possede une liste de chunks */
struct mspace{
	size_t mspace_size;
	chunk_t *linked_list;
	mspace_t* next_mspace;
};

/* liste de chunk, par default un seul avec allcated a faut */
struct chunk{
	size_t chunk_size;
	size_t previous_chunk_size;
	char allocated;
	chunk_t *next_chunk;
};


// Pointeur vers la liste de blocs mémoires
static mspace_t *mem = NULL;

/************************************************************
*					Gestion de mspace						*
************************************************************/

static void *add_mspace()
{

}












/*
static void *add_mspace()
{
	mem = (mspace_t*)mmap(0, sizeof(mspace_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

	mem->mspace_size = MSIZE;
	mem->linked_list = mmap(0, MSIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	mem->next_mspace = NULL;

	return mem;
}

static void *del_mspace(mspace_t *addr)
{
	munmap(addr->linked_list, MSIZE);
	munmap(addr,sizeof(mspace_t));

	return mem;
}
*/

/************************************************************
*					Gestion de chunk						*
************************************************************/

