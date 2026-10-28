// UDP client for BrandMeister's Rewind protocol in its "Open DMR Terminal"
// flavour: a master server's port 54006, logged in with the user's own DMR ID
// and the hotspot password set in BrandMeister SelfCare.
// All fields are little-endian; the layout follows BrandMeister's Rewind.h.
//
// rewindclient.cpp       login, keep-alive, subscriptions, voice calls
// rewindclient_text.cpp  text messages

#ifndef REWINDCLIENT_H
#define REWINDCLIENT_H

#include <QByteArray>
#include <QHostAddress>
#include <QHostInfo>
#include <QObject>
#include <QSet>
#include <QTimer>
#include <QUdpSocket>

#include "fulllc.h"

class RewindClient : public QObject
{
    Q_OBJECT

public:
    enum class State { Disconnected, Resolving, LoggingIn, Connected };

    enum SessionType : quint32 { PrivateVoice = 5, GroupVoice = 7 };

    struct Login {
        QString host;
        quint16 port = 0;
        quint32 remoteId = 0;
        QString password;
    };

    explicit RewindClient(QObject *parent = nullptr);

    State state() const { return m_state; }

    void connectToServer(const Login &login);
    void disconnectFromServer();

    // Subscriptions are remembered and renewed after every (re)login.
    void subscribe(SessionType type, quint32 id);
    void unsubscribe(SessionType type, quint32 id);

    void startCall(SessionType type, quint32 sourceId, quint32 destinationId);
    // Three 72-bit AMBE frames (27 bytes), one DMR voice burst = 60 ms.
    void sendAudio(const QByteArray &frames);
    void endCall();

    // Text messages: UTF-16 text to a station (private) or a talkgroup.
    void sendText(quint32 sourceId, quint32 destinationId, bool group, const QString &text);

signals:
    void stateChanged(RewindClient::State state);
    void callStarted(quint32 type, quint32 sourceId, quint32 destinationId);
    void audioReceived(const QByteArray &frames);
    void callEnded();
    void textReceived(quint32 sourceId, quint32 destinationId, bool group, const QString &text);
    // Delivery report for a private message we sent to destinationId.
    void textDelivered(quint32 destinationId, bool success);
    void problem(const QString &text);

private:
    // Open DMR Terminal message types (used by both source files).
    static constexpr quint16 kTypeMessageText = 0x0a10;
    static constexpr quint16 kTypeMessageStatus = 0x0a11;

    void setState(State state);
    void lookUpHost();
    void onHostResolved(const QHostInfo &info);
    void onReadyRead();
    void onKeepAliveTimer();
    void handlePacket(quint16 type, const QByteArray &payload);
    void answerChallenge(const QByteArray &challenge);
    void handleVoiceHeader(const QByteArray &payload);
    void handleTextMessage(const QByteArray &payload);
    void handleTextStatus(const QByteArray &payload);
    void onLoggedIn();

    void send(quint16 type, const QByteArray &payload, bool realTime = false);
    void sendKeepAlive();
    void sendSubscription(quint16 type, SessionType session, quint32 id);

    QUdpSocket m_socket;
    QTimer m_keepAliveTimer;
    QTimer m_resolveRetryTimer;
    Login m_login;
    FullLc::Call m_txCall;
    QHostAddress m_address;
    State m_state = State::Disconnected;
    qint64 m_lastPacketMs = 0;
    int m_lookupId = -1;
    qint64 m_authSentMs = 0;
    quint32 m_sequence = 0;
    quint32 m_realTimeSequence = 0;
    QSet<quint64> m_subscriptions;
};

#endif
