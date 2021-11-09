all: prog
prog: prog.c
	gcc -g -Wall -fsanitize=address,undefined -o prog prog.c
.PHONY: clean
clean:
	rm prog
