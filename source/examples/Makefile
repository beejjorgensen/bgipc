BIN=echoc echos fork1 kirk lockdemo mmapdemo pipe1 pipe2 pipe3 semdemo \
	semrm shmdemo sigint sigusr spair speak spock tick

CC=gcc
CCOPTS=-Wall -Wextra

.PHONY: all clean pristine

all: $(BIN)

clean:
	rm -f $(BIN)
	rm -f american_maid

pristine: clean

%: %.c
	$(CC) $(CCOPTS) -o $@ $<

