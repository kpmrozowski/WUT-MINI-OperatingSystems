all: sopL4copy
sopL4copy: sopL4copy.c
	gcc -g -Wall -fsanitize=address,undefined -o sopL4copy sopL4copy.c -lpthread -lm
.PHONY: clean
clean:
	rm sopL4copy
CC=gcc
CFLAGS= -std=gnu99 -Wall -fsanitize=thread,undefined
LDFLAGS=-fsanitize=thread,undefined
LDLIBS = -lpthread -lm
