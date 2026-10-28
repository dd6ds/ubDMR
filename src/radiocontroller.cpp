// RadioController: connection, talkgroups and receiving, see radiocontroller.h.

#include "radiocontroller.h"

#include <QDebug>

#include <algorithm>

namespace {

// A call is over when its terminator arrives, or after this much silence
// (terminators get lost on UDP).
constexpr int kReceiveTimeoutMs = 1500;
constexpr int kBurstBytes = 3 * AmbeFrame::kFrame72Bytes;
// IDs above this carry a two-digit hotspot suffix (e.g. 262123401).
constexpr quint32 kMaxPlainDmrId = 9999999;

} // namespace

RadioController::RadioController(QObject *parent) : QObject(parent)
{
    m_receiveWatchdog.setSingleShot(true);
    m_receiveWatchdog.setInterval(kReceiveTimeoutMs);
    connect(&m_receiveWatchdog, &QTimer::timeout, this, &RadioController::endReceive);

    connect(&m_rewind, &RewindClient::stateChanged, this, &RadioController::onRewindState);
    connect(&m_rewind, &RewindClient::callStarted, this, &RadioController::onCallStarted);
    connect(&m_rewind, &RewindClient::audioReceived, this, &RadioController::onAudioReceived);
    connect(&m_rewind, &RewindClient::callEnded, this, &RadioController::endReceive);
    connect(&m_rewind, &RewindClient::problem, this, &RadioController::setStatus);
    connect(&m_rewind, &RewindClient::textReceived, this, &RadioController::onTextReceived);
    connect(&m_rewind, &RewindClient::textDelivered, this, &RadioController::onTextDelivered);

    connectVocoder(&m_softwareVocoder);
    connectVocoder(&m_serverVocoder);

    connect(&m_recorder, &AudioRecorder::frameReady, this, &RadioController::onMicFrame);

    m_txFlushTimer.setSingleShot(true);
    connect(&m_txFlushTimer, &QTimer::timeout, this, &RadioController::finishTransmit);
    m_txLimitTimer.setSingleShot(true);
    connect(&m_txLimitTimer, &QTimer::timeout, this, &RadioController::stopTransmit);

    m_lookup.refreshTalkgroups();
}

// Only the active vocoder is ever fed, so both can share the handlers.
void RadioController::connectVocoder(Vocoder *vocoder)
{
    connect(vocoder, &Vocoder::readyChanged, this, &RadioController::vocoderReadyChanged);
    connect(vocoder, &Vocoder::decoded, this, &RadioController::onDecoded);
    connect(vocoder, &Vocoder::encoded, this, &RadioController::onEncoded);
    connect(vocoder, &Vocoder::problem, this, &RadioController::setStatus);
}

bool RadioController::connecting() const
{
    const RewindClient::State state = m_rewind.state();
    return state == RewindClient::State::Resolving || state == RewindClient::State::LoggingIn;
}

void RadioController::setStatus(const QString &text)
{
    if (!text.isEmpty())
        qDebug() << "RadioController:" << text;
    m_statusText = text;
    emit statusTextChanged();
}

quint32 RadioController::sourceId() const
{
    const quint32 id = m_settings.value(QStringLiteral("dmrId")).toUInt();
    return id > kMaxPlainDmrId ? id / 100 : id;
}

void RadioController::connectToNetwork(const QVariantMap &settings)
{
    m_rewind.unsubscribe(RewindClient::PrivateVoice, sourceId());
    m_settings = settings;
    RewindClient::Login login;
    login.host = settings.value(QStringLiteral("host")).toString().trimmed();
    login.port = quint16(settings.value(QStringLiteral("port")).toUInt());
    // The terminal logs in with the plain ID, never with a hotspot suffix.
    login.remoteId = sourceId();
    login.password = settings.value(QStringLiteral("password")).toString();
    if (login.host.isEmpty() || login.remoteId == 0 || login.password.isEmpty()) {
        setStatus(tr("Server, DMR ID and password are required"));
        return;
    }
    setStatus(QString());
    m_rewind.subscribe(RewindClient::PrivateVoice, sourceId());
    m_rewind.connectToServer(login);
    selectVocoder();
}

void RadioController::disconnectFromNetwork()
{
    stopTransmit();
    endReceive();
    m_rewind.unsubscribe(RewindClient::PrivateVoice, sourceId());
    m_settings.clear();
    m_rewind.disconnectFromServer();
    m_serverVocoder.close();
}

void RadioController::onApplicationActive()
{
    if (m_settings.isEmpty())
        return;
    if (m_rewind.state() == RewindClient::State::Disconnected) {
        qDebug() << "RadioController: app resumed while offline, logging in again";
        connectToNetwork(m_settings);
    } else if (m_vocoder == &m_serverVocoder && !m_serverVocoder.isReady()
               && !m_serverVocoder.isOpening()) {
        qDebug() << "RadioController: app resumed without vocoder, reopening it";
        openServerVocoder();
    }
}

