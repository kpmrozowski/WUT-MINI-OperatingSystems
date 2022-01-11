all: etap2
etap2: etap2.c
	gcc -g -Wall -pthread -fsanitize=address,undefined -lrt -o etap2 etap2.c
.PHONY: clean
clean:
	rm etap2
