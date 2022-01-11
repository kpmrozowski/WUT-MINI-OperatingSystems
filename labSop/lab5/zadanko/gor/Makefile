all: dryf_2e
dryf_2e: dryf_2e.c
	gcc -g -Wall -fsanitize=address,undefined -o dryf_2e dryf_2e.c -lpthread -lm -lrt
.PHONY: clean
clean:
	rm dryf_2e
CC=gcc
CFLAGS= -std=gnu99 -Wall -fsanitize=thread,undefined
LDFLAGS=-fsanitize=thread,undefined
LDLIBS = -lpthread -lm
