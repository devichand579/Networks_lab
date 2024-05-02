SHELL := /bin/bash

lib: msocket.o
	ar rcs libmsocket.a msocket.o
	

msocket.o: msocket.c
	gcc -c msocket.c -o msocket.o


user1: lib user1.c
	gcc user1.c  -L. -o user1 -lmsocket


user2: lib user2.c
	gcc user2.c  -L. -o user2 -lmsocket

run1: user1
	./user1

run2: user2
	./user2

init : initmsocket.c
	gcc initmsocket.c -o init

runinit : init
	./init

clean :
	-rm -f ./user1 ./user2 ./init user1.o user2.o initmsocket.o msocket.o libmsocket.a New.txt
