#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <assert.h>
#include <string.h>
#include <math.h>

extern __inline__ u_int64_t rdtsc(void) {
	u_int64_t a, d;
	__asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
	return (d<<32) | a;
}

static double step = 22.0/(217.0-63.0);
static double inv_step = (217.0-63.0)/22.0;

static inline int fct_hachage(size_t size)
{
	if(size >= 512){
		return log2(size+1)*inv_step; /* OK */
	}else{
		return size>>3;	/* OK */
	}
}




#define N 10000

int main(int argc, char const *argv[])
{
	int cpt = 0;
	u_int64_t start = 0, stop = 0, sum = 0;

	for(int i = 0; i < 64; i++){
		size_t s = i*8;
		for(int j = 0; j < N; j++){
			start = rdtsc();
			fct_hachage(s);
			stop = rdtsc();
			sum += stop - start;}
		printf("%d %d %d %d\n", s, cpt++, (s)%8, fct_hachage(s));
		printf("\t%d %d %d\n", fct_hachage(s-1), fct_hachage(s), fct_hachage(s+1));
	}

	for(int i = 64; i < 218; i++)
	{
		size_t s = (unsigned int) pow(2,i*step);
		for(int j = 0; j < N; j++){
			start = rdtsc();
			fct_hachage(s);
			stop = rdtsc();
			sum += stop - start;}
		printf("%u %d %d\n", s, cpt++,s%8);
		printf("\t%d %d %d\n", fct_hachage(s-1), fct_hachage(s), fct_hachage(s+1));
	}

	printf("%f\n", (float)sum/((cpt+1)*N));

	return 0;
}

//217 	2236853350
//2^31 	2147483648
//		2147483648