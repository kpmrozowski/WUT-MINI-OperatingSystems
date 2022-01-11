all: prog20
prog20: prog20.c
	gcc -g -Wall -fsanitize=address,undefined -o prog20 prog20.c -lpthread -lm
.PHONY: clean
clean:
	rm prog20
CC=gcc
CFLAGS= -std=gnu99 -Wall -fsanitize=thread,undefined
LDFLAGS=-fsanitize=thread,undefined
LDLIBS = -lpthread -lm
