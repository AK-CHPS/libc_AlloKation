#!/bin/bash

gcc strassen.c -o strassen -O3
./strassen $1

gcc strassen.c -o strassen -L/usr/local/lib/ -I/usr/local/include/ -lmAlloK -O3
./strassen $1