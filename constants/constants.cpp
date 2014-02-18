// constants.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#define POLY 0x1EDC6F41
#define IEEEPOLY 0x04C11DB7

/* Block sizes for three-way parallel crc computation.  LONG_SHIFT and SHORT_SHIFT must
   both be powers of two.  The associated string constants must be set
   accordingly, for use in constructing the assembler instructions. */
#define LONG_SHIFT 8192
#define SHORT_SHIFT 256

typedef const uint8_t *buffer;

static uint32_t compute_trivial(buffer input, size_t length, uint32_t poly)
{
    uint32_t crc = 0;
    for (size_t i = 0; i < length; ++i)
    {
        crc = crc ^ input[i];
        for (int j = 0; j < 8; j++)
            crc = (crc >> 1) ^ 0x80000000 ^ ((~crc & 1) * poly);
    }
    return crc;
}

static uint32_t reverse_bitwise(uint32_t poly)
{
    uint32_t reversed = 0;
    for (int i = 0; i < 32; ++i)
    {
        reversed = (reversed << 1) + (poly & 1);
        poly >>= 1;
    }
    return reversed;
}

static uint64_t reverse_bitwise64(uint64_t poly)
{
    uint64_t reversed = 0;
    for (int i = 0; i < 64; ++i)
    {
        reversed = (reversed << 1) + (poly & 1);
        poly >>= 1;
    }
    return reversed;
}

static uint64_t reverse_full(uint32_t poly)
{
    return ((uint64_t)reverse_bitwise(poly) << 1) | 1;
}

static uint32_t mod_poly(buffer input, int length, uint32_t poly)
{
    uint32_t r = ((uint32_t)input[0] << 24) | ((uint32_t)input[1] << 16) | ((uint32_t)input[2] << 8) | (uint32_t)input[3];
    while (length > 4)
    {
        for (int b = 7; b >= 0; --b)
        {
            if (r & 0x80000000u)
            {
                r <<= 1;
                r |= input[4] >> b;
                r ^= poly;
            }
            else
            {
                r <<= 1;
                r |= input[4] >> b;
            }
        }
        --length;
    }
    return r;
}

static uint64_t div_poly(buffer input, int length, uint32_t poly)
{
    uint32_t r = ((uint32_t)input[0] << 24) | ((uint32_t)input[1] << 16) | ((uint32_t)input[2] << 8) | (uint32_t)input[3];
    uint32_t d = 0;
    while (length > 4)
    {
        for (int b = 7; b >= 0; --b)
        {
            d <<= 1;
            if (r & 0x80000000u)
            {
                r <<= 1;
                r |= input[4] >> b;
                r ^= poly;
                d |= 1;
            }
            else
            {
                r <<= 1;
                r |= input[4] >> b;
            }
        }
        --length;
    }
    return d;
}

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

static uint64_t compute_reversed_clmul_constant(int shift, uint32_t poly)
{
    static uint8_t a[100] = { 1 };
    return (uint64_t)reverse_bitwise(mod_poly(a, shift / 8 + 1, poly)) << 1;
}

static std::ostream& operator<<(std::ostream &out, __m128i w128)
{
    out << "0x" << std::hex << std::setw(8) << std::setfill('0') << w128.m128i_u32[3] << std::setw(8) << w128.m128i_u32[2] << std::setw(8) << w128.m128i_u32[1] << std::setw(8) << w128.m128i_u32[0];
    return out;
}

static __m128i fold_one(__m128i input)
{
    __m128i k3 = _mm_set_epi32(0, 0, 0, 0xC5B9CD4C);
    __m128i k4 = _mm_set_epi32(0, 0, 0, 0xE8A45605);

    //return _mm_castps_si128(_mm_xor_ps(_mm_castsi128_ps(_mm_clmulepi64_si128(input, k3, 0x01)), _mm_castsi128_ps(_mm_clmulepi64_si128(input, k4, 0x00))));
    __m128i upper = _mm_clmulepi64_si128(input, k4, 0x01);
    __m128i lower = _mm_clmulepi64_si128(input, k4, 0x00);
    __m128i shifted = _mm_slli_si128(upper, 8);
    return _mm_castps_si128(_mm_xor_ps(_mm_castsi128_ps(shifted), _mm_castsi128_ps(lower)));
}

static void check_clmul_constants()
{
    std::cout << "P(x)' = " << std::hex << reverse_full(IEEEPOLY) << std::endl;
    std::cout << "k1' = " << std::hex << compute_reversed_clmul_constant(4 * 128 + 32, IEEEPOLY) << std::endl;
    std::cout << "k2' = " << std::hex << compute_reversed_clmul_constant(4 * 128 - 32, IEEEPOLY) << std::endl;
    std::cout << "k3' = " << std::hex << compute_reversed_clmul_constant(128 + 32, IEEEPOLY) << std::endl;
    std::cout << "k4' = " << std::hex << compute_reversed_clmul_constant(128 - 32, IEEEPOLY) << std::endl;
    std::cout << "k5' = " << std::hex << compute_reversed_clmul_constant(64, IEEEPOLY) << std::endl;
    std::cout << "k6' = " << std::hex << compute_reversed_clmul_constant(32, IEEEPOLY) << std::endl;
    static uint8_t a[100] = { 1 };
    std::cout << "u' = " << std::hex << ((uint64_t)reverse_bitwise((uint32_t)div_poly(a, 9, IEEEPOLY)) << 1) << std::endl;
    __m128i magic = _mm_set_epi32(0, 0, 0, 0x9db42487);
    for (int i = 0; i < 65; ++i)
    {
        std::cout << magic << std::endl;
        magic = fold_one(magic);
    }
}

int main(int argc, char* argv[])
{
    try
    {
        check_clmul_constants();
        uint32_t reversed_poly = reverse_bitwise(POLY);
        initialize_table(table, 16, reversed_poly);
        make_shift_table(long_shifts, LONG_SHIFT, reversed_poly);
        make_shift_table(short_shifts, SHORT_SHIFT, reversed_poly);
        std::ofstream out;
        out.exceptions(std::ios::badbit | std::ios::failbit);
        out.open("generated-constants.cpp", std::ios_base::trunc);
        out << "#define POLY 0x" << std::hex << std::setw(8) << std::setfill('0') << reversed_poly << std::dec << std::endl;
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

