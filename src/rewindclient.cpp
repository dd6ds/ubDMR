// Rewind protocol client: login, keep-alive, subscriptions and voice calls,
// see rewindclient.h. Text messages live in rewindclient_text.cpp.
//
// Login: we send KEEP_ALIVE, the server answers with a 4-byte CHALLENGE, we
// reply with SHA-256(challenge + password), and the server's empty KEEP_ALIVE
// confirms the login. A wrong password gets no answer at all, so silence after
// the answer means "rejected". Silence from the server for kTimeoutMs once
// logged in means the session is gone (also after the phone suspended the
// app), and the client starts over.
//
// Calls: terminals get no super header. Each call is announced with a DMR
// voice header (sent three times) carrying the full LC with both IDs, then
// audio frames, then a terminator; we transmit the same sequence.

#include "rewindclient.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QHostInfo>
#include <QtEndian>

namespace {

const QByteArray kSign = QByteArrayLiteral("REWIND01");
constexpr int kHeaderLength = 18;
constexpr int kKeepAliveMs = 5000;
constexpr int kTimeoutMs = 15000;
constexpr int kLoginReplyMs = 8000;
constexpr int kResolveRetryMs = 5000;
// The server repeats the voice header three times; so do we.
constexpr int kVoiceHeaderRepeats = 3;
const QByteArray kDescription = QByteArrayLiteral("ubDMR 0.1.0 Ubuntu Touch");

// Packet types (class + offset).
enum : quint16 {
    TypeKeepAlive = 0x0000,
    TypeClose = 0x0001,
    TypeChallenge = 0x0002,
    TypeAuthentication = 0x0003,
    TypeReport = 0x0100,
    TypeBusyNotice = 0x0200,
    TypeSubscription = 0x0901,
    TypeCancelling = 0x0902,
    TypeVoiceHeader = 0x0911,
    TypeTerminator = 0x0912,
    TypeAudioFrame = 0x0920,
    TypeFailureCode = 0x0929,
};



// DMR data and audio types use their own sequence counter and this flag.
constexpr quint16 kFlagRealTime = 0x0001;
constexpr quint8 kServiceTerminal = 0x21;

void appendLe32(QByteArray &out, quint32 value)
{
    char buf[4];
    qToLittleEndian(value, buf);
    out.append(buf, 4);
}

quint64 subscriptionKey(RewindClient::SessionType type, quint32 id)
{
    return (quint64(type) << 32) | id;
}

} // namespace

RewindClient::RewindClient(QObject *parent) : QObject(parent)
{
    m_keepAliveTimer.setInterval(kKeepAliveMs);
    connect(&m_keepAliveTimer, &QTimer::timeout, this, &RewindClient::onKeepAliveTimer);
    connect(&m_socket, &QUdpSocket::readyRead, this, &RewindClient::onReadyRead);
    m_resolveRetryTimer.setSingleShot(true);
    m_resolveRetryTimer.setInterval(kResolveRetryMs);
    connect(&m_resolveRetryTimer, &QTimer::timeout, this, &RewindClient::lookUpHost);
}

void RewindClient::setState(State state)
{
    if (m_state == state)
        return;
    m_state = state;
    emit stateChanged(state);
}

void RewindClient::connectToServer(const Login &login)
{
    disconnectFromServer();
    m_login = login;
    setState(State::Resolving);
    lookUpHost();
}

void RewindClient::lookUpHost()
{
    m_lookupId = QHostInfo::lookupHost(m_login.host, this, &RewindClient::onHostResolved);
}

void RewindClient::onHostResolved(const QHostInfo &info)
{
    if (m_state != State::Resolving || info.lookupId() != m_lookupId)
        return;
    // Usually no network yet (Wi-Fi still connecting): keep trying.
    if (info.error() != QHostInfo::NoError || info.addresses().isEmpty()) {
        emit problem(tr("Cannot resolve %1, no network? Retrying…").arg(m_login.host));
        m_resolveRetryTimer.start();
        return;
    }
    m_address = info.addresses().first();
    m_socket.close();
    m_socket.bind(m_address.protocol() == QAbstractSocket::IPv6Protocol
                  ? QHostAddress::AnyIPv6 : QHostAddress::AnyIPv4);
    m_lastPacketMs = QDateTime::currentMSecsSinceEpoch();
    m_authSentMs = 0;
    setState(State::LoggingIn);
    sendKeepAlive();
    m_keepAliveTimer.start();
}

void RewindClient::disconnectFromServer()
{
    if (m_state == State::Connected)
        send(TypeClose, QByteArray());
    m_keepAliveTimer.stop();
    m_resolveRetryTimer.stop();
    m_socket.close();
    setState(State::Disconnected);
}

void RewindClient::onKeepAliveTimer()
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (m_state == State::LoggingIn && m_authSentMs && now - m_authSentMs > kLoginReplyMs) {
        disconnectFromServer();
        emit problem(tr("Login rejected: check DMR ID and hotspot password"));
        return;
    }
    if (now - m_lastPacketMs > kTimeoutMs) {
        qDebug() << "RewindClient: server timeout, logging in again";
        m_lastPacketMs = QDateTime::currentMSecsSinceEpoch();
        setState(State::LoggingIn);
    }
    sendKeepAlive();
}

