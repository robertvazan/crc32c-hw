// runtests.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <random>
#define NOMINMAX
#include <windows.h>

#include "crc32c.h"

#define TEST_BUFFER 65536
#define TEST_SLICES 1000000
#define POLY 0x82f63b78

typedef const uint8_t *buffer;

static uint32_t table[16][256];

uint32_t trivial_append_sw(uint32_t crc, buffer input, size_t length)
{
    for (size_t i = 0; i < length; ++i)
    {
        crc = crc ^ input[i];
        for (int j = 0; j < 8; j++)
            crc = (crc >> 1) ^ 0x80000000 ^ ((~crc & 1) * POLY);
    }
    return crc;
}

void calculate_table() 
{
	for(int i = 0; i < 256; i++) 
	{
		uint32_t res = (uint32_t)i;
		for(int t = 0; t < 16; t++) {
			for (int k = 0; k < 8; k++) res = (res & 1) == 1 ? POLY ^ (res >> 1) : (res >> 1);
			table[t][i] = res;
		}
	}
}

/* Table-driven software version as a fall-back.  This is about 15 times slower
   than using the hardware instructions.  This assumes little-endian integers,
   as is the case on Intel processors that the assembler code here is for. */
uint32_t adler_append_sw(uint32_t crci, buffer input, size_t length)
{
    buffer next = input;
    uint64_t crc;

    crc = crci ^ 0xffffffff;
    while (length && ((uintptr_t)next & 7) != 0)
    {
        crc = table[0][(crc ^ *next++) & 0xff] ^ (crc >> 8);
        --length;
    }
    while (length >= 8)
    {
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

static int benchmark(const char *name, uint32_t(*function)(uint32_t, buffer, size_t), buffer input, int *offsets, int *lengths, uint32_t *crcs)
{
    uint64_t startTime = GetTickCount64();
    int slice = 0;
    uint64_t totalBytes = 0;
    bool first = true;
    int iterations = 0;
    uint32_t crc = 0;
    while (GetTickCount64() - startTime < 1000)
    {
        crc = function(crc, input + offsets[slice], lengths[slice]);
        totalBytes += lengths[slice];
        if (first)
            crcs[slice] = crc;
        ++slice;
        ++iterations;
        if (slice == TEST_SLICES)
        {
            slice = 0;
            first = false;
        }
    }
    int time = static_cast<int>(GetTickCount64() - startTime);
    double throughput = totalBytes * 1000.0 / time;
    printf("%s: ", name);
    if (throughput > 1024.0 * 1024.0 * 1024.0)
        printf("%.1f GB/s\n", throughput / 1024 / 1024 / 1024);
    else
        printf("%.0f MB/s\n", throughput / 1024 / 1024);
    return std::min(TEST_SLICES, iterations);
}

static void compare_crcs(const char *leftName, uint32_t *left, const char *rightName, uint32_t *right, int count)
{
    for (int i = 0; i < count; ++i)
        if (left[i] != right[i])
        {
            printf("CRC mismatch between algorithms %s and %s at offset %d: %x vs %x\n", leftName, rightName, i, left[i], right[i]);
            exit(1);
        }
}

void crc32c_unittest()
{
    std::random_device rd;
    std::uniform_int_distribution<int> byteDist(0, 255);
    uint8_t *input = new uint8_t[TEST_BUFFER];
    for (int i = 0; i < TEST_BUFFER; ++i)
        input[i] = byteDist(rd);
    int *offsets = new int[TEST_SLICES];
    int *lengths = new int[TEST_SLICES];
    std::uniform_int_distribution<int> lengthDist(0, TEST_BUFFER);
    for (int i = 0; i < TEST_SLICES; ++i)
    {
        lengths[i] = lengthDist(rd);
        std::uniform_int_distribution<int> offsetDist(0, TEST_BUFFER - lengths[i]);
        offsets[i] = offsetDist(rd);
    }
    uint32_t *crcsTrivial = new uint32_t[TEST_SLICES];
    uint32_t *crcsAdlerTable = new uint32_t[TEST_SLICES];
    uint32_t *crcsTable = new uint32_t[TEST_SLICES];
    uint32_t *crcsHw = new uint32_t[TEST_SLICES];
    int iterationsTrivial = benchmark("trivial", trivial_append_sw, input, offsets, lengths, crcsTrivial);
	calculate_table(); // precalculating table out-of-benchmark, because it is one-time operation
	int iterationsAdlerTable = benchmark("adler_table", adler_append_sw, input, offsets, lengths, crcsAdlerTable);
    compare_crcs("trivial", crcsTrivial, "adler_table", crcsAdlerTable, std::min(iterationsTrivial, iterationsAdlerTable));
    int iterationsTable = benchmark("table", crc32c_append_sw, input, offsets, lengths, crcsTable);
    compare_crcs("adler_table", crcsAdlerTable, "table", crcsTable, std::min(iterationsAdlerTable, iterationsTable));
    if (crc32c_hw_available())
    {
        int iterationsHw = benchmark("hw", crc32c_append_hw, input, offsets, lengths, crcsHw);
        compare_crcs("table", crcsTable, "hw", crcsHw, std::min(iterationsTable, iterationsHw));
    }
    else
        printf("HW doesn't have crc instruction\n");
    benchmark("auto", crc32c_append, input, offsets, lengths, crcsHw);
}

int main(int argc, char* argv[])
{
    crc32c_unittest();
    return 0;
}
