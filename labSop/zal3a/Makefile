all: etap5
etap5: etap5.c
	gcc -pthread -g -Wall -fsanitize=address,undefined -o etap5 etap5.c
.PHONY: clean
clean:
	rm etap5
