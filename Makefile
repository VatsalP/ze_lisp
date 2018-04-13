all:
	cc -std=c99 -g -Wall ze_lisp.c mpc.c -lreadline -lm -o ze_lisp.o

clean:
	rm ze_lisp.o
