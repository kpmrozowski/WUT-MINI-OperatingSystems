CC = gcc
CFLAGS = -std=gnu99 -Wall
LDFLAGS = -Wall
LDLIBS = -lpthread -lm
SRCS = interface.c main.c
OBJS = $(subst .c,.o,$(SRCS))

all: main

main: $(OBJS)
	$(CC) $(LDFLAGS) -o main $(OBJS) $(LDLIBS)

main.o: main.c interface.h
	$(CC) $(CFLAGS) -c main.c

interface.o: interface.c interface.h
	$(CC) $(CFLAGS) -c interface.c

.PHONY: clean

clean:
	rm main main.o interface.o
