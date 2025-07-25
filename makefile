CC = gcc
CFLAGS = -Wall -Werror -fsanitize=address

all: mysh

mysh: mysh.c
	$(CC) $(CFLAGS) -o mysh mysh.c
	


clean:
	rm -f mysh
