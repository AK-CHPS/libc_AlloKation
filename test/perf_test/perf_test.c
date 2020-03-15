#ifndef N
	#define N 1000
#endif

#ifndef M
	#define M 10000
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


	int *tab[N] = {};
	size_t size_tab[N] = {};
	size_t size = 0, new_size = 0;
	u_int64_t start = 0, stop = 0;
	u_int64_t malloc_sum = 0, malloc_cpt = 0;
	u_int64_t calloc_sum = 0, calloc_cpt = 0;
	u_int64_t realloc_sum = 0, realloc_cpt = 0;
	u_int64_t free_sum = 0, free_cpt = 0;
	u_int64_t write_sum = 0, write_cpt = 0;
	u_int64_t read_sum = 0, read_cpt = 0;

	for(int i = 0; i < N; i++){
		size = rand_a_b(1,M);
		size_tab[i] = size;

		if(rand_a_b(0,2) == 0){
			start = rdtsc();
			tab[i] = malloc(sizeof(int) * size);
			stop = rdtsc();
			
			malloc_sum += stop - start;
			malloc_cpt += sizeof(int) * size;
		
			for(int z = 0; z < size; z++){
				tab[i][z] = z;}
		}else{
			start = rdtsc();
			tab[i] = calloc(size, sizeof(int));
			stop = rdtsc();
			
			calloc_sum += stop - start;
			calloc_cpt += sizeof(int) * size;
		}

		if(rand_a_b(0,5) == 0){
			new_size = rand_a_b(1,M);
			int j = rand_a_b(0,i);
			if(tab[j] != NULL){
				start = rdtsc();
				tab[j] = realloc(tab[j], sizeof(int) * new_size);
				stop = rdtsc();
			
				realloc_sum += stop - start;
				realloc_cpt += sizeof(int) * new_size;

				size_tab[j] = new_size;
			}
		}

		if(rand_a_b(0,5) == 0){
			int j = rand_a_b(0,i);
			if(tab[j] != NULL){
				start = rdtsc();
				free(tab[j]);
				stop = rdtsc();
			
				tab[j] = NULL;	
				
				free_cpt += size_tab[j];
				free_sum += stop - start;
			}
		}

	}

	for(int i = 0; i < N; ++i){
		if(tab[i] != NULL){
			for(int z = 0; z < size_tab[i]; z++){
				start = rdtsc();
				tab[i][z];
				stop = rdtsc();
				
				read_cpt += sizeof(int);
				read_sum += stop - start;				
			
				start = rdtsc();
				tab[i][z] = z;
				stop = rdtsc();
				
				write_sum += stop - start;
				write_cpt+= sizeof(int);
			}
		}
	}

	for(int i = 0; i < N; i++){
		if(tab[i] != NULL){
			free_cpt += size_tab[i];
			
			start = rdtsc();
			free(tab[i]);
			stop = rdtsc();
			
			tab[i] = NULL;	
			free_sum += stop - start;
		}
	}

	printf("%f\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\n",													\
			log10(M),																				\
			(double) malloc_sum/malloc_cpt, 														\
			(double) calloc_sum/calloc_cpt,															\
			(double) realloc_sum/realloc_cpt,														\
			(double) free_sum/free_cpt,																\
			(double) write_sum/write_cpt,															\
			(double) read_sum/read_cpt);

	return 0;
}
