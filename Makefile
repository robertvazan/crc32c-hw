CC = gcc

all: build check

build:
	${CC} crc32c/crc32c.cpp -D CRC32C_STATIC -lstdc++ -msse4.2 -fPIC -shared -o crc32c.o
	${CC} runtests/runtests.cpp -D CRC32C_STATIC -I crc32c -lstdc++ -l crc32c.o -o run_tests

check:
	./run_tests
