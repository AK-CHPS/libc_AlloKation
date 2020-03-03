#
#  Makefile for malloc test suite
#

CC=cc
#CFLAGS=-g
CFLAGS=-O3 -fomit-frame-pointer -fno-inline -fno-strength-reduce

all: malloc-test newlars cache

cache: cache.o
	cc -o cache cache.o -lpthread

cache.o: cache.c
	$(CC) $(CFLAGS) -c cache.c

newlars: newlars.o
	cc -o newlars newlars.o -lpthread

newlars.o: newlars.c
	$(CC) $(CFLAGS) -c newlars.c

malloc-test: malloc-test.o
	cc -o malloc-test malloc-test.o -lpthread

malloc-test.o: malloc-test.c
	$(CC) $(CFLAGS) -c malloc-test.c

clean:
	rm -f malloc-test malloc-test.o newlars newlars.o cache cache.o
