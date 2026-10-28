// AMBE+2 vocoder reached over the network: an AMBEserver (DV3000 / ThumbDV
// behind a UDP port, default 2460) somewhere on the LAN or VPN.
//
// The chip runs at rate index 34 (2450 bit/s, no FEC), so it takes and
// returns bare 49-bit frames; the FEC of the network format is handled by
// AmbeFrame. Sounds better than SoftwareVocoder, but needs the hardware.

#ifndef AMBESERVERVOCODER_H
#define AMBESERVERVOCODER_H

#include <QHostAddress>
#include <QHostInfo>
#include <QTimer>
#include <QUdpSocket>

#include "vocoder.h"

class AmbeServerVocoder : public Vocoder
{
    Q_OBJECT

public:
    explicit AmbeServerVocoder(QObject *parent = nullptr);

    bool isReady() const override { return m_ready; }
    // Between open() and the chip's answer (or the timeout).
    bool isOpening() const { return m_lookupId >= 0 || m_setupTimer.isActive(); }

    // Resolves the host and configures the chip; readyChanged(true) follows
    // once the chip has confirmed the rate.
    void open(const QString &host, quint16 port);
    void close();

    void decode(const AmbeFrame::Frame49 &frame) override;
    void encode(const QByteArray &pcm) override;

private:
    void onHostResolved(const QHostInfo &info);
    void onReadyRead();
    void onSetupTimeout();
    void handlePacket(const QByteArray &packet);
    void setReady(bool ready);
    void sendPacket(char type, const QByteArray &fields);

    QUdpSocket m_socket;
    QTimer m_setupTimer;
    QString m_host;
    QHostAddress m_address;
    quint16 m_port = 0;
    int m_lookupId = -1;
    bool m_ready = false;
};

#endif
