// The audio format the vocoder works in: 8 kHz, mono, signed 16-bit LE.
// PulseAudio on the phone resamples to and from the hardware rate.

#ifndef VOICEFORMAT_H
#define VOICEFORMAT_H

#include <QAudioFormat>

inline QAudioFormat voiceFormat()
{
    QAudioFormat format;
    format.setSampleRate(8000);
    format.setChannelCount(1);
    format.setSampleSize(16);
    format.setSampleType(QAudioFormat::SignedInt);
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setCodec(QStringLiteral("audio/pcm"));
    return format;
}

#endif
