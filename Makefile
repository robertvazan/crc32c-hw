CC = gcc

all: build check

build:
	${CC} crc32c/crc32c.c -D CRC32C_STATIC -msse4.2 -fPIC -c -o crc32c.o
	ar rcs libcrc32c.a crc32c.o
	${CC} runtests/runtests.cpp -D CRC32C_STATIC -I crc32c -lstdc++ -c -o run_tests.o
	${CC} run_tests.o libcrc32c.a -lstdc++ -o run_tests

check:
	./run_tests
