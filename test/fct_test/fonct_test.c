#ifndef N
	#define N 1000
#endif

#ifndef M
	#define M 10000
#endif

#ifndef PRINT 
	#define PRINT 0
#endif


#include <stdlib.h>

#ifndef WARN
	#define WARN 0
#endif

#include <stdio.h>
#include <time.h>
#include <math.h>

#define min(a, b) ((a < b) ? a : b)

size_t rand_a_b(size_t a, size_t b)
{
	if(a == b){
		return a;
	}

	return rand()%(b-a)+a;
}

int main(int argc, char const *argv[])
{
	int *tab[N] = {};
	size_t size_tab[N] = {};
	size_t size = 0, new_size = 0;

	srand(time(NULL));

	for(int i = 0; i < N; i++){
		printf("\niteration %d\n", i);
		size = rand_a_b(1,M);
		size_tab[i] = size;

		if(rand_a_b(0,2) == 0){
			printf("malloc %d\n", i);
			tab[i] = malloc(size * sizeof(int));
		
			for(int z = 0; z < size; z++){
				tab[i][z] = z;}
			printf("fin malloc\n\n");
		}else{
			printf("calloc %d\n", i);
			tab[i] = calloc(size , sizeof(int));

			for(int z = 0; z < size; z++){
				if(tab[i][z] != 0){
					printf("erreur calloc\n");
					abort();}
				tab[i][z] = z;}
			printf("fin calloc\n");
		}

		if(rand_a_b(0,5) == 0){
			new_size = rand_a_b(1,M);
			int j = rand_a_b(0,i);
			printf("realloc %d\n\n", j);
			if(tab[j] != NULL){
				tab[j] = realloc(tab[j], new_size * sizeof(int));
			
				for(int z = 0; z < min(size_tab[j], new_size) ; z++){
					if(tab[j][z] != z){
						printf("erreur realloc - size: %zu - new_size: %zu - %d \n", size_tab[j] , new_size, z);
						abort();}
				}
				for(int z = 0; z < new_size; z++){
					tab[j][z] = z;
				}

				size_tab[j] = new_size;
			}
			printf("fin realloc\n");
		}

		if(rand_a_b(0,5) == 0){
			int j = rand_a_b(0,i);
			printf("free %d\n", j);
			if(tab[j] != NULL){
				free(tab[j]);
				tab[j] = NULL;	
			}
			printf("fin free\n\n");
		}

	}

	for(int i = 0; i < N; i++){
		if(tab[i] != NULL){
			free(tab[i]);
		}
	}

	return 0;
}
