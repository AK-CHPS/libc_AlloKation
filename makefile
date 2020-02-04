alan: alan_test.c
	gcc alan_test.c -o alan_test -Wall -lm

strassen: strassen.c
	gcc strassen.c -o strassen -O3 -Wall -lpthread

bin: bin.c
	gcc bin.c -o bin -lm -Wall

test: test.c 
	gcc test.c -o test -lm -Wall

lib_test: lib_test.c
	gcc -Wall -o lib_test lib_test.c -lmAlloK -lm
