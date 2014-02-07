#ifndef CRC32C_H
#define CRC32C_H

#ifdef CRC32C_EXPORTS
#define CRC32C_API __declspec(dllexport)
#else
#define CRC32C_API __declspec(dllimport)
#endif

#include <stdint.h>

extern "C" CRC32C_API uint32_t crc32c_append(uint32_t crc, const void *input, size_t length);

extern "C" CRC32C_API void crc32c_unittest();

#endif