void RewindClient::onReadyRead()
{
    while (m_socket.hasPendingDatagrams()) {
        QByteArray datagram(int(m_socket.pendingDatagramSize()), '\0');
        m_socket.readDatagram(datagram.data(), datagram.size());
        if (datagram.size() < kHeaderLength || !datagram.startsWith(kSign))
            continue;
        const auto *raw = reinterpret_cast<const uchar *>(datagram.constData());
        const quint16 type = qFromLittleEndian<quint16>(raw + 8);
        const quint16 length = qFromLittleEndian<quint16>(raw + 16);
        m_lastPacketMs = QDateTime::currentMSecsSinceEpoch();
        handlePacket(type, datagram.mid(kHeaderLength, length));
    }
}

void RewindClient::handlePacket(quint16 type, const QByteArray &payload)
{
    switch (type) {
    case TypeChallenge:
        answerChallenge(payload);
        break;
    case TypeKeepAlive:
        if (m_state == State::LoggingIn)
            onLoggedIn();
        break;
    case TypeClose:
        emit problem(tr("Server closed the session"));
        setState(State::LoggingIn);
        break;
    case TypeReport:
        qDebug() << "RewindClient: report" << payload;
        break;
    case TypeBusyNotice:
        emit problem(tr("Talkgroup busy"));
        break;
    case TypeFailureCode:
        emit problem(tr("Server refused the call"));
        break;
    case TypeVoiceHeader:
        handleVoiceHeader(payload);
        break;
    case TypeAudioFrame:
        emit audioReceived(payload);
        break;
    case TypeTerminator:
        emit callEnded();
        break;
    case kTypeMessageText:
        handleTextMessage(payload);
        break;
    case kTypeMessageStatus:
        handleTextStatus(payload);
        break;
    default:
        break;
    }
}

void RewindClient::answerChallenge(const QByteArray &challenge)
{
    if (m_state == State::Connected)
        setState(State::LoggingIn);
    m_authSentMs = QDateTime::currentMSecsSinceEpoch();
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(challenge);
    hash.addData(m_login.password.toUtf8());
    send(TypeAuthentication, hash.result());
}

void RewindClient::onLoggedIn()
{
    m_authSentMs = 0;
    setState(State::Connected);
    for (quint64 key : qAsConst(m_subscriptions))
        sendSubscription(TypeSubscription, SessionType(key >> 32), quint32(key));
}

void RewindClient::handleVoiceHeader(const QByteArray &payload)
{
    FullLc::Call call;
    if (FullLc::parse(payload, &call))
        emit callStarted(call.isPrivate ? PrivateVoice : GroupVoice, call.sourceId,
                         call.destinationId);
}

void RewindClient::subscribe(SessionType type, quint32 id)
{
    m_subscriptions.insert(subscriptionKey(type, id));
    if (m_state == State::Connected)
        sendSubscription(TypeSubscription, type, id);
}

void RewindClient::unsubscribe(SessionType type, quint32 id)
{
    if (m_subscriptions.remove(subscriptionKey(type, id)) && m_state == State::Connected)
        sendSubscription(TypeCancelling, type, id);
}

void RewindClient::startCall(SessionType type, quint32 sourceId, quint32 destinationId)
{
    m_txCall.isPrivate = type == PrivateVoice;
    m_txCall.sourceId = sourceId;
    m_txCall.destinationId = destinationId;
    const QByteArray header = FullLc::build(m_txCall, FullLc::Kind::VoiceHeader);
    for (int i = 0; i < kVoiceHeaderRepeats; ++i)
        send(TypeVoiceHeader, header, true);
}

void RewindClient::sendAudio(const QByteArray &frames)
{
    send(TypeAudioFrame, frames, true);
}

void RewindClient::endCall()
{
    send(TypeTerminator, FullLc::build(m_txCall, FullLc::Kind::Terminator), true);
}

void RewindClient::sendKeepAlive()
{
    QByteArray version;
    appendLe32(version, m_login.remoteId);
    version.append(char(kServiceTerminal));
    version += kDescription;
    send(TypeKeepAlive, version);
}

void RewindClient::sendSubscription(quint16 type, SessionType session, quint32 id)
{
    QByteArray data;
    appendLe32(data, session);
    appendLe32(data, id);
    send(type, data);
}

void RewindClient::send(quint16 type, const QByteArray &payload, bool realTime)
{
    if (m_address.isNull())
        return;
    QByteArray packet = kSign;
    char buf[4];
    qToLittleEndian(type, buf);
    packet.append(buf, 2);
    qToLittleEndian(quint16(realTime ? kFlagRealTime : 0), buf);
    packet.append(buf, 2);
    appendLe32(packet, realTime ? m_realTimeSequence++ : m_sequence++);
    qToLittleEndian(quint16(payload.size()), buf);
    packet.append(buf, 2);
    packet += payload;
    m_socket.writeDatagram(packet, m_address, m_login.port);
}
