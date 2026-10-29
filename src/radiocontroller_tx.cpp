// RadioController: transmitting, see radiocontroller.h.
//
// Microphone frames (20 ms) go to the vocoder; every three encoded frames
// make one 60 ms burst for the network. When PTT is released the frames still
// at the vocoder are awaited briefly, the last burst is padded with silence
// and the call is closed with a terminator.

#include "radiocontroller.h"

#include <QDebug>
#include <QSettings>

namespace {

constexpr int kBurstBytes = 3 * AmbeFrame::kFrame72Bytes;
// Time to wait for the vocoder's last answers after PTT release.
constexpr int kTxFlushMs = 250;
// Level meter: how fast the shown peak falls (dB per 20 ms frame), and how
// long the limiter warning stays on (frames).
constexpr double kMicFallDbPerFrame = 0.6;
constexpr int kMicLimitedHoldFrames = 25;

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
    stopMicTest();
    m_txMicFrames = 0;
    m_txMicPeakMax = kMicFloorDb;
    m_txMicPeakSum = 0.0;
    m_txMicLimited = 0;

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
    resetMicLevel();
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
    if (m_txMicFrames > 0)
        qDebug().nospace() << "RadioController: mic gain " << m_micGainDb << " dB, peak max "
                           << qRound(m_txMicPeakMax) << " dBFS, average "
                           << qRound(m_txMicPeakSum / m_txMicFrames) << " dBFS, limited "
                           << (100 * m_txMicLimited / m_txMicFrames) << "% of "
                           << m_txMicFrames << " frames";
    m_transmitting = false;
    m_stopping = false;
    m_lastHeard.endCall();
    emit callChanged();
}

void RadioController::setMicGainDb(double db)
{
    db = qBound(kMicGainMinDb, db, kMicGainMaxDb);
    if (qFuzzyCompare(db + 100.0, m_micGainDb + 100.0))
        return;
    m_micGainDb = db;
    m_recorder.setGainDb(db);
    QSettings().setValue(QStringLiteral("micGainDb"), db);
    emit micGainDbChanged();
}

void RadioController::startMicTest()
{
    if (m_transmitting || m_micTesting)
        return;
    if (!m_recorder.start()) {
        setStatus(tr("Microphone not available"));
        return;
    }
    m_micTesting = true;
    m_micTestTimer.start();
    emit micTestingChanged();
}

void RadioController::stopMicTest()
{
    if (!m_micTesting)
        return;
    m_micTestTimer.stop();
    m_micTesting = false;
    m_recorder.stop();
    resetMicLevel();
    emit micTestingChanged();
}

void RadioController::onMicLevel(double peakDbfs, bool limited)
{
    if (m_transmitting && !m_stopping) {
        ++m_txMicFrames;
        m_txMicPeakMax = qMax(m_txMicPeakMax, peakDbfs);
        m_txMicPeakSum += qMax(kMicFloorDb, peakDbfs);
        if (limited)
            ++m_txMicLimited;
    }
    const double level = qMax(kMicFloorDb, peakDbfs);
    m_micLevelDb = qMax(level, m_micLevelDb - kMicFallDbPerFrame);
    m_micLimitedFrames = limited ? kMicLimitedHoldFrames : qMax(0, m_micLimitedFrames - 1);
    emit micLevelChanged();
}

void RadioController::resetMicLevel()
{
    m_micLevelDb = kMicFloorDb;
    m_micLimitedFrames = 0;
    emit micLevelChanged();
}
