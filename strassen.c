#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>

typedef unsigned long long int u_64;
typedef unsigned int u_32;

static u_64 allocation_time = 0;
static u_64 allocation_cpt = 0;
static u_64 free_time = 0;
static u_64 free_cpt = 0;

static inline u_64 rdtsc (void)
{
  //64 bit variables
  unsigned long long a, d;

  __asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
  
  return (d << 32) | a;
}

int rand_a_b(int a, int b)
{
	return rand()%(b-a)+a;
}

int **alloue(u_64 n)
{
	u_64 start = rdtsc();
	int ** t = (int**) malloc(sizeof(int*)*n);
	t[0] = (int*)malloc(sizeof(int)*n*n);
	allocation_time += (rdtsc() - start);
	allocation_cpt++;

	for(u_64 i = 1; i < n; i++)
		t[i] = t[i-1]+n;

	for(u_64 i = 0; i < n; i++){
		for(int j = 0; j < n; j++){
			t[i][j] = rand_a_b(0,10);
		}
	}

	return t;
}

void libere(int **m)
{
	u_64 start = rdtsc();
	free(m);
	free_time += (rdtsc() - start);
	free_cpt++;
}

void print(const int ** t, u_64 n)
{
	for(u_64 i = 0; i < n; i++){
		for(u_64 j = 0; j < n; j++){
			printf("%d ", t[i][j]);
		}
		printf("\n");
	}
	printf("\n");
}


void add_matrix(int **A, int **B, int **C, u_64 n)
{
	for(int i = 0; i < n; i++){
		for(int j = 0; j < n; j++){
			C[i][j] = A[i][j] + B[i][j]; 		
		}
	}
}

void sub_matrix(int **A, int **B, int **C, u_64 n)
{
	for(int i = 0; i < n; i++){
		for(int j = 0; j < n; j++){
			C[i][j] = A[i][j] - B[i][j]; 		
		}
	}	
}

