// Conversion between the two AMBE+2 frame formats the app deals with.
//
// BrandMeister carries voice as 72-bit frames: 49 voice bits protected by
// Golay codes, scrambled and interleaved exactly as on the DMR air interface.
// The vocoder (DV3000 at rate index 34) works on the bare 49 voice bits.
// Bit layout and tables follow dmr_utils (N0MJS), which BrandMeister bridges
// use for the same job.

#ifndef AMBEFRAME_H
#define AMBEFRAME_H

#include <array>
#include <cstdint>

namespace AmbeFrame {

constexpr int kFrame72Bytes = 9;
constexpr int kFrame49Bytes = 7;
constexpr int kFrame49Bits = 49;

using Frame72 = std::array<uint8_t, kFrame72Bytes>;
using Frame49 = std::array<uint8_t, kFrame49Bytes>;

// Drops the FEC; bit errors are not corrected (network frames carry none).
Frame49 toRaw(const Frame72 &frame);

// Adds Golay FEC, scrambling and interleaving for transmission.
Frame72 toAir(const Frame49 &frame);

// The silence frame DMR repeaters send (first frame of MMDVM's
// DMR_SILENCE_DATA). Pads the last packet of a transmission.
constexpr Frame72 kSilence72 = {0xB9, 0xE8, 0x81, 0x52, 0x61, 0x73, 0x00, 0x2A, 0x6B};

} // namespace AmbeFrame

#endif
