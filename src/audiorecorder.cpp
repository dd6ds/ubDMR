// Microphone capture, see audiorecorder.h.

#include "audiorecorder.h"

#include <QDebug>

#include "vocoder.h"
#include "voiceformat.h"

AudioRecorder::AudioRecorder(QObject *parent) : QObject(parent)
{
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
        emit frameReady(m_pending.left(Vocoder::kPcmBytesPerFrame));
        m_pending.remove(0, Vocoder::kPcmBytesPerFrame);
    }
}
