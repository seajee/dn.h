CC=gcc
CFLAGS=-Wall -Wextra
LDFLAGS=-lpthread

all: client server

client: client.c
	$(CC) $(CFLAGS) -o client client.c $(LDFLAGS)

server: server.c
	$(CC) $(CFLAGS) -o server server.c $(LDFLAGS)

clean:
	rm -rf client
	rm -rf server
