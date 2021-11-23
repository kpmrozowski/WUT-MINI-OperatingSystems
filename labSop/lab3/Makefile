all: prog16a
prog16a: prog16a.c
	gcc -g -Wall -fsanitize=address,undefined -o prog16a prog16a.c
.PHONY: clean
clean:
	rm prog16a
