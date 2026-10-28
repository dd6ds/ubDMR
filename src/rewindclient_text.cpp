// Rewind protocol client: text messages, see rewindclient.h.
//
// MESSAGE_TEXT carries reserved(4), source ID(4), destination ID(4),
// option(2: 128 = talkgroup, 0 = private), length in bytes(2) and the text
// as UTF-16LE. The receiver of a private message answers with
// MESSAGE_STATUS: reserved(4), source(4) = its own ID, destination(4) =
// the sender, status(1) as in a DMR data call report (0x08 = success).

#include "rewindclient.h"

#include <QDebug>
#include <QtEndian>

namespace {

constexpr int kTextHeaderLength = 16;
constexpr int kStatusLength = 13;
constexpr quint16 kOptionGroup = 128;
constexpr quint8 kStatusSuccess = 0x08;

void appendLe(QByteArray &out, quint32 value, int bytes)
{
    for (int i = 0; i < bytes; ++i)
        out.append(char(value >> (8 * i)));
}

} // namespace

void RewindClient::sendText(quint32 sourceId, quint32 destinationId, bool group,
                            const QString &text)
{
    QByteArray data(reinterpret_cast<const char *>(text.utf16()), text.size() * 2);
    // utf16() is host order; Rewind wants little-endian.
    if (QSysInfo::ByteOrder == QSysInfo::BigEndian)
        qToLittleEndian<quint16>(data.constData(), text.size(), data.data());
    QByteArray payload;
    appendLe(payload, 0, 4);
    appendLe(payload, sourceId, 4);
    appendLe(payload, destinationId, 4);
    appendLe(payload, group ? kOptionGroup : 0, 2);
    appendLe(payload, quint32(data.size()), 2);
    send(kTypeMessageText, payload + data);
}

void RewindClient::handleTextMessage(const QByteArray &payload)
{
    if (payload.size() < kTextHeaderLength)
        return;
    const auto *raw = reinterpret_cast<const uchar *>(payload.constData());
    const quint32 sourceId = qFromLittleEndian<quint32>(raw + 4);
    const quint32 destinationId = qFromLittleEndian<quint32>(raw + 8);
    const bool group = qFromLittleEndian<quint16>(raw + 12) & kOptionGroup;
    // Private messages to other IDs (or the server's own notices) are not ours.
    if (!group && destinationId != m_login.remoteId)
        return;
    const int length = qMin<int>(qFromLittleEndian<quint16>(raw + 14),
                                 payload.size() - kTextHeaderLength) & ~1;
    QString text(length / 2, Qt::Uninitialized);
    for (int i = 0; i < length / 2; ++i)
        text[i] = QChar(qFromLittleEndian<quint16>(raw + kTextHeaderLength + 2 * i));
    // Senders often pad with NULs.
    text = text.section(QChar(0), 0, 0).trimmed();
    qDebug() << "RewindClient: text from" << sourceId << "to" << destinationId
             << (group ? "(group)" : "(private)");
    if (!group) {
        QByteArray status;
        appendLe(status, 0, 4);
        appendLe(status, destinationId, 4);
        appendLe(status, sourceId, 4);
        status.append(char(kStatusSuccess));
        send(kTypeMessageStatus, status);
    }
    if (!text.isEmpty())
        emit textReceived(sourceId, destinationId, group, text);
}

void RewindClient::handleTextStatus(const QByteArray &payload)
{
    if (payload.size() < kStatusLength)
        return;
    const auto *raw = reinterpret_cast<const uchar *>(payload.constData());
    // Source of the report is the recipient of our message.
    emit textDelivered(qFromLittleEndian<quint32>(raw + 4), raw[12] == kStatusSuccess);
}
