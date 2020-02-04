.DEFAULT_GOAL := default

default: lib 

lib: mAlloK.c
	gcc -O3 -fPIC -shared -o libmAlloK.so mAlloK.c -lm

install: uninstall libmAlloK.so libmAlloK.h
	cp libmAlloK.so /usr/local/lib/
	cp libmAlloK.h /usr/local/include/

uninstall:
	rm -f /usr/local/lib/libmAlloK.so
	rm -f /usr/local/include/libmAlloK.h

installcheck: ../lib_test.c
	gcc -O3 -o ../lib_test ../lib_test.c -I/usr/local/include/ -L/usr/local/lib/ -lmAlloK
	echo | ../lib_test
