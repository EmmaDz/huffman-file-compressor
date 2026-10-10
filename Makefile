CC=clang
CFLAGS=-Wall -Wextra -O2 -std=c11

all: huffman

huffman: main.o huffman.o
	$(CC) $(CFLAGS) -o huffman main.o huffman.o

main.o: main.c huffman.h
	$(CC) $(CFLAGS) -c main.c

huffman.o: huffman.c huffman.h
	$(CC) $(CFLAGS) -c huffman.c

clean:
	rm -f *.o huffman
