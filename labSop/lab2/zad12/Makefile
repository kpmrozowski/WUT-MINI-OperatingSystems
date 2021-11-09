all: prog12
prog12: prog12.c
	gcc -g -Wall -fsanitize=address,undefined -o prog12 prog12.c
.PHONY: clean
clean:
	rm prog12
