#!/bin/bash

rm -f OUTPUT/test_perf_mAlloK.dat
rm -f OUTPUT/test_perf_malloc.dat

i=100

while [ $i -lt 10000000 ]
do
	echo "i=$i sur 10000000"
	gcc -D N=50 -D WARN=0 -D PERSO=1 -D M=$i test.c -o test -O3
	./test >> OUTPUT/test_perf_mAlloK.dat

	gcc -D N=50 -D WARN=0 -D PERSO=0 -D M=$i test.c -o test -O3
	./test >> OUTPUT/test_perf_malloc.dat

	i=$(( $i * 2))
done