void mult_matrix(int **A, int **B, int **C, u_64 n)
{
	if(n == 1){
		C[0][0] = A[0][0] * B[0][0];
		return;
	}

	u_64 half = n/2;

	int **M1 = alloue(half);
	int **M1a = alloue(half);
	int **M1b = alloue(half);
	int **M2 = alloue(half);
	int **M2a = alloue(half);
	int **M2b = alloue(half);
	int **M3 = alloue(half);
	int **M3a = alloue(half);
	int **M3b = alloue(half);
	int **M4 = alloue(half);
	int **M4a = alloue(half);
	int **M4b = alloue(half);
	int **M5 = alloue(half);
	int **M5a = alloue(half);
	int **M5b = alloue(half);
	int **M6 = alloue(half);
	int **M6a = alloue(half);
	int **M6b = alloue(half);
	int **M7 = alloue(half);
	int **M7a = alloue(half);
	int **M7b = alloue(half);

	/* M1 */
	for(u_64 i = 0; i < half; i++){
		for(u_64 j = 0; j < half; j++){
			M1a[i][j] = A[i][j] + A[half+i][half+j];
			M1b[i][j] = B[i][j] + B[half+i][half+j];
		}
	}
	mult_matrix(M1a, M1b, M1, half);

	/* M2 */
	for(u_64 i = 0; i < half; i++){
		for(u_64 j = 0; j < half; j++){
			M2a[i][j] = A[half+i][j] + A[half+i][half+j];
			M2b[i][j] = B[i][j];
		}
	}
	mult_matrix(M2a, M2b, M2, half);

	/* M3 */
	for(u_64 i = 0; i < half; i++){
		for(u_64 j = 0; j < half; j++){
			M3a[i][j] = A[i][j];
			M3b[i][j] = B[i][half+j] - B[half+i][half+j];
		}
	}
	mult_matrix(M3a, M3b, M3, half);

	/* M4 */
	for(u_64 i = 0; i < half; i++){
		for(u_64 j = 0; j < half; j++){
			M4a[i][j] = A[half+i][half+j];
			M4b[i][j] = B[half+i][j] - B[i][j];
		}
	}
	mult_matrix(M4a, M4b, M4, half);

	/* M5 */
	for(u_64 i = 0; i < half; i++){
		for(u_64 j = 0; j < half; j++){
			M5a[i][j] = A[i][j] + A[i][half+j];
			M5b[i][j] = B[half+i][half+j];
		}
	}
	mult_matrix(M5a, M5b, M5, half);

	/* M6 */
	for(u_64 i = 0; i < half; i++){
		for(u_64 j = 0; j < half; j++){
			M6a[i][j] = A[half+i][j] - A[i][j];
			M6b[i][j] = B[i][j] + B[i][half+j];
		}
	}
	mult_matrix(M6a, M6b, M6, half);

	/* M7 */
	for(u_64 i = 0; i < half; i++){
		for(u_64 j = 0; j < half; j++){
			M7a[i][j] = A[i][half+j] - A[half+i][half+j];
			M7b[i][j] = B[half+i][j] + B[half+i][half+j];
		}
	}
	mult_matrix(M7a, M7b, M7, n/2);

	/* C1 */
	for(u_64 i = 0; i < half; i++){
		for(u_64 j = 0; j < half; j++){
			C[i][j] = M1[i][j] + M4[i][j] - M5[i][j] + M7[i][j];
		}
	}

	/* C2 */
	for(u_64 i = 0; i < half; i++){
		for(u_64 j = 0; j < half; j++){
			C[i][half+j] = M3[i][j] + M5[i][j];
		}
	}

	/* C3 */
	for(u_64 i = 0; i < half; i++){
		for(u_64 j = 0; j < half; j++){
			C[half+i][j] = M2[i][j] + M4[i][j];
		}
	}

	/* C4 */
	for(u_64 i = 0; i < half; i++){
		for(u_64 j = 0; j < half; j++){
			C[half+i][half+j] = M1[i][j] - M2[i][j] + M3[i][j] + M6[i][j];
		}
	}

	libere(M1);
	libere(M2);
	libere(M3);
	libere(M4);
	libere(M5);
	libere(M6);
	libere(M7);
}

void compare(int **A, int **B, u_64 n)
{
	for(u_64 i = 0; i < n; i++){
		for(u_64 j = 0; j < n; j++){
			if(A[i][j] != B[i][j]){
				printf("Erreur [%lld][%lld]\n", i, j);
				print(A,n);
				print(B,n);
				return;
			}
		}
	}

	printf("\nSUCCES!!!!!\n\n");
}


int main(int argc, char const *argv[])
{
	srand(time(NULL));

	if(argc != 2){
		fprintf(stderr, "./strassen <size>\n");
		return 1;
	}else if((atol(argv[1]) & (atol(argv[1])-1)) != 0){
		fprintf(stderr, "size must be a power of two\n");
		return 1;
	}

	u_64 n = atol(argv[1]);

	/* Etape 1 : initialisation */
	int **A = alloue(n);
	int **B = alloue(n);
	int **C = alloue(n);
	int **D = alloue(n);
	
	mult_matrix(A, B, C, n);

	for(u_64 i = 0; i < n; i++){
		for(u_64 j = 0; j < n; j++){
			D[i][j] = 0;
			for(u_64 k = 0; k < n; k++){
				D[i][j] += A[i][k] * B[k][j];
			}
		}
	}

	compare(C, D, n);

	libere(A);
	libere(B);
	libere(C);
	libere(D);

	printf("nb de cycle moyenn par d'allocation: %lld/%lld = %lf cycles\n", allocation_time, allocation_cpt, (double)allocation_time/allocation_cpt);
	printf("nb de cycle moyenn par liberation: %lld/%lld = %lf  cycles\n", free_time, free_cpt, (double)free_time/free_cpt);

	return 0;
}