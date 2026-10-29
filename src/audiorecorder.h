// Records the microphone and hands it out in 20 ms vocoder frames, amplified
// by the adjustable microphone gain, and measures the level of each frame
// for the level meter.

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
    bool isRecording() const { return m_device != nullptr; }

    // Gain applied to the microphone signal, in dB. Peaks the gain would push
    // beyond full scale are rounded off by a soft limiter instead of clipping.
    void setGainDb(double db);

signals:
    // Vocoder::kPcmBytesPerFrame bytes of signed 16-bit LE audio.
    void frameReady(const QByteArray &pcm);
    // Peak of the amplified frame in dB full scale (-90 for silence), and
    // whether the gain drove it into the limiter.
    void levelMeasured(double peakDbfs, bool limited);

private:
    void onReadyRead();
    QByteArray process(const QByteArray &pcm);

    QAudioInput *m_input = nullptr;
    QIODevice *m_device = nullptr;
    QByteArray m_pending;
    float m_gain = 1.0f;
};

#endif
