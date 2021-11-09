all: prog11
prog11: prog11.c
	gcc -g -Wall -fsanitize=address,undefined -o prog11 prog11.c
.PHONY: clean
clean:
	rm prog11
