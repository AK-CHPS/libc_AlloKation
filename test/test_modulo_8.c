#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <time.h>
#include <unistd.h>

#ifndef N
	#define N 100
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

static size_t next_mod_8(size_t size)
{
	return ((size+7) & (-8));
}

int main(int argc, char const *argv[])
{
	srand(time(NULL)); 
	size_t cycle_cpt = 0, start = 0, stop = 0;

	for(int i = 0; i < N; i++){
		size_t old_val = rand_a_b(0,100000);
		size_t new_val = 0;
		start = rdtsc();
		new_val = next_mod_8(old_val);
		stop = rdtsc();
		printf("old_val = %zu modulo 8 = %zu et new_val = %zu modulo 8 = %zu\n", old_val, old_val%8, new_val, new_val%8);
		cycle_cpt += stop - start;
	}

	printf("nombre de cycle moyen = %lf\n", (double)cycle_cpt/N);

	
	return 0;
}