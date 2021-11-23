all: prog15
prog15: prog15.c
	gcc -g -Wall -fsanitize=address,undefined -o prog15 prog15.c
.PHONY: clean
clean:
	rm prog15
