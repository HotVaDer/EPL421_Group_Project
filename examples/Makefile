# Builds every .c file in this directory into an executable of the same name.
CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -O2 -g
LDLIBS  := -luring

SRCS := $(wildcard *.c)
BINS := $(SRCS:.c=)

.PHONY: all clean

all: $(BINS)

%: %.c
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

clean:
	rm -f $(BINS)
