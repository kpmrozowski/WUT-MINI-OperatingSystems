all: prog3
prog3: prog3.c
	gcc -g -Wall -fsanitize=address,undefined -o prog3 prog3.c
.PHONY: clean
clean:
	rm prog3
