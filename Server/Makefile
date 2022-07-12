CC=gcc
CFLAGS=-g
SRCDIR=src
INCLDIR=include

server:	$(SRCDIR)/server.c
	$(CC) $(CFLAGS) -Wall -pthread -Iinclud -o server $(SRCDIR)/server.c

clean:
	rm server
	rm -rf *.dSYM

