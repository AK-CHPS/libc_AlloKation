#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#define HEAP_SIZE 3221225472

/* Structure d'un chunk */
typedef struct chunk_t
{
	size_t block_size;				// taille du block auquel appartient le chunk en octet
	size_t chunk_size;				// taille du chunk en octet
	char allocated;					// indicateur d'allocation 

	struct chunk_t *previous;		// pointeur vers le chunk precedent du block (NULL si tete de la liste) // :') j'aurais fais l'inverse xD
	struct chunk_t *next;			// pointeur vers le chunk suivant du block (NULL si queue de la liste)

	struct chunk_t *next_size;		// pointeur vers le chunk suivant de la table de hachage (NULL si queue de la liste)  de la table de hash ?
                                        // ça veut dire que tu répertorie sur les chuncks où est le prochain libre ?
                                        // si oui ça me parait couteux à mettre à jour comme info mais je me trompe peut etre
	struct chunk_t *next_address;	


	void *address;					// adresse de l'espace mémoire correspondant a ce chunk
}chunk_t;

/* Table de hachage */
static chunk_t *size_tab[218] = {};
static chunk_t *address_tab[1024] = {};

/* Fonction de hachage permettant d'acceder aux chunks en fonction de leurs tailles*/
static inline int size_hachage(size_t size)
{
	if(size >= 512){
		return log2(size+1)*7;/* OK */
	}else{
		return size>>3;	/* OK */
	}
}

static u_int64_t first_address = -1;

/* Fonction de hachage permettant d'acceder aux chunks en fonctions de leurs adresses*/
static inline int address_hachage(u_int64_t address)
{
	if(first_address == -1)
		first_address = sbrk(0);
	return (address-first_address)/(HEAP_SIZE>>10);
}

void print_size()
{
	printf("size:\n");
	for(int i = 0; i < 218; i++){
		printf("\tsize_tab[%d] = %zu\n", i, size_tab[i]);
		print_chunk(size_tab[i]);
	}
}

void print_address()
{
	printf("address:\n");
	for(int i = 0; i < 218; i++){
		printf("\taddress_tab[%d] = %zu\n", i, address_tab[i]);
		print_chunk(address_tab[i]);
	}
}

void print_chunk(chunk_t *chunk)
{
	if(chunk == NULL){
		printf("NULL\n");
		return 0;
	}
	printf("\t\t\t\t|\n");
	printf("\t\ttaille: %zu\n", chunk->chunk_size);
	printf("\t\taddress: %zu\n", chunk->address);
	printf("\t\tallocated: %d\n", chunk->allocated);
	print_chunk(chunk->next);

}

chunk_t *search_size(size_t size)
{
	const int index = size_hachage(size);
	assert(index >= 0 && index < 218);

	for(int i = index; i < 218; i++){
		if(size_tab[i] != NULL){
			return size_tab[i];}
	}

	return NULL;
}

chunk_t *search_address(void* addr)
{
	const int index = address_hachage(addr);
	assert(index >= 0 && index < 1024);

	for(int i = index; i < 1024; i++){
		chunk_t *ptr = address_tab[i];
		while(ptr != NULL){
			if(ptr->address == addr)
				return ptr;
			ptr = ptr->next_address;
		}
	}

	return NULL;
}


/* Les fonctions qui nous interessent */
void *my_malloc(size_t size)
{
	/* Etape 1 Chercher dans la table de hachage si un chunk convient */
		/* Etape 1.1 si oui */
			/* Etape 1.1.1 Le scinder dans la liste des chunks du blocks */
			/* Etape 1.1.2 Ajouter les deux chunks dans la table de hachage */  // les 2 ? :o pas juste le libre ?
                        /* Etape 1.1.3 Supprimer l'ancien chunk de la table de hachage */    
			/* Etape 1.1.4 Retourner le chunk de taille convenu */
		/* Etape 1.2 si non */
			/* Etape 1.1.1 Allouer un chunk de taille >= MIN_BLOCK_SIZE avec mmap*/
			/* Etape 1.1.2 Scinder ce block si necessaire dans la liste de chunk */ //cas si < MIN_BLOCK_SIZE
                        /* Etape 1.1.3 Ajouter les deux chunks dans la table de hachage */ //On ajoute les blocks occupé ???
			/* Etape 1.1.4 Retourner le chunk de taille convenu */

	return NULL;
}



int main(int argc, char const *argv[])
{
	print_address();
	print_size();


	return 0;
}