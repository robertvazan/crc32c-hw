// crc32c.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "crc32c.h"

#define POLY 0x82f63b78

typedef uint8_t byte;
typedef uint32_t word;
typedef size_t size;
typedef const byte *buffer;

static word trivial(word crc, buffer input, size length)
{
	for (size i = 0; i < length; i++)
	{
		crc = crc ^ input[i];
		for (int j = 0; j < 8; j++)
			crc = (crc >> 1) ^ 0x80000000 ^ ((~crc & 1) * POLY);
	}
	return crc;
}

word append(word crc, buffer input, size_t length)
{
	return trivial(crc, input, length);
}

extern "C" CRC32C_API uint32_t crc32c_append(uint32_t crc, const void *input, size_t length)
{
	return append(crc, reinterpret_cast<buffer>(input), length);
}

extern "C" CRC32C_API void crc32c_unittest()
{
	buffer sample = reinterpret_cast<buffer>("Hello world!");
	int length = strlen(reinterpret_cast<const char *>(sample));
	printf("%x", trivial(0, sample, length));
}
