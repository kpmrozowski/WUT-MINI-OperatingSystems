all: lab4_tutF
lab4_tutF: lab4_tutF.c
	gcc -g -Wall -fsanitize=address,undefined -o lab4_tutF lab4_tutF.c -lpthread -lm -lrt
.PHONY: clean
clean:
	rm lab4_tutF
CC=gcc
CFLAGS= -std=gnu99 -Wall -fsanitize=thread,undefined
LDFLAGS=-fsanitize=thread,undefined
LDLIBS = -lpthread -lm
