// DMR full link control (ETSI TS 102 361-2): who calls whom. BrandMeister
// sends it to terminals in the voice header and terminator packets as
// 9 data bytes plus a 3-byte Reed-Solomon (12,9) checksum, and expects the
// same from us.

#ifndef FULLLC_H
#define FULLLC_H

#include <QByteArray>

namespace FullLc {

constexpr int kLength = 12;

enum class Kind { VoiceHeader, Terminator };

struct Call {
    bool isPrivate = false;
    quint32 sourceId = 0;
    quint32 destinationId = 0;
};

// False for anything that is not a voice call LC.
bool parse(const QByteArray &lc, Call *call);
QByteArray build(const Call &call, Kind kind);

} // namespace FullLc

#endif
