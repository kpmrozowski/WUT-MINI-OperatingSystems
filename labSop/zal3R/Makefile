all: etap2
etap2: etap2.c
	gcc -pthread -g -Wall -fsanitize=address,undefined -o etap2 etap2.c
.PHONY: clean
clean:
	rm etap2
