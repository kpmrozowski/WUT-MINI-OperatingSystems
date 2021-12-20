all: stage1
stage1: stage1.c
	gcc -pthread -g -Wall -fsanitize=address,undefined -o stage1 stage1.c
.PHONY: clean
clean:
	rm stage1
