#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#define HEAP_SIZE 3221225472
#define MIN_BLOCK_SIZE 4096

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
		first_address = (u_int64_t)sbrk(0);

	return ((address-first_address)/HEAP_SIZE)>>10;
}

void print_chunk_size(chunk_t *chunk)
{
	if(chunk == NULL){
		printf("NULL\n");
		return;
	}
	printf("\t\t\t\t|\n");
	printf("\t\ttaille: %zu\n", chunk->chunk_size);
	printf("\t\taddress: %p\n", chunk->address);
	printf("\t\tallocated: %d\n", chunk->allocated);
	print_chunk_size(chunk->next_size);

}

void print_chunk_address(chunk_t *chunk)
{
	if(chunk == NULL){
		printf("NULL\n");
		return;
	}
	printf("\t\t\t\t|\n");
	printf("\t\ttaille: %zu\n", chunk->chunk_size);
	printf("\t\taddress: %p\n", chunk->address);
	printf("\t\tallocated: %d\n", chunk->allocated);
	print_chunk_address(chunk->next_address);

}

void print_size()
{
	printf("size:\n");
	for(int i = 0; i < 218; i++){
		printf("\tsize_tab[%d] = %p\n", i, size_tab[i]);
		print_chunk_size(size_tab[i]);
	}
}

void print_address()
{
	printf("address:\n");
	for(int i = 0; i < 1024; i++){
		printf("\taddress_tab[%d] = %p\n", i, address_tab[i]);
		print_chunk_address(address_tab[i]);
	}
}

chunk_t *search_size(size_t size)
{
	const int index = size_hachage(size);
	assert(index >= 0 && index < 218);

	for(int i = index; i < 218; i++){
		chunk_t *ptr = size_tab[i];
		while(ptr != NULL){
			if(ptr->allocated == 0)
				return ptr;
			ptr = ptr->next_size;
		}
	}

	return NULL;
}

