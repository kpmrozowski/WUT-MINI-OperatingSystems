all: prog20
prog20: prog20.c
	gcc -pthread -g -Wall -fsanitize=address,undefined -o prog20 prog20.c
.PHONY: clean
clean:
	rm prog20
