
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <assert.h>
#include <string.h>
#include <math.h>

/* Fonction de hachage permettant d'acceder aux chunks */
static inline int fct_hachage(size_t size)
{
	if(size >= 512){
		return log2(size+1)*7;/* OK */
	}else{
		return size>>3;	/* OK */
	}
}

/* Structure d'un chunk */
typedef struct chunk_t
{
	size_t block_size;			// taille du block auquel appartient le chunk en octet
	size_t chunk_size;			// taille du chunk en octet
	char allocated;				// indicateur d'allocation 

	struct chunk_t *previous;	// pointeur vers le chunk precedent du block (NULL si tete de la liste)
	struct chunk_t *next;		// pointeur vers le chunk suivant du block (NULL si queue de la liste)

	struct chunk_t *tab_next;	// pointeur vers le chunk suivant de la table de hachage (NULL si queue de la liste)
	
	void *adress;				// adresse de l'espace mémoire correspondant a ce chunk
}chunk_t;

/* Table de hachage */
static chunk_t *tab[218] = {};

/* Les fonctions qui nous interessent */
void *malloc(size_t)
{
	/* Etape 1 Chercher dans la table de hachage si un chunk convient */
		/* Etape 1.1 si oui */
			/* Etape 1.1.1 Le scinder dans la liste des chunks du blocks */
			/* Etape 1.1.2 Ajouter les deux chunks dans la table de hachage */
			/* Etape 1.1.3 Supprimer l'ancien chunk de la table de hachage */
			/* Etape 1.1.4 Retourner le chunk de taille convenu */
		/* Etape 1.2 si non */
			/* Etape 1.1.1 Allouer un chunk de taille >= MIN_BLOCK_SIZE avec mmap*/
			/* Etape 1.1.2 Scinder ce block si necessaire dans la liste de chunk */
			/* Etape 1.1.3 Ajouter les deux chunks dans la table de hachage */
			/* Etape 1.1.4 Retourner le chunk de taille convenu */

	return NULL;
}

void free(void *ptr)
{
	/* Etape 1 Chercher dans la table de hachage un chunk a l'adresse voulu */
		/* Etape 1.1 si il y en a un */
			/* Etape 1.1.1 le desallouer */
			/* Etape 1.1.2 le fusionner avec le chunk precedent dans le bloc si possible */
			/* Etape 1.1.3 le fusionner avec le chunk suivant dans le bloc si possible */
			/* Etape 1.1.4 si à la fin le chunk est de la meme taille que le block et qu'il n'a pas de predecesseur */
				/* Etape 1.1.4.1 alors unmap le chunk car il représente un block vide */
}

void *calloc(size_t nmemb, size_t size);
{

	/* Etape 1 Allouer un chunk de taille >= MIN_BLOCK_SIZE avec mmap*/
	/* Etape 2 Scinder ce block si necessaire dans la liste de chunk */
	/* Etape 3 Ajouter les deux chunks dans la table de hachage */
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
		/* Etape 1.2 si non */
			/* Etape 1.1.1 Allouer un chunk de taille >= MIN_BLOCK_SIZE avec mmap */
			/* Etape 1.1.2 Scinder ce block si necessaire dans la liste de chunk */
			/* Etape 1.1.3 Ajouter les deux chunks dans la table de hachage */
	/* Etape 2  copier le contenu de l'ancien chunk dans le nouveau chunk avec memcpy*/
	/* Etape 3 desallouer l'ancien chunk */
	/* Etape 4 le fusionner avec le chunk precedent dans le bloc si possible */
	/* Etape 5 le fusionner avec le chunk suivant dans le bloc si possible */
	/* Etape 6 si à la fin le chunk est de la mme taille que le block et qu'il n'a pas de predecesseur */
		/* Etape 6.1 alors unmap le chunk car il représente un block vide */
	/* Etape 7 retourner le nouveau chunk */

	return NULL;
}

/********************************************************************************************
*									Fonctions utiles										*
*																							*
*	chunk_t *recherche_chunk(size_t size);													*
*		retourne un pointeur vers un chunk de taille >= size ou NULL si il n'y en pas dans 	*
*		la table de hachage 																*
*																							*	
*	chunk_t	*merge_free(chunk_t *c);														*
*		fusionne un chunk avec ces voisins libres et retourne l'adresse du nouveau chunk	*
*																							*
********************************************************************************************/	




int main(int argc, char const *argv[])
{


	return 0;
}