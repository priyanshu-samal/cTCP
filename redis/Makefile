CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -g
SRC = src/server.c src/hashmap.c src/protocol.c

all: ctdredis

ctdredis: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o ctdredis

client: client/main.c
	$(CC) $(CFLAGS) client/main.c -o ctdredis-cli

clean:
	rm -f ctdredis ctdredis-cli test_hashmap test_protocol

.PHONY: all client clean