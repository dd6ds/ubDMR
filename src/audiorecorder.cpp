// Microphone capture, see audiorecorder.h.

#include "audiorecorder.h"

#include <QDebug>
#include <QtEndian>

#include <cmath>

#include "vocoder.h"
#include "voiceformat.h"

namespace {

constexpr float kFullScale = 32767.0f;
// Above this fraction of full scale the limiter starts to round peaks off.
constexpr float kLimiterKnee = 0.7f;
constexpr double kSilenceDbfs = -90.0;

// Maps |x| (fraction of full scale, any size) smoothly into [0, 1).
float limit(float x)
{
    const float a = std::fabs(x);
    if (a <= kLimiterKnee)
        return x;
    const float headroom = 1.0f - kLimiterKnee;
    const float y = kLimiterKnee + headroom * std::tanh((a - kLimiterKnee) / headroom);
    return x < 0 ? -y : y;
}

} // namespace

AudioRecorder::AudioRecorder(QObject *parent) : QObject(parent)
{
}

void AudioRecorder::setGainDb(double db)
{
    m_gain = float(std::pow(10.0, db / 20.0));
}

bool AudioRecorder::start()
{
    stop();
    m_input = new QAudioInput(voiceFormat(), this);
    m_device = m_input->start();
    if (!m_device) {
        qDebug() << "AudioRecorder: cannot open microphone, error" << m_input->error();
        stop();
        return false;
    }
    connect(m_device, &QIODevice::readyRead, this, &AudioRecorder::onReadyRead);
    return true;
}

void AudioRecorder::stop()
{
    if (m_input) {
        m_input->stop();
        delete m_input;
        m_input = nullptr;
    }
    m_device = nullptr;
    m_pending.clear();
}

void AudioRecorder::onReadyRead()
{
    m_pending += m_device->readAll();
    while (m_pending.size() >= Vocoder::kPcmBytesPerFrame) {
        emit frameReady(process(m_pending.left(Vocoder::kPcmBytesPerFrame)));
        m_pending.remove(0, Vocoder::kPcmBytesPerFrame);
    }
}

QByteArray AudioRecorder::process(const QByteArray &pcm)
{
    QByteArray out(pcm.size(), Qt::Uninitialized);
    const auto *in = reinterpret_cast<const uchar *>(pcm.constData());
    auto *dst = reinterpret_cast<uchar *>(out.data());
    float peak = 0.0f;
    bool limited = false;
    for (int i = 0; i + 1 < pcm.size(); i += 2) {
        const float x = qFromLittleEndian<qint16>(in + i) / kFullScale * m_gain;
        if (std::fabs(x) > kLimiterKnee)
            limited = true;
        const float y = limit(x);
        peak = std::max(peak, std::fabs(y));
        qToLittleEndian<qint16>(qint16(std::lround(y * kFullScale)), dst + i);
    }
    emit levelMeasured(peak > 0 ? std::max(kSilenceDbfs, 20.0 * std::log10(peak)) : kSilenceDbfs,
                       limited);
    return out;
}
