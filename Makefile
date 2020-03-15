DEFAULT_GOAL := default
CC?=gcc

default: lib preload

lib: ./src/mAlloK.c
	$(CC) -O3 -fPIC -shared -o ./build/libmAlloK.so ./src/mAlloK.c -lm -lpthread

preload: ./src/headers/preload.h
	$(CC) -O3 -fPIC -shared -o ./build/libmAlloK_preload_wrapper.so ./src/mAlloK_preload_wrapper.c ./src/mAlloK.c -lm -lpthread

install: uninstall ./build/libmAlloK.so ./src/headers/mAlloK.h
	cp ./build/libmAlloK.so /usr/local/lib/
	cp ./src/headers/mAlloK.h /usr/local/include/

uninstall:
	rm -f /usr/local/lib/libmAlloK.so
	rm -f /usr/local/include/mAlloK.h

