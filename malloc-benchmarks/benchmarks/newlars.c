/*
 *  newlars
 *  cel - Fri Mar 24 15:26:58 EST 2000
 *
 *  Benchmark libc's malloc.  Based on test driver for
 *  malloc written by Paul Larson, palarson@microsoft.com.
 *
 *  Syntax:
 *  time ./newlars [<blk_size> [<threads> [<blks/thread> [<rounds>]]]]
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <assert.h>
#include <sys/time.h>

#include <pthread.h>

#define pthread_attr_default NULL

#ifndef TRUE
enum { TRUE = 1, FALSE = 0 };
#endif

#define MAX_THREADS 100
#define MAX_BLOCKS 1000001

static int blk_size = 40;

/* lran2.h
 * by Wolfram Gloger 1996.
 *
 * A small, portable pseudo-random number generator.
 */

#define LRAN2_MAX 714025l /* constants for portable */
#define IA	  1366l	  /* random number generator */
#define IC	  150889l /* (see e.g. `Numerical Recipes') */

struct lran2_st {
	long x, y, v[97];
};

static struct lran2_st rgen;

static void lran2_init(struct lran2_st* d, long seed)
{
	long x;
	int j;

	x = (IC - seed) % LRAN2_MAX;
	if(x < 0) x = -x;
	for(j=0; j<97; j++) {
		x = (IA*x + IC) % LRAN2_MAX;
		d->v[j] = x;
	}
	d->x = (IA*x + IC) % LRAN2_MAX;
	d->y = d->x;
}

static long lran2(struct lran2_st* d)
{
	int j = (d->y % 97);

	d->y = d->v[j];
	d->x = (IA*d->x + IC) % LRAN2_MAX;
	d->v[j] = d->x;

	return d->y;
}

#undef IA
#undef IC

typedef struct thr_data {
	int	num_rounds;
	int	finished;
	int	seed;
	void **	array;
	int	asize;
	int	cAllocs;
	int	cFrees;
	struct lran2_st	rgen;
} thread_data;

static void *exercise_heap(void *pinput)
{
	thread_data *pdea = (thread_data *)pinput;
	int cblks=0;
	int victim;
	int range;
	pthread_t pt;
	int retval;

	/*
	 * Finish this sequence of threads after num_rounds rounds
	 */
	if (!pdea->num_rounds--) {
		pdea->finished = TRUE;
		pthread_exit(&retval);
	}

	/*
	 * Randomly replace a portion of the blocks in the array
	 */
	for (cblks=0; cblks < ((pdea->asize * 2)/ 3); cblks++) {
		victim = lran2(&pdea->rgen) % pdea->asize;
		free(pdea->array[victim]);
		pdea->cFrees++ ;

		pdea->array[victim] = malloc(blk_size);
		assert(pdea->array[victim] != NULL);
		pdea->cAllocs++ ;
	}

	/*
	 * Pass the array to another thread; lather, rinse, repeat.
	 */
	retval = pthread_create(&pt, pthread_attr_default,
				(void *) exercise_heap, pdea);
	if (retval) {
		pdea->finished = TRUE;
		printf("pthread_create returned %d\n", retval);
	}

	pthread_exit(&retval);
}

static void warmup(void **blkp, int num_chunks)
{
	int cblks ;
	int victim ;
	void *tmp ;

	/*
	 * Allocate all objects for this array.
	 */
	for (cblks=0; cblks < num_chunks; cblks++) {
		blkp[cblks] = malloc(blk_size);
		assert(blkp[cblks] != NULL) ;
	}

	/*
	 * Generate a random permutation of the objects.
	 */
	for (cblks=num_chunks; cblks > 0; cblks--) {
		victim = lran2(&rgen) % cblks;
		tmp = blkp[victim];
		blkp[victim]  = blkp[cblks-1];
		blkp[cblks-1] = tmp;
	}

	for (cblks=0; cblks < 4*num_chunks; cblks++) {
		victim = lran2(&rgen) % num_chunks;
		free(blkp[victim]);

		blkp[victim] = malloc(blk_size);
		assert(blkp[victim] != NULL);
	}
}

static void runthreads(int num_threads, int chperthread, int num_rounds)
{
	int i, sum_allocs, sum_frees, finished;
	long ticks_per_sec, start_cnt, end_cnt;
	long long ticks;
	double duration;
	pthread_t pt;
	thread_data *tdescs;

	/*
	 * Allocate thread descriptor area.
	 */
	tdescs = malloc(num_threads * sizeof(thread_data));
	assert(tdescs != NULL);

	/*
	 * Populate descriptor area and start all threads.
	 */
	for(i=0; i < num_threads; i++) {
		tdescs[i].num_rounds = num_rounds;
		tdescs[i].asize = chperthread;
		tdescs[i].array = malloc(chperthread * sizeof(void *));
		assert(tdescs[i].array != NULL);
		warmup(tdescs[i].array, chperthread);

		tdescs[i].finished = FALSE;
		tdescs[i].cAllocs = 0;
		tdescs[i].cFrees = 0;

		tdescs[i].seed = lran2(&rgen);
		lran2_init(&tdescs[i].rgen, tdescs[i].seed);

		pthread_create(&pt, pthread_attr_default,
			(void *) exercise_heap, &tdescs[i]);
	}

	printf(" Running test...\n");

	/*
	 * Wait for all threads to complete all rounds
	 */
	finished = 0;
	while (finished != num_threads) {
		finished = 0;
		sleep(3);
		for (i=0; i < num_threads; i++) {
			if (tdescs[i].finished)
				finished++;
		}
	}

	free(tdescs);
	return;
}

int main(int argc, char *argv[])
{
	int num_threads = 3;
	int num_rounds = 10;
	int chperthread = 1;
	struct timeval seed;

	printf("\nMulti-threaded test driver \n");

	gettimeofday(&seed, NULL);
	lran2_init(&rgen, (unsigned) (seed.tv_usec & 0xffff)) ;

	/*
	 * Parse our arguments
	 */
	switch (argc) {
	case 5:
		/* number of rounds */
		num_rounds = atoi(argv[4]);
	case 4:
		/* blocks per thread */
		chperthread = atoi(argv[3]);
	case 3:
		/* number of threads */
		num_threads = atoi(argv[2]);
	case 2:
		/* request size */
		blk_size = atoi(argv[1]);
	case 1:
		/* use default values */
		break;
	default:
		printf("Unrecognized arguments.\n");
		exit(1);
	}

	/*
	 * Linux pthread_create will only create 1024 threads, even if
	 * some of the created threads are already dead.
	 */
	if ((num_rounds * num_threads) > 1020) {
		printf("Benchmark would create too many threads.\n");
		exit(1);
	}

	printf("  Block size is %d bytes; ", blk_size);
	printf(" Each thread will receive a %d block array.\n",
		chperthread);
	printf("  %d thread(s) will be created %d times.\n",
		num_threads, num_rounds);

	runthreads(num_threads, chperthread, num_rounds) ;

	exit(0) ;
}