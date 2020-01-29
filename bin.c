#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <assert.h>
#include <string.h>
#include <math.h>

int fct_hachage(size_t size)
{
	if(size <= 512){
		return floor(size/8);	/* OK */
	}else{
		return (int)floor(log2(size)*7)-1;
	}
}

int main(int argc, char const *argv[])
{
	int cpt = 0;
	for(int i = 1; i < 65; i++){
		printf("%d %d %d\n", cpt, i*8, fct_hachage(i*8));
		cpt++;
	}

	for(int i = 65; i < 219; i++){
		unsigned long long int a = ceil(pow(2,i*(1.0/7.0)));
		if(a % 8 != 0){
			a += (8-(a%8));}
		printf("%d %lld %d\n", cpt, a, fct_hachage(a));
		cpt++;
	}

	for(size_t s = 512; s <= 1024; s+=3){
		printf("%zu %d\n", s, fct_hachage(s));
	}

	return 0;
}

//217 2371014177
//2^31 2147483648