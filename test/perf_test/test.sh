#!/bin/bash

rm -f dat/OUTPUT_AK.dat
rm -f dat/OUTPUT_libc.dat

i=10

while [ $i -lt 1000000 ]
do
	echo "i=$i sur 1000000"

	gcc -D N=1000 -D M=$i perf_test.c -o perf_test -O3
	./perf_test >> dat/OUTPUT_libc.dat &

	gcc -D N=1000 -D M=$i perf_test.c -o perf_test -O3
	LD_PRELOAD=../../build/libmAlloK_preload_wrapper.so ./perf_test >> dat/OUTPUT_AK.dat &

	i=$(( $i * 2))
done
