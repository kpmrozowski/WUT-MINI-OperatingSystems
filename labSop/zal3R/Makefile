all: etap1
etap1: etap1.c
	gcc -pthread -g -Wall -fsanitize=address,undefined -o etap1 etap1.c
.PHONY: clean
clean:
	rm etap1
