BIN=echoc echos fork1 kirk lockdemo mmapdemo pipe1 pipe2 pipe3 semdemo \
	semrm shmdemo sigint sigusr spair speak spock tick

all: $(BIN)

echoc: echoc.c
	gcc -Wall -Wextra -o $@ $<

echos: echos.c
	gcc -Wall -Wextra -o $@ $<

fork1: fork1.c
	gcc -Wall -Wextra -o $@ $<

kirk: kirk.c
	gcc -Wall -Wextra -o $@ $<

lockdemo: lockdemo.c
	gcc -Wall -Wextra -o $@ $<

mmapdemo: mmapdemo.c
	gcc -Wall -Wextra -o $@ $<

pipe1: pipe1.c
	gcc -Wall -Wextra -o $@ $<

pipe2: pipe2.c
	gcc -Wall -Wextra -o $@ $<

pipe3: pipe3.c
	gcc -Wall -Wextra -o $@ $<

semdemo: semdemo.c
	gcc -Wall -Wextra -o $@ $<

semrm: semrm.c
	gcc -Wall -Wextra -o $@ $<

shmdemo: shmdemo.c
	gcc -Wall -Wextra -o $@ $<

sigint: sigint.c
	gcc -Wall -Wextra -o $@ $<

sigusr: sigusr.c
	gcc -Wall -Wextra -o $@ $<

spair: spair.c
	gcc -Wall -Wextra -o $@ $<

speak: speak.c
	gcc -Wall -Wextra -o $@ $<

spock: spock.c
	gcc -Wall -Wextra -o $@ $<

tick: tick.c
	gcc -Wall -Wextra -o $@ $<

clean:
	rm -f $(BIN)
	rm -f american_maid
