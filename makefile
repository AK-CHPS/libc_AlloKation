alan: alan_test.c
	gcc alan_test.c -o alan_test -Wall -lm

killian: killian_test.c
	gcc killian_test.c -o killian_test -Wall

strassen: strassen.c
	gcc strassen.c -o strassen -O3 -Wall -lpthread