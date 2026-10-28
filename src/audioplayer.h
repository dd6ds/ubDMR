// Plays decoded voice. Network frames arrive in 60 ms bursts with jitter, so
// playback starts only once kPrebufferMs of audio are queued, and starts
// buffering again after an underrun.

#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <QAudioOutput>
#include <QByteArray>
#include <QObject>
#include <QTimer>

class AudioPlayer : public QObject
{
    Q_OBJECT

public:
    explicit AudioPlayer(QObject *parent = nullptr);

    void enqueue(const QByteArray &pcm);
    // Plays what is queued, then releases the audio device.
    void finish();
    // Drops everything immediately (e.g. before transmitting).
    void stop();

private:
    void ensureStarted();
    void onFeedTimer();

    QAudioOutput *m_output = nullptr;
    QIODevice *m_device = nullptr;
    QTimer m_feedTimer;
    QByteArray m_queue;
    bool m_buffering = true;
    bool m_finishing = false;
};

#endif
