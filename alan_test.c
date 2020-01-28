#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <assert.h>
#include <string.h>

/* structures de donneés */

typedef struct block block_t;
typedef struct chunk chunk_t;
typedef struct bin bin_t;

struct block{
	size_t size;
	chunk_t *list;
	block_t *next;
};

struct chunk{
	size_t size;
	char allocated;
	chunk_t *previous;
	chunk_t *next;
	void *adr;
};

static block_t *MEM = NULL;

static void *add_block(size_t block_size);
static void del_block(block_t *adr);
static void print_MEM();
static void *add_chunk(size_t chunk_size);
static void del_chunk(void *adr);
static void *move_chunk(void *adr, size_t chunk_newsize);
static void print_list();


/* Gestion des blocks */

#define SMALLEST_SIZE 4096

#define BIGEST_SIZE 2147483648

static void *add_block(size_t block_size)
{
	/* allocation des metadata du block */
	block_t *tmp = (block_t*)mmap(0, sizeof(block_t), PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	tmp->next = MEM;

	/* allocatin des metadata du chunk d'origine */
	tmp->list = (chunk_t*)mmap(0, sizeof(chunk_t), PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	chunk_t *l = tmp->list;

	/* allocation de l'espace mémoire dans le chunk d'origine */
	if(block_size < SMALLEST_SIZE){
		l->adr = mmap(0, SMALLEST_SIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		l->size = SMALLEST_SIZE;
		tmp->size = SMALLEST_SIZE;
	}else if(block_size > BIGEST_SIZE){
		l->adr = mmap(0, BIGEST_SIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		l->size = BIGEST_SIZE;
		tmp->size = BIGEST_SIZE;
	}else{
		l->adr = mmap(0, block_size, PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		l->size = block_size;
		tmp->size = block_size;
	}

	/* initialisation du chunk d'origine */
	l->allocated = 0;
	l->previous = NULL;
	l->next = NULL;

	MEM = tmp;
	return tmp;
}

static void del_block(block_t *adr)
{
	block_t *actual = MEM, *pred = NULL;
	chunk_t *c = actual->list, *n = NULL;

	/* Si l'adresse est celle du premier block */
	if(MEM == adr){
		MEM = actual->next;
		/* 1 suppression de l'esapce memoire du block contenu à l'adresse pointée par le premier chunk*/ 
		munmap(actual->list->adr, actual->size);
		/* 2 suppression de tous les chunk de la liste du block */ 
		while(c != NULL){
			n = c->next;
			munmap(c,sizeof(chunk_t));
			c = n;}
		/* 3 suppression du block*/
		munmap(actual,sizeof(block_t));
		return;}

	/* Sinon parcourir la list de block jusqu'a trouver l'adresse */
	while(actual != adr && actual != NULL){
		pred = actual;
		actual = actual->next;}

	/* si l'adresse à été trouvé alors supprimer le block */
	if(actual != NULL){
		pred->next = actual->next;
		/* 1 suppression de l'esapce memoire du block contenu à l'adresse pointée par le premier chunk*/ 
		munmap(actual->list->adr, actual->size);
		/* 2 suppression de tous les chunk de la liste du block */ 
		chunk_t *c = actual->list, *n = NULL;
		while(c != NULL){
			n = c->next;
			munmap(c,sizeof(chunk_t));
			c = n;}
		/* 3 suppression du block*/
		munmap(actual,sizeof(block_t));
		return;}
}

static void print_MEM()
{
	block_t *actual = MEM;

	while(actual != NULL){
		printf("block adress: %zu\n", actual);
		printf("block size: %zu\n", actual->size);
		printf("next block: %zu\n", actual->next);
		printf("list first chunk: %zu\n\n", actual->list);
		print_list(actual);
		actual = actual->next;
	}

	printf("NULL\n");
	printf("--------------------------------------------------------------------\n");
}

/* Gestion des chunks */

static void* add_chunk(size_t chunk_size)
{
	block_t *actual = MEM;
	chunk_t *pt = NULL;

	/* on parcourt les blocks */
	while(actual != NULL){
		/* on parcourt les chunks du blocks */
		pt = actual->list;
		while(pt != NULL){
			/* le chunk est de taille correspond a celle demandé et qu'il est libre */
			if(pt->size >= chunk_size && pt->allocated == 0)
				break;
			else
				pt = pt->next;
		}
		if(pt != NULL)
			break;
		else
			actual = actual->next;
	}

	/* 1 un chunk de taille suffisante est trouvé */
	if(pt != NULL){
		/* si il ne restera pas assez d'espace apres avoir retiré chunk size alors ne pas le faire */
		if(chunk_size == pt->size){
			pt->allocated = 1;			
			return pt->adr;
		}else{
			/* création du chunk représentant l'espace restant */
			chunk_t *new_chunk = (chunk_t*)mmap(0, sizeof(chunk_t), PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
			new_chunk->allocated = 0;
			new_chunk->size = pt->size - chunk_size;
			new_chunk->adr = &(*pt->adr) + chunk_size;
			new_chunk->next = pt->next;
			new_chunk->previous = pt;
			/* modification du chunk alloué */
			pt->allocated = 1;
			pt->size = chunk_size;
			pt->next = new_chunk;
			return pt->adr;
		}
	}

	/* 2 aucun chunk de taille suffisante n'à été trouvé */
	else{
		actual = add_block(chunk_size);
		pt = actual->list;
		/* si il ne restera pas assez d'espace apres avoir retiré chunk size alors ne pas le faire */
		if(chunk_size == pt->size){
			pt->allocated = 1;			
			return pt->adr;
		}else{
			/* création du chunk représentant l'espace restant */
			chunk_t *new_chunk = (chunk_t*)mmap(0, sizeof(chunk_t), PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
			new_chunk->allocated = 0;
			new_chunk->size = pt->size - chunk_size;
			new_chunk->adr = &(*pt->adr) + chunk_size;
			new_chunk->next = pt->next;
			new_chunk->previous = pt;
			/* modification du chunk alloué */
			pt->allocated = 1;
			pt->size = chunk_size;
			pt->next = new_chunk;
			return pt->adr;
		}
	}

	printf("\n\nchunk address %zu taille demandé %zu taille du chunk %zu \n\n\n", pt->adr, chunk_size, pt->size);
}

static void del_chunk(void *adr)
{
	block_t *actual = MEM;
	chunk_t *pt = NULL, *old_pt = NULL;

	/* on parcourt les blocks */
	while(actual != NULL){
		/* on parcourt les chunks du blocks */
		pt = actual->list;
		while(pt != NULL){
			/* le chunk pointe vers cette adresse */
			if(pt->adr == adr)
				break;
			else
				pt = pt->next;
		}
		if(pt != NULL)
			break;
		else
			actual = actual->next;
	}

	if(pt != NULL){
		/* 1 le désalloué */
		pt->allocated = 0;

		/* 2 le fusionner avec le chunk avant ou apres lui si ils sont libre */
		if(pt->previous != NULL && pt->previous->allocated == 0){
			pt->previous->size += pt->size;
			pt->previous->next = pt->next;
			if(pt->next != NULL)
				pt->next->previous = pt->previous;
			old_pt = pt;
			pt = pt->previous;
			munmap(old_pt,sizeof(chunk_t));
		}

		if(pt->next != NULL && pt->next->allocated == 0){
			pt->size += pt->next->size;
			pt->next = pt->next->next;
			if(pt->next != NULL)
				pt->next->previous = pt;
			munmap(pt->next,sizeof(chunk_t));
		}

	}
}

static void *move_chunk(void	*adr, size_t chunk_newsize)
{
	void *new_adr = add_chunk(chunk_newsize);

	block_t *actual = MEM;
	chunk_t *pt = NULL;

	/* on parcourt les blocks */
	while(actual != NULL){
		/* on parcourt les chunks du blocks */
		pt = actual->list;
		while(pt != NULL){
			/* le chunk pointe vers cette adresse */
			if(pt->adr == adr)
				break;
			else
				pt = pt->next;
		}
		if(pt != NULL)
			break;
		else
			actual = actual->next;
	}

	/* copie le contenu à la nouvelle adresse et supprimme l'ancienne */
	if(pt != NULL){
		memcpy(new_adr, adr, pt->size);
		del_chunk(adr);
	}else{
		return NULL;
	}

	return new_adr;
}

static void print_list(block_t *actual)
{
	chunk_t *pt = actual->list;

	while(pt != NULL){
		printf("\tadresse du chunk = %zu\n", pt);
		printf("\tallocated = %d\n", pt->allocated);
		printf("\tadresse pointe par le chunk = %zu\n", &(*pt->adr));
		printf("\ttaille du chunk = %zu\n", pt->size);
		printf("\tadresse du chunk precedent = %zu\n", pt->previous);
		printf("\tadresse du suivant = %zu\n\n", pt->next);
		pt = pt->next;
	}
}


int main(int argc, char const *argv[])
{
	block_t *block1 = add_block(454645);
	block_t *block2 = add_block(21000);
	block_t *block3 = add_block(50364);
	block_t *block4 = add_block(89);
	print_MEM();

	void *pt6 = add_chunk(60000);
	void *pt5 = add_chunk(50);
	void *pt4 = add_chunk(4096);
	void *pt3 = add_chunk(90000);
	void *pt2 = add_chunk(890);
	void *pt1 = add_chunk(8888888);
	print_MEM();

	del_chunk(pt1);
	print_MEM();

	del_chunk(pt3);
	print_MEM();

	del_chunk(pt5);
	print_MEM();

	del_chunk(pt2);
	print_MEM();

	del_block(block1);
	del_block(block3);
	del_block(block4);
	del_block(block2);
	print_MEM();
	printf("pas de panique c normal celui la n'est pas supprimé par nous mais par le systeme\n");
}