void RadioController::selectVocoder()
{
    if (m_settings.value(QStringLiteral("vocoder")).toString() == QLatin1String("ambeserver")) {
        m_vocoder = &m_serverVocoder;
        openServerVocoder();
    } else {
        m_serverVocoder.close();
        m_vocoder = &m_softwareVocoder;
    }
    emit vocoderReadyChanged();
}

void RadioController::openServerVocoder()
{
    m_serverVocoder.open(m_settings.value(QStringLiteral("vocoderHost")).toString().trimmed(),
                         quint16(m_settings.value(QStringLiteral("vocoderPort")).toUInt()));
}

void RadioController::onRewindState(RewindClient::State state)
{
    if (state == RewindClient::State::Connected) {
        setStatus(QString());
        m_lookup.refreshTalkgroups();
    } else
        endReceive();
    emit connectionChanged();
}

void RadioController::setTalkgroup(int talkgroup)
{
    const quint32 tg = quint32(qMax(0, talkgroup));
    if (tg == m_talkgroup)
        return;
    if (m_talkgroup)
        m_rewind.unsubscribe(RewindClient::GroupVoice, m_talkgroup);
    m_talkgroup = tg;
    if (m_talkgroup)
        m_rewind.subscribe(RewindClient::GroupVoice, m_talkgroup);
    emit talkgroupChanged();
}

void RadioController::onCallStarted(quint32 type, quint32 sourceId, quint32 destinationId)
{
    if (m_transmitting)
        return;
    // The server repeats the header; the same call is not a new one.
    if (m_receiving && m_call.sourceId == sourceId && m_call.destinationId == destinationId)
        return;
    LastHeardModel::Call call;
    call.sourceId = sourceId;
    call.destinationId = destinationId;
    call.isPrivate = type == RewindClient::PrivateVoice;
    qDebug() << "RadioController: call from" << sourceId << "to" << destinationId
             << (call.isPrivate ? "(private)" : "");
    // Audio that came first started an anonymous call: name it now.
    if (m_receiving && m_call.sourceId == 0) {
        call.started = m_call.started;
        m_call = call;
        m_lastHeard.updateActiveCall(m_call);
        noteCaller();
        emit callChanged();
        return;
    }
    beginReceive(call);
}

void RadioController::onAudioReceived(const QByteArray &frames)
{
    if (m_transmitting || frames.size() < kBurstBytes)
        return;
    // Audio without a header: show the call anyway, the names stay unknown.
    if (!m_receiving) {
        LastHeardModel::Call call;
        call.destinationId = m_talkgroup;
        beginReceive(call);
    }
    m_receiveWatchdog.start();
    if (!m_vocoder->isReady())
        return;
    for (int i = 0; i < kBurstBytes; i += AmbeFrame::kFrame72Bytes) {
        AmbeFrame::Frame72 frame;
        std::copy_n(frames.constData() + i, frame.size(), frame.begin());
        m_vocoder->decode(AmbeFrame::toRaw(frame));
    }
}

void RadioController::beginReceive(const LastHeardModel::Call &call)
{
    m_call = call;
    m_call.started = QDateTime::currentDateTime();
    m_receiving = true;
    m_receiveWatchdog.start();
    m_vocoder->reset();
    m_lastHeard.beginCall(m_call);
    noteCaller();
    emit callChanged();
}

// Fetches the caller's name and remembers private callers for "Reply".
void RadioController::noteCaller()
{
    m_lookup.request(m_call.sourceId);
    if (m_call.isPrivate && m_call.sourceId && m_call.sourceId != m_lastPrivateCaller) {
        m_lastPrivateCaller = m_call.sourceId;
        emit lastPrivateCallerChanged();
    }
}

void RadioController::endReceive()
{
    m_receiveWatchdog.stop();
    if (!m_receiving)
        return;
    m_receiving = false;
    m_lastHeard.endCall();
    m_player.finish();
    emit callChanged();
}

void RadioController::onDecoded(const QByteArray &pcm)
{
    if (!m_transmitting)
        m_player.enqueue(pcm);
}

bool RadioController::sendText(int destination, bool group, const QString &text)
{
    const QString trimmed = text.trimmed();
    qDebug() << "RadioController: text to" << destination << (group ? "(group)" : "(private)")
             << "online" << online() << "length" << trimmed.size();
    if (!online() || destination <= 0 || trimmed.isEmpty())
        return false;
    m_rewind.sendText(sourceId(), quint32(destination), group, trimmed.left(kMaxTextLength));
    return true;
}

void RadioController::onTextReceived(quint32 sourceId, quint32 destinationId, bool group,
                                     const QString &text)
{
    m_lookup.request(sourceId);
    emit textReceived(int(sourceId), int(destinationId), group, text);
}

void RadioController::onTextDelivered(quint32 destinationId, bool success)
{
    emit textDelivered(int(destinationId), success);
}
