// Voice playback with a small jitter buffer, see audioplayer.h.

#include "audioplayer.h"

#include <QDebug>

#include "voiceformat.h"

namespace {

constexpr int kBytesPerMs = 16; // 8000 samples/s * 2 bytes
constexpr int kPrebufferMs = 180;
constexpr int kDeviceBufferMs = 250;
constexpr int kFeedIntervalMs = 20;

} // namespace

AudioPlayer::AudioPlayer(QObject *parent) : QObject(parent)
{
    m_feedTimer.setInterval(kFeedIntervalMs);
    connect(&m_feedTimer, &QTimer::timeout, this, &AudioPlayer::onFeedTimer);
}

void AudioPlayer::enqueue(const QByteArray &pcm)
{
    m_queue += pcm;
    m_finishing = false;
    ensureStarted();
}

void AudioPlayer::ensureStarted()
{
    if (m_output)
        return;
    m_output = new QAudioOutput(voiceFormat(), this);
    m_output->setBufferSize(kDeviceBufferMs * kBytesPerMs);
    m_device = m_output->start();
    if (!m_device) {
        qDebug() << "AudioPlayer: cannot open output, error" << m_output->error();
        delete m_output;
        m_output = nullptr;
        return;
    }
    m_buffering = true;
    m_feedTimer.start();
}

void AudioPlayer::finish()
{
    m_finishing = true;
}

void AudioPlayer::stop()
{
    m_feedTimer.stop();
    m_queue.clear();
    m_finishing = false;
    if (m_output) {
        m_output->stop();
        delete m_output;
        m_output = nullptr;
        m_device = nullptr;
    }
}

void AudioPlayer::onFeedTimer()
{
    if (m_buffering && !m_finishing && m_queue.size() < kPrebufferMs * kBytesPerMs)
        return;
    m_buffering = false;

    if (m_queue.isEmpty()) {
        if (m_finishing)
            stop();
        else
            m_buffering = true;
        return;
    }
    const int chunk = qMin(m_output->bytesFree(), m_queue.size());
    if (chunk > 0) {
        m_device->write(m_queue.constData(), chunk);
        m_queue.remove(0, chunk);
    }
}
