CC      = gcc
CFLAGS  = -Wall -g -D_POSIX_SOURCE -D_BSD_SOURCE -std=c99  -pedantic -lcurl

.SUFFIXES: .c .o

.PHONY: clean

all: algorithm

algorithm: algorithm.o trader.o cJSON.o
	$(CC) $(CFLAGS) -o algorithm algorithm.o trader.o cJSON.o $(shell curl-config --libs) -lm
trader.o: trader.h cJSON.h

cJSON.o: cJSON.h


clean:
	rm -f $(wildcard *.o)
	rm -f $(wildcard *.gch)
	rm -f algorithm
