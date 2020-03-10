lib_test.DEFAULT_GOAL := default

default: lib 

lib: mAlloK.c
	gcc -O3 -fPIC -shared -o libmAlloK.so mAlloK.c -lm -lpthread

install: uninstall libmAlloK.so libmAlloK.h
	cp libmAlloK.so /usr/local/lib/
	cp libmAlloK.h /usr/local/include/

uninstall:
	rm -f /usr/local/lib/libmAlloK.so
	rm -f /usr/local/include/libmAlloK.h

installcheck: ../lib_test.c
	gcc -O3 -o ../lib_test ../lib_test.c -L/usr/local/lib/ -I/usr/local/include/  -lmAlloK
	echo | ../lib_test
