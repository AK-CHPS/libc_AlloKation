#ifdef AK
	#include <libmAlloK.h>
#endif

#ifndef N
	#define N 1000
#endif

#ifndef AK
	#include <stdlib.h>
#endif

#ifndef WARN
	#define WARN 0
#endif

#include <stdio.h>
#include <time.h>



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

	size_t size = 0;
	u_int64_t start = 0, stop = 0;
	u_int64_t malloc_sum = 0, malloc_cpt = 0;
	u_int64_t calloc_sum = 0, calloc_cpt = 0;
	u_int64_t realloc_sum = 0, realloc_cpt = 0;
	u_int64_t free_sum = 0, free_cpt = 0;
	u_int64_t write_sum = 0, write_cpt = 0;
	u_int64_t read_sum = 0, read_cpt = 0;
	int *tab[N] = {};

	for(int i = 0; i < N; i++){
		if(WARN) printf("Iteration n° %d\n", i);

		// choix de la taille
		size = rand_a_b(1,100000);

		// Soit malloc, realloc, calloc 
		if(rand_a_b(0,2) == 0){
			if(WARN) printf("\tMalloc de %zu octets\n", size*sizeof(int));
			// malloc
			start = rdtsc();
			tab[i] = malloc(sizeof(int)*size);
			stop = rdtsc();
			malloc_sum += stop - start;
			malloc_cpt += sizeof(int)*size;
			if(WARN) printf("\tFin Malloc\n\n");
		}else{
			if(WARN) printf("\tCalloc de %zu octets\n", size*sizeof(int));
			// calloc
			start = rdtsc();
			tab[i] = calloc(size, sizeof(int));
			stop = rdtsc();
			calloc_sum += stop - start;
			calloc_cpt += sizeof(int)*size;
			if(WARN) printf("\tFin Calloc\n\n");
		}

		if(rand_a_b(0,5) == 0){
			size = rand_a_b(1,100000);
			if(WARN) printf("\tRealloc de %zu octets\n", size*sizeof(int));
			// realloc
			start = rdtsc();
			tab[i] = realloc(tab[i], sizeof(int) * size);
			stop = rdtsc();
			realloc_sum += stop - start;
			realloc_cpt += sizeof(int)*size;		
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
	}

	// liberation de la memoire
	for(int i = 0; i < N; i++){
		if(tab[i] != NULL){
			if(WARN) printf("\tFree de %zu octets\n", tab[i][0] * sizeof(int));
			free_cpt += tab[i][0] * sizeof(int);
			start = rdtsc();
			free(tab[i]);
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
			(double) read_cpt/read_sum);															\


	return 0;
}