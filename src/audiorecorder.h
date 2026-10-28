// Records the microphone and hands it out in 20 ms vocoder frames.

#ifndef AUDIORECORDER_H
#define AUDIORECORDER_H

#include <QAudioInput>
#include <QByteArray>
#include <QObject>

class AudioRecorder : public QObject
{
    Q_OBJECT

public:
    explicit AudioRecorder(QObject *parent = nullptr);

    bool start();
    void stop();

signals:
    // Vocoder::kPcmBytesPerFrame bytes of signed 16-bit LE audio.
    void frameReady(const QByteArray &pcm);

private:
    void onReadyRead();

    QAudioInput *m_input = nullptr;
    QIODevice *m_device = nullptr;
    QByteArray m_pending;
};

#endif
