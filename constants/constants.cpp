// constants.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#define POLY 0x82f63b78

/* Block sizes for three-way parallel crc computation.  LONG_SHIFT and SHORT_SHIFT must
   both be powers of two.  The associated string constants must be set
   accordingly, for use in constructing the assembler instructions. */
#define LONG_SHIFT 8192
#define SHORT_SHIFT 256

/* Construct table for software CRC-32C calculation. */
static void initialize_table(uint32_t table[][256], int levels, uint32_t poly)
{
    for (uint32_t n = 0; n < 256; ++n)
    {
        uint32_t crc = n;
        crc = crc & 1 ? (crc >> 1) ^ poly : crc >> 1;
        crc = crc & 1 ? (crc >> 1) ^ poly : crc >> 1;
        crc = crc & 1 ? (crc >> 1) ^ poly : crc >> 1;
        crc = crc & 1 ? (crc >> 1) ^ poly : crc >> 1;
        crc = crc & 1 ? (crc >> 1) ^ poly : crc >> 1;
        crc = crc & 1 ? (crc >> 1) ^ poly : crc >> 1;
        crc = crc & 1 ? (crc >> 1) ^ poly : crc >> 1;
        crc = crc & 1 ? (crc >> 1) ^ poly : crc >> 1;
        table[0][n] = crc;
    }
    for (uint32_t n = 0; n < 256; ++n)
    {
        uint32_t crc = table[0][n];
        for (int k = 1; k < 16; k++)
            crc = table[k][n] = table[0][crc & 0xff] ^ (crc >> 8);
    }
}

/* Multiply a matrix times a vector over the Galois field of two elements,
   GF(2).  Each element is a bit in an unsigned integer.  mat must have at
   least as many entries as the power of two for most significant one bit in
   vec. */
static inline uint32_t gf2_matrix_times(uint32_t *mat, uint32_t vec)
{
    uint32_t sum = 0;
    while (vec)
    {
        if (vec & 1)
            sum ^= *mat;
        vec >>= 1;
        ++mat;
    }
    return sum;
}

/* Multiply a matrix by itself over GF(2).  Both mat and square must have 32 rows. */
static inline void gf2_matrix_square(uint32_t *square, uint32_t *mat)
{
    for (int n = 0; n < 32; n++)
        square[n] = gf2_matrix_times(mat, mat[n]);
}

/* Construct an operator to apply len zeros to a crc.  len must be a power of
   two.  If len is not a power of two, then the result is the same as for the
   largest power of two less than len.  The result for len == 0 is the same as
   for len == 1.  A version of this routine could be easily written for any
   len, but that is not needed for this application. */
static void make_shift_op(uint32_t *even, size_t len, uint32_t poly)
{
    uint32_t odd[32];       /* odd-power-of-two zeros operator */

    /* put operator for one zero bit in odd */
    odd[0] = poly;
    uint32_t row = 1;
    for (int n = 1; n < 32; ++n)
    {
        odd[n] = row;
        row <<= 1;
    }

    /* put operator for two zero bits in even */
    gf2_matrix_square(even, odd);

    /* put operator for four zero bits in odd */
    gf2_matrix_square(odd, even);

    /* first square will put the operator for one zero byte (eight zero bits),
       in even -- next square puts operator for two zero bytes in odd, and so
       on, until len has been rotated down to zero */
    do
    {
        gf2_matrix_square(even, odd);
        len >>= 1;
        if (len == 0)
            return;
        gf2_matrix_square(odd, even);
        len >>= 1;
    } while (len);

    /* answer ended up in odd -- copy to even */
    for (int n = 0; n < 32; ++n)
        even[n] = odd[n];
}

/* Take a length and build four lookup tables for applying the zeros operator
   for that length, byte-by-byte on the operand. */
static void make_shift_table(uint32_t shift_table[][256], size_t len, uint32_t poly)
{
    uint32_t op[32];
    make_shift_op(op, len, poly);

    for (uint32_t n = 0; n < 256; n++)
    {
        shift_table[0][n] = gf2_matrix_times(op, n);
        shift_table[1][n] = gf2_matrix_times(op, n << 8);
        shift_table[2][n] = gf2_matrix_times(op, n << 16);
        shift_table[3][n] = gf2_matrix_times(op, n << 24);
    }
}

static void write_table(std::ostream& out, uint32_t table[][256], int levels, const char *name)
{
    out << "static uint32_t " << name << "[" << levels << "][256] =" << std::endl;
    out << "{" << std::endl;
    for (int level = 0; level < levels; ++level)
    {
        if (level != 0)
            out << "," << std::endl;
        out << "    { ";
        for (int i = 0; i < 256; ++i)
        {
            if (i != 0)
                out << ", ";
            out << "0x" << std::hex << std::setw(8) << std::setfill('0') << table[level][i];
        }
        out << " }";
    }
    out << std::endl << "};" << std::endl;
}

static void write_shift_table(std::ostream& out, uint32_t shift_table[][256], const char *name)
{
    out << "static uint32_t " << name << "[4][256] =" << std::endl;
    out << "{" << std::endl;
    for (int level = 0; level < 4; ++level)
    {
        if (level != 0)
            out << "," << std::endl;
        out << "    { ";
        for (int i = 0; i < 256; ++i)
        {
            if (i != 0)
                out << ", ";
            out << "0x" << std::hex << std::setw(8) << std::setfill('0') << shift_table[level][i];
        }
        out << " }";
    }
    out << std::endl << "};" << std::endl;
}

static uint32_t table[16][256];

/* Tables for hardware crc that shift a crc by LONG_SHIFT and SHORT_SHIFT zeros. */
static uint32_t long_shifts[4][256];
static uint32_t short_shifts[4][256];

int main(int argc, char* argv[])
{
    try
    {
        initialize_table(table, 16, POLY);
        make_shift_table(long_shifts, LONG_SHIFT, POLY);
        make_shift_table(short_shifts, SHORT_SHIFT, POLY);
        std::ofstream out;
        out.exceptions(std::ios::badbit | std::ios::failbit);
        out.open("generated-constants.cpp", std::ios_base::trunc);
        out << "#define LONG_SHIFT " << LONG_SHIFT << std::endl;
        out << "#define SHORT_SHIFT " << SHORT_SHIFT << std::endl;
        out << std::endl;
        write_table(out, table, 16, "table");
        out << std::endl;
        write_shift_table(out, long_shifts, "long_shifts");
        out << std::endl;
        write_shift_table(out, short_shifts, "short_shifts");
    }
    catch (const std::exception &e)
    {
        std::cout << "Exception occured" << std::endl;
        std::cout << e.what();
    }
    return 0;
}

