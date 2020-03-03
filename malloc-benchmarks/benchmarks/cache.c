/*
 *  cache
 *  cel - Wed Mar 15 15:55:30 EST 2000
 *
 *  Benchmark libc's malloc.  We measure the impact of
 *  Level 1 cache misalignment, and how much malloc
 *  contributes to this.
 *
 *  Syntax:
 *  cache [ <size> [<iterations> [<thread count]]]
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include <pthread.h>

#define USECSPERSEC (1000000)
#define pthread_attr_default NULL
#define MAX_THREADS (50)
#define L1_CACHE_LINE_BYTES (32)
#define L1_CACHE_ALIGN(x) ((((x) / L1_CACHE_LINE_BYTES) * L1_CACHE_LINE_BYTES) + L1_CACHE_LINE_BYTES)

void run_test(char *);

static size_t size = 15;
static unsigned long iteration_count = 100000000;

int main(int argc, char *argv[])
{
	unsigned i, arena_size;
	unsigned thread_count = 2;
	unsigned long next;
	struct timeval start, end, elapsed;
	void * arena;
	void * object[MAX_THREADS + 1];
	pthread_t thread[MAX_THREADS + 1];

	/*
	 * Parse our arguments
	 */
	switch (argc) {
	case 4:
		/* size, iteration count, and thread count were specified */
		thread_count = atoi(argv[3]);
		if (thread_count > MAX_THREADS) thread_count = MAX_THREADS;
	case 3:
		/* size and iteration count were specified; others default */
		iteration_count = atoi(argv[2]);
	case 2:
		/* size was specified; others default */
		size = atoi(argv[1]);
	case 1:
		/* use default values */
		break;
	default:
		printf("Unrecognized arguments.\n");
		exit(1);
	}

	/*
	 * Invoke the tests
	 */
	printf("Testing carefully aligned %ld byte objects...\n", size);

	arena_size = (L1_CACHE_ALIGN(size) * thread_count) + L1_CACHE_LINE_BYTES;
	printf(" arena size is %d bytes\n", arena_size);
	arena = malloc(arena_size);

	next = (unsigned long) arena;
	for (i=1; i<=thread_count; i++) {
		next = L1_CACHE_ALIGN(next);
		object[i] = (void *) next;
		next += size;
	}

	gettimeofday(&start, NULL);

	for (i=1; i<=thread_count; i++)
		if (pthread_create(&(thread[i]), pthread_attr_default,
					(void *) &run_test, object[i]))
			printf("failed.\n");

	/*
	 * Wait for tests to finish
	 */
	for (i=1; i<=thread_count; i++)
		pthread_join(thread[i], NULL);

	gettimeofday(&end, NULL);
	elapsed.tv_sec = end.tv_sec - start.tv_sec;
	elapsed.tv_usec = end.tv_usec - start.tv_usec;
	if (elapsed.tv_usec < 0) {
		elapsed.tv_sec--;
		elapsed.tv_usec += USECSPERSEC;
	}

	printf(" >> Elapsed time: %lu iterations in %ld.%06ld seconds.\n\n",
		iteration_count, elapsed.tv_sec, elapsed.tv_usec);

	free(arena);

	printf("Testing normally aligned %ld byte objects...\n", size);

	for (i=1; i<=thread_count; i++)
		if (!(object[i] = malloc(size))) {
			fprintf(stderr, "malloc failed\n");
			exit(1);
		}

	gettimeofday(&start, NULL);

	for (i=1; i<=thread_count; i++)
		if (pthread_create(&(thread[i]), pthread_attr_default,
					(void *) &run_test, object[i]))
			printf("failed.\n");

	/*
	 * Wait for tests to finish
	 */
	for (i=1; i<=thread_count; i++)
		pthread_join(thread[i], NULL);

	gettimeofday(&end, NULL);
	elapsed.tv_sec = end.tv_sec - start.tv_sec;
	elapsed.tv_usec = end.tv_usec - start.tv_usec;
	if (elapsed.tv_usec < 0) {
		elapsed.tv_sec--;
		elapsed.tv_usec += USECSPERSEC;
	}

	printf(" >> Elapsed time: %lu iterations in %ld.%06ld seconds.\n\n",
		iteration_count, elapsed.tv_sec, elapsed.tv_usec);

	for (i=1; i<=thread_count; i++)
		free(object[i]);

	exit(0);
}

void run_test(char * object)
{
	unsigned long i;
	register size_t end = size - 1;

	printf(" thread %ld: object address is %p\n", pthread_self(),
	       /*(unsigned)*/ object);

	for (i=0; i<iteration_count; i++) {
		object[0] = 1;
		object[end] = 1;
	}

	pthread_exit(NULL);
}
