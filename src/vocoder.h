// Common interface of the two AMBE+2 vocoders: SoftwareVocoder (built in)
// and AmbeServerVocoder (a DV3000 behind an AMBEserver). Both work on bare
// 49-bit frames; one frame is 20 ms = 160 samples of 8 kHz 16-bit audio.

#ifndef VOCODER_H
#define VOCODER_H

#include <QByteArray>
#include <QObject>

#include "ambeframe.h"

class Vocoder : public QObject
{
    Q_OBJECT

public:
    static constexpr int kSamplesPerFrame = 160;
    static constexpr int kPcmBytesPerFrame = kSamplesPerFrame * 2;

    using QObject::QObject;

    virtual bool isReady() const = 0;
    virtual void decode(const AmbeFrame::Frame49 &frame) = 0;
    // pcm: kPcmBytesPerFrame bytes, signed 16-bit little-endian.
    virtual void encode(const QByteArray &pcm) = 0;
    // A new call starts; drop what was learned from the previous one.
    virtual void reset() {}

signals:
    void readyChanged(bool ready);
    // Signed 16-bit little-endian, kPcmBytesPerFrame bytes.
    void decoded(const QByteArray &pcm);
    void encoded(const AmbeFrame::Frame49 &frame);
    void problem(const QString &text);
};

#endif