chunk_t *search_address(void* addr)
{
	const int index = address_hachage((u_int64_t)addr);
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

void add_address(chunk_t *chunk)
{
	const int index = address_hachage((u_int64_t)chunk->address);
	assert(index >= 0 && index < 1024);
	chunk->next_address = address_tab[index];
	address_tab[index] = chunk;
}

void add_size(chunk_t *chunk)
{
	const int index = size_hachage(chunk->chunk_size);
	assert(index >= 0 && index < 218);
	chunk->next_size = size_tab[index];
	size_tab[index] = chunk;
}

void remove_address(chunk_t *chunk)
{
	const int index = address_hachage((u_int64_t)chunk->address);
	chunk_t *ptr = address_tab[index];
	if(ptr == chunk){
		address_tab[index] = chunk->next_address;
		chunk->next_address = NULL;
		return;
	}
	while(ptr->next_address != chunk || ptr != NULL){
		ptr = ptr->next;}
	assert(ptr != NULL);
	ptr->next_address = ptr->next_address->next_address;
	ptr->next_address->next_address = NULL;
}

void remove_size(chunk_t *chunk)
{
	const int index = size_hachage(chunk->chunk_size);
	chunk_t *ptr = size_tab[index];
	if(ptr == chunk){
		size_tab[index] = chunk->next_size;
		chunk->next_size = NULL;
		return;
	}
	while(ptr->next_size != chunk || ptr != NULL){
		ptr = ptr->next;}
	assert(ptr != NULL);
	ptr->next_size = ptr->next_size->next_size;
	ptr->next_size->next_size = NULL;
}

/* Les fonctions qui nous interessent */
void *my_malloc(size_t size)
{
	/* Etape 1 Chercher dans la table de hachage si un chunk convient */
	chunk_t *ptr = search_size(size);
	if(ptr != NULL){
		/* Etape 1.1 si oui */
			if(size_hachage(ptr->chunk_size) == size_hachage(size)){
				ptr->allocated = 1;
			}else{
				chunk_t *old_ptr = ptr->next;
				size_t old_size = ptr->chunk_size;
				remove_size(ptr);
				ptr->allocated = 1;
				ptr->chunk_size = size;
				ptr->next = (chunk_t*)mmap(0, sizeof(chunk_t), PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);				
				ptr->next->block_size = ptr->block_size;
				ptr->next->chunk_size = old_size-size;
				ptr->next->address = ptr->address + ptr->chunk_size;
				ptr->next->previous = ptr;
				ptr->next->allocated = 0;
				ptr->next->next_size = NULL;
				ptr->next->next_address = NULL;
				ptr->next->next = old_ptr;
				if(old_ptr != NULL)
					old_ptr->previous = ptr->next;
				add_size(ptr);
				add_address(ptr->next);
				add_size(ptr->next);	
			}
			return ptr;
	}else{
		/* Etape 1.2 si non */
			/* Etape 1.1.1 Allouer un chunk de taille >= MIN_BLOCK_SIZE avec mmap*/
			if(size < MIN_BLOCK_SIZE){
				/* Initialisation du premier chunk */
				ptr = (chunk_t*)mmap(0, sizeof(chunk_t), PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
				ptr->block_size = MIN_BLOCK_SIZE;
				ptr->chunk_size = size;
				ptr->address = mmap(0, MIN_BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
				ptr->previous = NULL;
				ptr->next_size = NULL;
				ptr->allocated = 1;
				ptr->next_address = NULL;
				ptr->next = (chunk_t*)mmap(0, sizeof(chunk_t), PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
				/* Initialisation du second chunk */
				ptr->next->block_size = MIN_BLOCK_SIZE;
				ptr->next->chunk_size = MIN_BLOCK_SIZE-size;
				ptr->next->address = ptr->address + ptr->chunk_size;
				ptr->next->previous = ptr;
				ptr->next->allocated = 0;
				ptr->next->next_size = NULL;
				ptr->next->next_address = NULL;
				ptr->next->next = NULL;
				/* ajout dans les tables de hachage */
				add_size(ptr);
				add_size(ptr->next);
				add_address(ptr);
				add_address(ptr->next);
			}else{
				ptr = (chunk_t*)mmap(0, sizeof(chunk_t), PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
				ptr->block_size = size;
				ptr->chunk_size = size;
				ptr->address = mmap(0, size, PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
				ptr->previous = NULL;
				ptr->next = NULL;	
				ptr->next_size = NULL;
				ptr->allocated = 1;
				ptr->next_address = NULL;
				add_address(ptr);
				add_size(ptr);
			}
		return ptr;
	}

	return NULL;
}

void free(void *ptr)
{
	/* Etape 1 Chercher dans la table de hachage un chunk a l'adresse voulu */
                /* Etape 1.1 si il y en a un */ //et que ce n'est pas un block entier et que > MAX_FREE_CAP //pour éviter de garder trop de mem ?
			/* Etape 1.1.1 le desallouer */
			/* Etape 1.1.2 le fusionner avec le chunk precedent dans le bloc si possible */
			/* Etape 1.1.3 le fusionner avec le chunk suivant dans le bloc si possible */
			/* Etape 1.1.4 si à la fin le chunk est de la meme taille que le block et qu'il n'a pas de predecesseur */
                               /* Etape 1.1.4.1 alors unmap le chunk car il représente un block vide */ //ah je viens de voir que tu fais ça là xD
                // Etape 1.2 unmap le block si on fait ma version
}

void *calloc(size_t nmemb, size_t size)
{

	/* Etape 1 Allouer un chunk de taille >= MIN_BLOCK_SIZE avec mmap*/
	/* Etape 2 Scinder ce block si necessaire dans la liste de chunk */
	/* Etape 3 Ajouter les deux chunks dans la table de hachage */ // toujours cette histoire de block occupé qui est ajouté au hash qui me dérange
	/* Etape 4 Retourner le chunk de taille convenu */

	return NULL;
}

void *realloc(void *ptr, size_t size)
{
	/* Etape 1 Chercher dans la table de hachage si un chunk convient */
		/* Etape 1.1 si oui */
			/* Etape 1.1.1 Le scinder dans la liste des chunks du blocks */
			/* Etape 1.1.2 Ajouter les deux chunks dans la table de hachage */
			/* Etape 1.1.3 Supprimer l'ancien chunk de la table de hachage */
                /* Etape 1.2 si non */ //realloc fait du malloc si y a pas de block ??? :o
			/* Etape 1.1.1 Allouer un chunk de taille >= MIN_BLOCK_SIZE avec mmap */
			/* Etape 1.1.2 Scinder ce block si necessaire dans la liste de chunk */
			/* Etape 1.1.3 Ajouter les deux chunks dans la table de hachage */
	/* Etape 2  copier le contenu de l'ancien chunk dans le nouveau chunk avec memcpy*/
	/* Etape 3 desallouer l'ancien chunk */ //utiliser free --> etape 3 à 6.1
	/* Etape 4 le fusionner avec le chunk precedent dans le bloc si possible */
	/* Etape 5 le fusionner avec le chunk suivant dans le bloc si possible */
	/* Etape 6 si à la fin le chunk est de la mme taille que le block et qu'il n'a pas de predecesseur */
		/* Etape 6.1 alors unmap le chunk car il représente un block vide */
	/* Etape 7 retourner le nouveau chunk */

	return NULL;
}

/********************************************************************************************
*								       Fonctions utiles										*
*							       													*
*	chunk_t *recherche_chunk(size_t size);													*
*		retourne un pointeur vers un chunk de taille >= size ou NULL si il n'y en pas dans 	*
*		la table de hachage 																*
*																				*
*	chunk_t	*merge_free(chunk_t *c);														*
*		fusionne un chunk avec ces voisins libres et retourne l'adresse du nouveau chunk	*
*																				*
********************************************************************************************/	




int main(int argc, char const *argv[])
{
	print_address();
	print_size();

	my_malloc(1056);
	my_malloc(2048);

	print_address();
	print_size();
	return 0;
}
