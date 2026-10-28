// AMBE+2 72 <-> 49 bit conversion, see ambeframe.h.
//
// The 72 bits are 4 code words C0..C3 (24, 23, 11 and 14 bits) held here as
// rows of a 4x24 bit matrix. C0 and C1 carry 12 data bits each plus Golay
// check bits; C1 is additionally XORed with a pseudo-random sequence seeded
// from C0's data bits.

#include "ambeframe.h"

namespace AmbeFrame {

namespace {

using Matrix = int[4][24];

// Interleave schedule: air bit pair i goes to (W,X) and (Y,Z) in the matrix.
const int kRowA[36] = {0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
                       0, 1, 0, 1, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2};
const int kColA[36] = {23, 10, 22, 9, 21, 8, 20, 7, 19, 6, 18, 5, 17, 4, 16, 3, 15, 2,
                       14, 1, 13, 0, 12, 10, 11, 9, 10, 8, 9, 7, 8, 6, 7, 5, 6, 4};
const int kRowB[36] = {0, 2, 0, 2, 0, 2, 0, 2, 0, 3, 0, 3, 1, 3, 1, 3, 1, 3,
                       1, 3, 1, 3, 1, 3, 1, 3, 1, 3, 1, 3, 1, 3, 1, 3, 1, 3};
const int kColB[36] = {5, 3, 4, 2, 3, 1, 2, 0, 1, 13, 0, 12, 22, 11, 21, 10, 20, 9,
                       19, 8, 18, 7, 17, 6, 16, 5, 15, 4, 14, 3, 13, 2, 12, 1, 11, 0};

constexpr uint32_t kGolayPoly = 0xAE3;
constexpr int kScramblerLength = 24;

// [23,12] Golay code word as check(11) | data(12).
uint32_t golay2312(uint32_t data)
{
    data &= 0xfff;
    uint32_t cw = data;
    for (int i = 0; i < 12; ++i) {
        if (cw & 1)
            cw ^= kGolayPoly;
        cw >>= 1;
    }
    return (cw << 12) | data;
}

uint32_t parity(uint32_t cw)
{
    uint32_t p = (cw ^ (cw >> 8) ^ (cw >> 16)) & 0xff;
    p ^= p >> 4;
    p ^= p >> 2;
    p ^= p >> 1;
    return p & 1;
}

int bitAt(const uint8_t *bytes, int index)
{
    return (bytes[index / 8] >> (7 - index % 8)) & 1;
}

void setBit(uint8_t *bytes, int index, int value)
{
    if (value)
        bytes[index / 8] |= uint8_t(0x80 >> (index % 8));
}

// XORs C1 with the sequence seeded from C0's data bits. Self-inverse, so it
// both scrambles and descrambles.
void scrambleC1(Matrix fr)
{
    uint32_t seed = 0;
    for (int i = 23; i >= 12; --i)
        seed = (seed << 1) | uint32_t(fr[0][i]);
    uint32_t pr[kScramblerLength];
    pr[0] = 16 * seed;
    for (int i = 1; i < kScramblerLength; ++i)
        pr[i] = (173 * pr[i - 1] + 13849) % 65536;
    int k = 1;
    for (int j = 22; j >= 0; --j, ++k)
        fr[1][j] ^= int(pr[k] / 32768);
}

void deinterleave(const Frame72 &in, Matrix fr)
{
    for (int i = 0; i < 36; ++i) {
        fr[kRowA[i]][kColA[i]] = bitAt(in.data(), 2 * i);
        fr[kRowB[i]][kColB[i]] = bitAt(in.data(), 2 * i + 1);
    }
}

Frame72 interleave(const Matrix fr)
{
    Frame72 out{};
    for (int i = 0; i < 36; ++i) {
        setBit(out.data(), 2 * i, fr[kRowA[i]][kColA[i]]);
        setBit(out.data(), 2 * i + 1, fr[kRowB[i]][kColB[i]]);
    }
    return out;
}

// Writes the low `count` bits of `value` into row `row`, LSB at column
// `firstCol`, going down to lower columns.
void putWord(Matrix fr, int row, int firstCol, int count, uint32_t value)
{
    for (int i = 0; i < count; ++i) {
        fr[row][firstCol - i] = int(value & 1);
        value >>= 1;
    }
}

} // namespace

Frame49 toRaw(const Frame72 &frame)
{
    Matrix fr = {};
    deinterleave(frame, fr);
    scrambleC1(fr);

    Frame49 out{};
    int bit = 0;
    for (int j = 23; j >= 12; --j)
        setBit(out.data(), bit++, fr[0][j]);
    for (int j = 22; j >= 11; --j)
        setBit(out.data(), bit++, fr[1][j]);
    for (int j = 10; j >= 0; --j)
        setBit(out.data(), bit++, fr[2][j]);
    for (int j = 13; j >= 0; --j)
        setBit(out.data(), bit++, fr[3][j]);
    return out;
}

Frame72 toAir(const Frame49 &frame)
{
    Matrix fr = {};

    // C0: 12 data bits, 11 check bits, parity. Voice bit 0 lands in column 23.
    uint32_t c0 = 0;
    for (int i = 11; i >= 0; --i)
        c0 = (c0 << 1) | uint32_t(bitAt(frame.data(), i));
    c0 = golay2312(c0);
    c0 |= parity(c0) << 23;
    putWord(fr, 0, 23, 24, c0);

    // C1: 12 data bits, 11 check bits.
    uint32_t c1 = 0;
    for (int i = 23; i >= 12; --i)
        c1 = (c1 << 1) | uint32_t(bitAt(frame.data(), i));
    putWord(fr, 1, 22, 23, golay2312(c1));

    // C2 and C3 are sent without protection.
    for (int j = 10; j >= 0; --j)
        fr[2][j] = bitAt(frame.data(), 34 - j);
    for (int j = 13; j >= 0; --j)
        fr[3][j] = bitAt(frame.data(), 48 - j);

    scrambleC1(fr);
    return interleave(fr);
}

} // namespace AmbeFrame
