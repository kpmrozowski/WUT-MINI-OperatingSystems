all: stage1
stage1: stage1.c
	gcc -g -Wall -fsanitize=address,undefined -o stage1 stage1.c
.PHONY: clean
clean:
	rm stage1
