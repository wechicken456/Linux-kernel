CC=gcc
CLFAGS= -luring

SRCS = $(wildcard *.c)

PROGS = $(patsubst %.c,%,$(SRCS))

all: $(PROGS)

%: %.c
	$(CC) $< $(CLFAGS) -o $@ 

clean:
	rm $(PROGS)
