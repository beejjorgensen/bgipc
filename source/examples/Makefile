SRCS=$(wildcard *c)
TARGETS=$(patsubst %.c,%,$(SRCS))

CC=gcc
CCOPTS=-Wall -Wextra

.PHONY: all clean pristine

all: $(TARGETS)

clean:
	rm -f $(TARGETS)
	rm -f american_maid

pristine: clean

%: %.c
	$(CC) $(CCOPTS) -o $@ $<

