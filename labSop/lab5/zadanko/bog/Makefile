all: main
main: main.c
	gcc -g -Wall -fsanitize=address,undefined -o main main.c -lpthread -lm -lrt
.PHONY: clean
clean:
	rm main
CC=gcc
CFLAGS= -std=gnu99 -Wall -fsanitize=thread,undefined
LDFLAGS=-fsanitize=thread,undefined
LDLIBS = -lpthread -lm
