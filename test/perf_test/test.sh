#!/bin/bash

rm -f OUTPUT_AK.dat
rm -f OUTPUT_libc.dat

i=10

while [ $i -lt 10000000 ]
do
	echo "i=$i sur 10000000"

	gcc -D N=50 -D M=$i perf_test.c -o perf_test -O3 -lm
	./perf_test >> OUTPUT_libc.dat &

	gcc -D N=50 -D AK -D M=$i perf_test.c -o perf_test -L/usr/local/lib/ -I/usr/local/include/ -lm -lmAlloK -O3
	./perf_test >> OUTPUT_AK.dat &	

	i=$(( $i * 2))
done