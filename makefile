CC = gcc
CFLAGS = -Wall -Werror

all: hello

hello: main.c
	$(CC) $(CFLAGS) main.c -o hello

run: hello
	./hello

clean:
	rm -f hello