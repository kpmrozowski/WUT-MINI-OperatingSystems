all: stage2
stage2: stage2.c
	gcc -g -Wall -pthread -fsanitize=address,undefined -o stage2 stage2.c
.PHONY: clean
clean:
	rm stage2
