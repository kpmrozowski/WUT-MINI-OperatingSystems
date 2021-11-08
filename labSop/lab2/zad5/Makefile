all: prog5
prog5: prog5.c
	gcc -g -Wall -fsanitize=address,undefined -o prog5 prog5.c
.PHONY: clean
clean:
	rm prog5
