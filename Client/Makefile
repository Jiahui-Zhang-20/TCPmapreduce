CC=gcc
CFLAGS=-g
SRCDIR=src
INCLDIR=include

client: $(SRCDIR)/client.c
	$(CC) $(CFLAGS) -Wall -pthread -Iinclude -o client $(SRCDIR)/client.c

.PHONY: clean

clean:
	rm client
	rm -rf log
	rm -rf *.dSYM


