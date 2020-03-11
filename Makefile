CC = gcc

all: build check

build:
	${CC} crc32c/crc32c.cpp -o crc32c.o
	${CC} runtests/runtests.cpp -o runtests.o
	${CC} crc32c.o runtests.o -o runtest

check:
	./runtest
