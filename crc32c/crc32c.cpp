// crc32c.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "crc32c.h"

#define POLY 0x82f63b78

typedef const uint8_t *buffer;

static uint32_t append_trivial(uint32_t crc, buffer input, size_t length)
{
	for (size_t i = 0; i < length; ++i)
	{
		crc = crc ^ input[i];
		for (int j = 0; j < 8; j++)
			crc = (crc >> 1) ^ 0x80000000 ^ ((~crc & 1) * POLY);
	}
	return crc;
}

static uint32_t table[8][256];

static void initialize_table()
{
	for (uint32_t n = 0; n < 256; ++n)
	{
		uint32_t crc = n;
		crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
		crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
		crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
		crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
		crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
		crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
		crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
		crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
		table[0][n] = crc;
	}
	for (uint32_t n = 0; n < 256; ++n)
	{
		uint32_t crc = table[0][n];
		for (int k = 1; k < 8; k++)
			crc = table[k][n] = table[0][crc & 0xff] ^ (crc >> 8);
	}
}

static uint32_t append_table(uint32_t crci, buffer input, size_t length)
{
	buffer next = input;
	uint64_t crc;

	crc = crci ^ 0xffffffff;
	while (length && ((uintptr_t)next & 7) != 0)
	{
		crc = table[0][(crc ^ *next++) & 0xff] ^ (crc >> 8);
		--length;
	}
	while (length >= 8) {
		crc ^= *(uint64_t *)next;
		crc = table[7][crc & 0xff]
			^ table[6][(crc >> 8) & 0xff]
			^ table[5][(crc >> 16) & 0xff]
			^ table[4][(crc >> 24) & 0xff]
			^ table[3][(crc >> 32) & 0xff]
			^ table[2][(crc >> 40) & 0xff]
			^ table[1][(crc >> 48) & 0xff]
			^ table[0][crc >> 56];
		next += 8;
		length -= 8;
	}
	while (length)
	{
		crc = table[0][(crc ^ *next++) & 0xff] ^ (crc >> 8);
		--length;
	}
	return (uint32_t)crc ^ 0xffffffff;
}

static uint32_t append_switch(uint32_t crc, buffer input, size_t length)
{
	return append_table(crc, input, length);
}

void crc32c_initialize()
{
	initialize_table();
}

extern "C" CRC32C_API uint32_t crc32c_append(uint32_t crc, const void *input, size_t length)
{
	return append_switch(crc, reinterpret_cast<buffer>(input), length);
}

extern "C" CRC32C_API void crc32c_unittest()
{
	buffer sample = reinterpret_cast<buffer>("Hello world! Just in case a longer string changes the result.");
	int length = strlen(reinterpret_cast<const char *>(sample));
	printf("%x\n", append_trivial(0, sample, length));
	printf("%x\n", append_table(0, sample, length));
}
