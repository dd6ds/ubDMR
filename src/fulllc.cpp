// DMR full link control, see fulllc.h.
//
// Layout: FLCO (call type), feature set ID, service options, 24-bit
// destination, 24-bit source, then the RS(12,9) parity over GF(2^8) with
// polynomial 0x11D, XORed with a mask that tells header and terminator apart.

#include "fulllc.h"

namespace FullLc {

namespace {

constexpr quint8 kFlcoGroupVoice = 0x00;
constexpr quint8 kFlcoPrivateVoice = 0x03;
constexpr quint8 kFlcoMask = 0x3f;
constexpr quint8 kHeaderMask = 0x96;
constexpr quint8 kTerminatorMask = 0x99;
constexpr int kDataLength = 9;
constexpr int kParityLength = 3;
const quint8 kGenerator[kParityLength] = {64, 56, 14};

// Multiplication in GF(2^8) with the field polynomial 0x11D.
quint8 gfMultiply(quint8 a, quint8 b)
{
    quint8 product = 0;
    while (b) {
        if (b & 1)
            product ^= a;
        const bool carry = a & 0x80;
        a = quint8(a << 1);
        if (carry)
            a ^= 0x1D;
        b >>= 1;
    }
    return product;
}

QByteArray parity(const QByteArray &data, quint8 mask)
{
    quint8 p[kParityLength] = {};
    for (int i = 0; i < kDataLength; ++i) {
        const quint8 feedback = quint8(data[i]) ^ p[2];
        p[2] = p[1] ^ gfMultiply(kGenerator[2], feedback);
        p[1] = p[0] ^ gfMultiply(kGenerator[1], feedback);
        p[0] = gfMultiply(kGenerator[0], feedback);
    }
    QByteArray out;
    for (int i = kParityLength - 1; i >= 0; --i)
        out.append(char(p[i] ^ mask));
    return out;
}

quint32 read24(const QByteArray &lc, int offset)
{
    return (quint32(quint8(lc[offset])) << 16) | (quint32(quint8(lc[offset + 1])) << 8)
            | quint8(lc[offset + 2]);
}

void append24(QByteArray &lc, quint32 value)
{
    lc.append(char(value >> 16));
    lc.append(char(value >> 8));
    lc.append(char(value));
}

} // namespace

bool parse(const QByteArray &lc, Call *call)
{
    if (lc.size() < kLength)
        return false;
    const quint8 flco = quint8(lc[0]) & kFlcoMask;
    if (flco != kFlcoGroupVoice && flco != kFlcoPrivateVoice)
        return false;
    call->isPrivate = flco == kFlcoPrivateVoice;
    call->destinationId = read24(lc, 3);
    call->sourceId = read24(lc, 6);
    return true;
}

QByteArray build(const Call &call, Kind kind)
{
    QByteArray lc;
    lc.append(char(call.isPrivate ? kFlcoPrivateVoice : kFlcoGroupVoice));
    lc.append('\0'); // standard feature set
    lc.append('\0'); // no service options
    append24(lc, call.destinationId);
    append24(lc, call.sourceId);
    return lc + parity(lc, kind == Kind::VoiceHeader ? kHeaderMask : kTerminatorMask);
}

} // namespace FullLc
