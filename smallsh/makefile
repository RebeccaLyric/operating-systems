CC = gcc
CFLAGS = -Wall -std=c99 -D_POSIX_C_SOURCE=200809L

smallsh: smallsh.c
	gcc -o smallsh smallsh.c $(CFLAGS)

debug: smallsh.c
	gcc -o smallsh smallsh.c $(CFLAGS) -g

grading:
	bash p3testscript 2>&1

savescript:
	bash p3testscript > smallshresults.out 2>&1

clean:
	rm -f smallsh *.txt *.out junk*

