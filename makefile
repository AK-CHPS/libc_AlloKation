alan: alan_test.c
	gcc alan_test.c -o alan_test -Wall -lm

strassen: strassen.c
	gcc strassen.c -o strassen -O3 -Wall -lpthread
