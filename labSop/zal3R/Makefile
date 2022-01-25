all: etap4
etap4: etap4.c
	gcc -pthread -g -Wall -fsanitize=address,undefined -o etap4 etap4.c
.PHONY: clean
clean:
	rm etap4
