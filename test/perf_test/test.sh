#!/bin/bash

gcc -D N=100 perf_test.c -o perf_test -O3
./perf_test 000000

gcc -D N=200 -D AK perf_test.c -o perf_test -I/usr/local/include/ -L/usr/local/lib/ -lmAlloK -O3
./perf_test $i