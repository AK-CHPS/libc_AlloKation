#!/bin/bash

gcc -D N=200 perf_test.c -o perf_test -O3
./perf_test 000000

gcc -D N=200 -D AK perf_test.c -o perf_test -L/usr/local/lib/ -I/usr/local/include/ -lmAlloK -O3
./perf_test $i