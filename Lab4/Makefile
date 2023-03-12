CC=gcc
CFLAGS=-c -Wall
LDFLAGS=-pthread
all: server client
server: server.o
	$(CC) $(LDFLAGS) server.o -o server
client: client.o
	$(CC) $(LDFLAGS) client.o -o client
client.o: client.c
	$(CC) $(CFLAGS) client.c
server.o: server.c
	$(CC) $(CFLAGS) server.c
clean:
	rm -f *.o server client