#ifndef CRC32C_H
#define CRC32C_H

#ifdef CRC32C_EXPORTS
#define CRC32C_API __declspec(dllexport)
#else
#define CRC32C_API __declspec(dllimport)
#endif

#include <stdint.h>

extern "C" CRC32C_API uint32_t  // computed CRC-32C (Castagnoli)
crc32c_append(                  // calculates CRC-32C, utilizing HW automatically if available
    uint32_t crc,               // initial CRC, typically 0, may be used to accumulate CRC from multiple buffers
    const uint8_t *input,       // data to be put through the CRC algorithm
    size_t length);             // length of the data in the input buffer

extern "C" CRC32C_API void crc32c_unittest();

#endif
