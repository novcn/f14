#makefile
# compiler = gcc

CC=g++

CFLAGS=-Wall -W -g -Werror

all: client server

client: client.c raw.c
	$(CC) client.c raw.c $(CFLAGS) -o client

server: server.c list.c
	$(CC) server.c list.c $(CFLAGS) -o server

clean:
	rm -f client server *.o

