// AMBEserver client, see ambeservervocoder.h.
//
// AMBEserver forwards each UDP datagram unchanged to the DV3000's serial
// port and sends the chip's reply back. Packets are the chip's own format:
// 0x61, 16-bit big-endian length of the rest minus the type byte, type, fields.
//   control: RATET (0x09) <index>          -> reply 0x09 0x00 on success
//   channel: CHAND (0x01) <bits> <data>    -> decoder replies with speech
//   speech:  SPEECHD (0x00) <samples> <pcm, big-endian> -> encoder replies
//            with channel data

#include "ambeservervocoder.h"

#include <QtEndian>

#include <algorithm>

namespace {

constexpr char kStartByte = 0x61;
constexpr char kTypeControl = 0x00;
constexpr char kTypeChannel = 0x01;
constexpr char kTypeSpeech = 0x02;
constexpr char kFieldChannelData = 0x01;
constexpr char kFieldSpeechData = 0x00;
constexpr char kFieldRateTable = 0x09;
constexpr char kRateAmbePlus2NoFec = 34;
constexpr int kHeaderLength = 4;
constexpr int kSetupTimeoutMs = 3000;

} // namespace

AmbeServerVocoder::AmbeServerVocoder(QObject *parent) : Vocoder(parent)
{
    m_setupTimer.setSingleShot(true);
    m_setupTimer.setInterval(kSetupTimeoutMs);
    connect(&m_setupTimer, &QTimer::timeout, this, &AmbeServerVocoder::onSetupTimeout);
    connect(&m_socket, &QUdpSocket::readyRead, this, &AmbeServerVocoder::onReadyRead);
}

void AmbeServerVocoder::open(const QString &host, quint16 port)
{
    close();
    m_host = host;
    m_port = port;
    m_lookupId = QHostInfo::lookupHost(host, this, &AmbeServerVocoder::onHostResolved);
}

void AmbeServerVocoder::onHostResolved(const QHostInfo &info)
{
    if (info.lookupId() != m_lookupId)
        return;
    m_lookupId = -1;
    if (info.error() != QHostInfo::NoError || info.addresses().isEmpty()) {
        emit problem(tr("Cannot resolve vocoder %1").arg(m_host));
        return;
    }
    m_address = info.addresses().first();
    m_socket.close();
    m_socket.bind(m_address.protocol() == QAbstractSocket::IPv6Protocol
                  ? QHostAddress::AnyIPv6 : QHostAddress::AnyIPv4);
    sendPacket(kTypeControl, QByteArray(1, kFieldRateTable) + char(kRateAmbePlus2NoFec));
    m_setupTimer.start();
}

void AmbeServerVocoder::onSetupTimeout()
{
    emit problem(tr("No answer from vocoder %1:%2").arg(m_host).arg(m_port));
}

void AmbeServerVocoder::close()
{
    if (m_lookupId >= 0)
        QHostInfo::abortHostLookup(m_lookupId);
    m_lookupId = -1;
    m_setupTimer.stop();
    m_socket.close();
    m_address.clear();
    setReady(false);
}

void AmbeServerVocoder::setReady(bool ready)
{
    if (m_ready == ready)
        return;
    m_ready = ready;
    emit readyChanged(ready);
}

void AmbeServerVocoder::decode(const AmbeFrame::Frame49 &frame)
{
    QByteArray fields;
    fields.append(kFieldChannelData);
    fields.append(char(AmbeFrame::kFrame49Bits));
    fields.append(reinterpret_cast<const char *>(frame.data()), int(frame.size()));
    sendPacket(kTypeChannel, fields);
}

void AmbeServerVocoder::encode(const QByteArray &pcm)
{
    QByteArray fields(2 + kPcmBytesPerFrame, '\0');
    fields[0] = kFieldSpeechData;
    fields[1] = char(kSamplesPerFrame);
    const auto *in = reinterpret_cast<const uchar *>(pcm.constData());
    auto *out = reinterpret_cast<uchar *>(fields.data()) + 2;
    for (int i = 0; i < kSamplesPerFrame; ++i)
        qToBigEndian(qFromLittleEndian<qint16>(in + 2 * i), out + 2 * i);
    sendPacket(kTypeSpeech, fields);
}

void AmbeServerVocoder::sendPacket(char type, const QByteArray &fields)
{
    if (m_address.isNull())
        return;
    QByteArray packet(kHeaderLength, '\0');
    packet[0] = kStartByte;
    qToBigEndian(quint16(fields.size()), reinterpret_cast<uchar *>(packet.data()) + 1);
    packet[3] = type;
    m_socket.writeDatagram(packet + fields, m_address, m_port);
}

void AmbeServerVocoder::onReadyRead()
{
    while (m_socket.hasPendingDatagrams()) {
        QByteArray datagram(int(m_socket.pendingDatagramSize()), '\0');
        m_socket.readDatagram(datagram.data(), datagram.size());
        if (datagram.size() > kHeaderLength && datagram[0] == kStartByte)
            handlePacket(datagram);
    }
}

void AmbeServerVocoder::handlePacket(const QByteArray &packet)
{
    const char type = packet[3];
    const QByteArray fields = packet.mid(kHeaderLength);
    if (type == kTypeControl && fields.size() >= 2 && fields[0] == kFieldRateTable) {
        m_setupTimer.stop();
        if (fields[1] == 0)
            setReady(true);
        else
            emit problem(tr("Vocoder rejected the DMR rate"));
    } else if (type == kTypeSpeech && fields.size() >= 2 + kPcmBytesPerFrame) {
        QByteArray pcm(kPcmBytesPerFrame, '\0');
        const auto *in = reinterpret_cast<const uchar *>(fields.constData()) + 2;
        auto *out = reinterpret_cast<uchar *>(pcm.data());
        for (int i = 0; i < kSamplesPerFrame; ++i)
            qToLittleEndian(qFromBigEndian<qint16>(in + 2 * i), out + 2 * i);
        emit decoded(pcm);
    } else if (type == kTypeChannel && fields.size() >= 2 + AmbeFrame::kFrame49Bytes) {
        AmbeFrame::Frame49 frame;
        std::copy_n(fields.constData() + 2, frame.size(), frame.begin());
        emit encoded(frame);
    }
}
