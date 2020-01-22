#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void sort(int *t, int n)
{
	for(int i = 1; i < n; i++){
		for(int j = i; j >= 1 && t[j] <= t[j-1]; j--){
			int tmp = t[j-1];
			t[j-1] = t[j];
			t[j] = tmp;			
		}
	}
}

int rand_a_b(int a, int b)
{
	return rand()%(b-a)+a;
}

int main(int argc, char const *argv[])
{
	srand(time(NULL));
	int n = atoi(argv[1]);
	int *t = malloc(sizeof(int)*n);

	for(int i = 0; i < n; i++){
		t[i] = rand_a_b(0,15);}

	sort(t,n);

	for(int i = 0; i < n; i++){
		printf("%d\n", t[i]);
	}
	printf("\n");

	return 0;
}