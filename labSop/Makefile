all: prog18
prog18: prog18.c
	gcc -g -Wall -fsanitize=address,undefined -o prog18 prog18.c -lpthread -lm
.PHONY: clean
clean:
	rm prog18
CC=gcc
CFLAGS= -std=gnu99 -Wall -fsanitize=thread,undefined
LDFLAGS=-fsanitize=thread,undefined
LDLIBS = -lpthread -lm
