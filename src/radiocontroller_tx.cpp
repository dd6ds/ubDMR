// RadioController: transmitting, see radiocontroller.h.
//
// Microphone frames (20 ms) go to the vocoder; every three encoded frames
// make one 60 ms burst for the network. When PTT is released the frames still
// at the vocoder are awaited briefly, the last burst is padded with silence
// and the call is closed with a terminator.

#include "radiocontroller.h"

namespace {

constexpr int kBurstBytes = 3 * AmbeFrame::kFrame72Bytes;
// Time to wait for the vocoder's last answers after PTT release.
constexpr int kTxFlushMs = 250;

} // namespace

void RadioController::startTransmit(int destination, bool privateCall)
{
    if (m_transmitting)
        return;
    if (!online() || !m_vocoder->isReady() || destination <= 0) {
        setStatus(tr("Not ready to transmit"));
        return;
    }
    endReceive();
    m_player.stop();
    m_txBurst.clear();
    m_encodesInFlight = 0;
    m_stopping = false;
    m_vocoder->reset();

    if (!m_recorder.start()) {
        setStatus(tr("Microphone not available"));
        return;
    }
    m_rewind.startCall(privateCall ? RewindClient::PrivateVoice : RewindClient::GroupVoice,
                       sourceId(), quint32(destination));
    const QString callsign = m_settings.value(QStringLiteral("callsign")).toString().toUpper();

    m_transmitting = true;
    m_call = LastHeardModel::Call();
    m_call.sourceId = sourceId();
    m_call.sourceCall = callsign;
    m_call.destinationId = quint32(destination);
    m_call.isPrivate = privateCall;
    m_call.outgoing = true;
    m_call.started = QDateTime::currentDateTime();
    m_lastHeard.beginCall(m_call);
    m_txLimitTimer.start(kTransmitLimitMs);
    emit callChanged();
}

void RadioController::stopTransmit()
{
    if (!m_transmitting || m_stopping)
        return;
    m_recorder.stop();
    m_txLimitTimer.stop();
    m_stopping = true;
    if (m_encodesInFlight == 0)
        finishTransmit();
    else
        m_txFlushTimer.start(kTxFlushMs);
}

void RadioController::onMicFrame(const QByteArray &pcm)
{
    if (!m_transmitting || m_stopping)
        return;
    ++m_encodesInFlight;
    m_vocoder->encode(pcm);
}

void RadioController::onEncoded(const AmbeFrame::Frame49 &frame)
{
    if (!m_transmitting)
        return;
    m_encodesInFlight = qMax(0, m_encodesInFlight - 1);
    const AmbeFrame::Frame72 air = AmbeFrame::toAir(frame);
    m_txBurst.append(reinterpret_cast<const char *>(air.data()), int(air.size()));
    if (m_txBurst.size() == kBurstBytes) {
        m_rewind.sendAudio(m_txBurst);
        m_txBurst.clear();
    }
    if (m_stopping && m_encodesInFlight == 0)
        finishTransmit();
}

void RadioController::finishTransmit()
{
    m_txFlushTimer.stop();
    if (!m_transmitting)
        return;
    if (!m_txBurst.isEmpty()) {
        const auto *silence = reinterpret_cast<const char *>(AmbeFrame::kSilence72.data());
        while (m_txBurst.size() < kBurstBytes)
            m_txBurst.append(silence, AmbeFrame::kFrame72Bytes);
        m_rewind.sendAudio(m_txBurst);
        m_txBurst.clear();
    }
    m_rewind.endCall();
    m_transmitting = false;
    m_stopping = false;
    m_lastHeard.endCall();
    emit callChanged();
}
