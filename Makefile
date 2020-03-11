CC = gcc

all: build check

build:
	${CC} crc32c/crc32c.cpp -D CRC32C_STATIC -lstdc++ -msse4.2 -shared -o crc32c.o
	${CC} runtests/runtests.cpp -D CRC32C_STATIC -o runtests.o
	${CC} crc32c.o runtests.o -o runtest

check:
	./runtest
