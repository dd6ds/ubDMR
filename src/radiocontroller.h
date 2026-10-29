// The radio as QML sees it ("Radio"): network login, the talkgroup being
// monitored, the call in progress and push-to-talk. Wires RewindClient,
// the active vocoder and the audio classes together.
//
// radiocontroller.cpp     connection, talkgroups, receiving
// radiocontroller_tx.cpp  transmitting

#ifndef RADIOCONTROLLER_H
#define RADIOCONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QVariantMap>

#include "ambeservervocoder.h"
#include "audioplayer.h"
#include "audiorecorder.h"
#include "idlookup.h"
#include "lastheardmodel.h"
#include "rewindclient.h"
#include "softwarevocoder.h"

class RadioController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool online READ online NOTIFY connectionChanged)
    Q_PROPERTY(bool connecting READ connecting NOTIFY connectionChanged)
    Q_PROPERTY(bool vocoderReady READ vocoderReady NOTIFY vocoderReadyChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(int talkgroup READ talkgroup NOTIFY talkgroupChanged)
    Q_PROPERTY(bool receiving READ receiving NOTIFY callChanged)
    Q_PROPERTY(bool transmitting READ transmitting NOTIFY callChanged)
    Q_PROPERTY(int callSourceId READ callSourceId NOTIFY callChanged)
    Q_PROPERTY(QString callSourceCall READ callSourceCall NOTIFY callChanged)
    Q_PROPERTY(int callDestination READ callDestination NOTIFY callChanged)
    Q_PROPERTY(bool callPrivate READ callPrivate NOTIFY callChanged)
    Q_PROPERTY(double callStartedMs READ callStartedMs NOTIFY callChanged)
    Q_PROPERTY(int transmitLimitMs READ transmitLimitMs CONSTANT)
    Q_PROPERTY(int maxTextLength READ maxTextLength CONSTANT)
    Q_PROPERTY(int lastPrivateCaller READ lastPrivateCaller NOTIFY lastPrivateCallerChanged)
    Q_PROPERTY(LastHeardModel *lastHeard READ lastHeard CONSTANT)
    Q_PROPERTY(IdLookup *lookup READ lookup CONSTANT)
    // Microphone: gain in dB (stored), level of the last frames in dB full
    // scale with a falling peak, whether the limiter is working, and the
    // test mode that runs the microphone without transmitting.
    Q_PROPERTY(double micGainDb READ micGainDb WRITE setMicGainDb NOTIFY micGainDbChanged)
    Q_PROPERTY(double micGainMinDb READ micGainMinDb CONSTANT)
    Q_PROPERTY(double micGainMaxDb READ micGainMaxDb CONSTANT)
    Q_PROPERTY(double micLevelDb READ micLevelDb NOTIFY micLevelChanged)
    Q_PROPERTY(bool micLimited READ micLimited NOTIFY micLevelChanged)
    Q_PROPERTY(bool micTesting READ micTesting NOTIFY micTestingChanged)

public:
    // Hard limit for one transmission, like a radio's time-out timer.
    static constexpr int kTransmitLimitMs = 180000;
    // Longest text message sent; DMR short data services carry about this much.
    static constexpr int kMaxTextLength = 140;
    static constexpr double kMicGainMinDb = -10.0;
    static constexpr double kMicGainMaxDb = 30.0;
    // Lowest level the meter shows.
    static constexpr double kMicFloorDb = -60.0;

    explicit RadioController(QObject *parent = nullptr);

    bool online() const { return m_rewind.state() == RewindClient::State::Connected; }
    bool connecting() const;
    bool vocoderReady() const { return m_vocoder->isReady(); }
    QString statusText() const { return m_statusText; }
    int talkgroup() const { return int(m_talkgroup); }
    bool receiving() const { return m_receiving; }
    bool transmitting() const { return m_transmitting; }
    int callSourceId() const { return int(m_call.sourceId); }
    QString callSourceCall() const { return m_call.sourceCall; }
    int callDestination() const { return int(m_call.destinationId); }
    bool callPrivate() const { return m_call.isPrivate; }
    double callStartedMs() const { return double(m_call.started.toMSecsSinceEpoch()); }
    int transmitLimitMs() const { return kTransmitLimitMs; }
    int maxTextLength() const { return kMaxTextLength; }
    int lastPrivateCaller() const { return int(m_lastPrivateCaller); }
    LastHeardModel *lastHeard() { return &m_lastHeard; }
    IdLookup *lookup() { return &m_lookup; }
    double micGainDb() const { return m_micGainDb; }
    void setMicGainDb(double db);
    double micGainMinDb() const { return kMicGainMinDb; }
    double micGainMaxDb() const { return kMicGainMaxDb; }
    double micLevelDb() const { return m_micLevelDb; }
    bool micLimited() const { return m_micLimitedFrames > 0; }
    bool micTesting() const { return m_micTesting; }

    // settings: host, port, dmrId, password, callsign, vocoder ("builtin" or
    // "ambeserver"), vocoderHost, vocoderPort
    Q_INVOKABLE void connectToNetwork(const QVariantMap &settings);
    Q_INVOKABLE void disconnectFromNetwork();
    Q_INVOKABLE void setTalkgroup(int talkgroup);
    Q_INVOKABLE void startTransmit(int destination, bool privateCall);
    Q_INVOKABLE void stopTransmit();
    // Opens the microphone for the level meter only; ends by itself after a minute.
    Q_INVOKABLE void startMicTest();
    Q_INVOKABLE void stopMicTest();
    // False when offline or the text is empty.
    Q_INVOKABLE bool sendText(int destination, bool group, const QString &text);

    // Lomiri may have suspended the app; bring the links back.
    void onApplicationActive();

signals:
    void connectionChanged();
    void vocoderReadyChanged();
    void statusTextChanged();
    void talkgroupChanged();
    void callChanged();
    void lastPrivateCallerChanged();
    void micGainDbChanged();
    void micLevelChanged();
    void micTestingChanged();
    void textReceived(int source, int destination, bool group, const QString &text);
    void textDelivered(int destination, bool success);

private:
    void setStatus(const QString &text);
    quint32 sourceId() const;
    void connectVocoder(Vocoder *vocoder);
    void selectVocoder();
    void openServerVocoder();
    void onRewindState(RewindClient::State state);
    void onCallStarted(quint32 type, quint32 sourceId, quint32 destinationId);
    void onAudioReceived(const QByteArray &frames);
    void beginReceive(const LastHeardModel::Call &call);
    void noteCaller();
    void endReceive();
    void onDecoded(const QByteArray &pcm);
    void onTextReceived(quint32 sourceId, quint32 destinationId, bool group,
                        const QString &text);
    void onTextDelivered(quint32 destinationId, bool success);

    void onMicFrame(const QByteArray &pcm);
    void onEncoded(const AmbeFrame::Frame49 &frame);
    void finishTransmit();
    void onMicLevel(double peakDbfs, bool limited);
    void resetMicLevel();

    RewindClient m_rewind;
    SoftwareVocoder m_softwareVocoder;
    AmbeServerVocoder m_serverVocoder;
    Vocoder *m_vocoder = &m_softwareVocoder;
    AudioPlayer m_player;
    AudioRecorder m_recorder;
    IdLookup m_lookup;
    LastHeardModel m_lastHeard;

    QVariantMap m_settings;
    QString m_statusText;
    quint32 m_talkgroup = 0;
    quint32 m_lastPrivateCaller = 0;

    LastHeardModel::Call m_call;
    bool m_receiving = false;
    QTimer m_receiveWatchdog;

    bool m_transmitting = false;
    bool m_stopping = false;
    int m_encodesInFlight = 0;
    QByteArray m_txBurst;
    QTimer m_txFlushTimer;
    QTimer m_txLimitTimer;

    double m_micGainDb = 0.0;
    double m_micLevelDb = kMicFloorDb;
    int m_micLimitedFrames = 0;
    bool m_micTesting = false;
    QTimer m_micTestTimer;
    // Per transmission, for the log: frames, loudest peak, sum of peaks,
    // frames in the limiter.
    int m_txMicFrames = 0;
    double m_txMicPeakMax = kMicFloorDb;
    double m_txMicPeakSum = 0.0;
    int m_txMicLimited = 0;
};

#endif
