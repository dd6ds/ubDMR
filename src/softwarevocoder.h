// Built-in AMBE+2 vocoder: mbelib-neo for decoding, OP25's MBE encoder for
// encoding (third_party/opendmr). Needs no hardware; sounds rougher than a
// DVSI chip, like the other software DMR clients do.
//
// Encoding costs several milliseconds per 20 ms frame on a phone, so the
// codec runs on its own thread; results arrive as signals on the caller's.

#ifndef SOFTWAREVOCODER_H
#define SOFTWAREVOCODER_H

#include <QThread>

#include <memory>

#include "vocoder.h"

class SoftwareVocoder : public Vocoder
{
    Q_OBJECT

public:
    explicit SoftwareVocoder(QObject *parent = nullptr);
    ~SoftwareVocoder() override;

    bool isReady() const override { return true; }
    void decode(const AmbeFrame::Frame49 &frame) override;
    void encode(const QByteArray &pcm) override;
    void reset() override;

private:
    struct Codec;
    // Only touched on m_thread, through m_worker.
    std::unique_ptr<Codec> m_codec;
    QThread m_thread;
    QObject *m_worker = nullptr;
};

#endif
