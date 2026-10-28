// DMR ID and talkgroup names, see idlookup.h.

#include "idlookup.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QUrl>

namespace {

const QString kOperatorUrl = QStringLiteral("https://radioid.net/api/dmr/user/?id=%1");
const QString kCallsignUrl = QStringLiteral("https://radioid.net/api/dmr/user/?callsign=%1");
// radioid.net returns at most this many entries per page; more are not shown.
constexpr int kMaxSearchResults = 200;
const QString kTalkgroupUrl = QStringLiteral("https://api.brandmeister.network/v2/talkgroup");
const QString kOperatorFile = QStringLiteral("operators.json");
const QString kTalkgroupFile = QStringLiteral("talkgroups.json");
constexpr int kTalkgroupMaxAgeDays = 7;

QNetworkRequest jsonRequest(const QString &url)
{
    QNetworkRequest request{QUrl(url)};
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    request.setRawHeader("Accept", "application/json");
    return request;
}

QJsonObject readJsonFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return QJsonObject();
    return QJsonDocument::fromJson(file.readAll()).object();
}

void writeJsonFile(const QString &path, const QJsonObject &object)
{
    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        file.write(QJsonDocument(object).toJson(QJsonDocument::Compact));
}

} // namespace

IdLookup::IdLookup(QObject *parent) : QObject(parent)
{
    m_cacheDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(m_cacheDir);
    loadCache();
}

void IdLookup::loadCache()
{
    const QJsonObject operators = readJsonFile(m_cacheDir + '/' + kOperatorFile);
    for (auto it = operators.begin(); it != operators.end(); ++it)
        m_operators.insert(it.key().toUInt(), it.value().toObject());
    const QJsonObject talkgroups = readJsonFile(m_cacheDir + '/' + kTalkgroupFile);
    for (auto it = talkgroups.begin(); it != talkgroups.end(); ++it)
        m_talkgroups.insert(it.key().toUInt(), it.value().toString());
}

void IdLookup::bump()
{
    ++m_revision;
    emit changed();
}

QString IdLookup::callsign(int id) const
{
    return m_operators.value(quint32(id)).value(QStringLiteral("callsign")).toString();
}

QString IdLookup::operatorText(int id) const
{
    const QJsonObject op = m_operators.value(quint32(id));
    QStringList parts;
    for (const char *key : {"fname", "city", "country"}) {
        const QString value = op.value(QLatin1String(key)).toString().trimmed();
        if (!value.isEmpty() && !parts.contains(value))
            parts << value;
    }
    return parts.join(QStringLiteral(", "));
}

QString IdLookup::talkgroupName(int id) const
{
    return m_talkgroups.value(quint32(id));
}

void IdLookup::request(quint32 id)
{
    if (id == 0 || m_operators.contains(id) || m_pending.contains(id))
        return;
    m_pending.insert(id);
    QNetworkReply *reply = m_network.get(jsonRequest(kOperatorUrl.arg(id)));
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, id] { onOperatorReply(reply, id); });
}

void IdLookup::onOperatorReply(QNetworkReply *reply, quint32 id)
{
    reply->deleteLater();
    m_pending.remove(id);
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "IdLookup: radioid.net" << id << reply->errorString();
        return;
    }
    const QJsonArray results = QJsonDocument::fromJson(reply->readAll())
            .object().value(QStringLiteral("results")).toArray();
    if (results.isEmpty())
        return;
    remember(results.first().toObject());
    saveOperators();
    bump();
}

void IdLookup::remember(const QJsonObject &op)
{
    m_operators.insert(quint32(op.value(QStringLiteral("id")).toInt()), op);
}

void IdLookup::search(const QString &text)
{
    if (m_searchReply)
        m_searchReply->abort();
    const QString query = text.trimmed().toUpper();
    bool isId = false;
    const quint32 id = query.toUInt(&isId);
    QString url;
    if (isId)
        url = kOperatorUrl.arg(id);
    else // radioid.net's wildcard is SQL's %.
        url = kCallsignUrl.arg(QString::fromLatin1(
                QUrl::toPercentEncoding(QString(query).replace(QLatin1Char('*'),
                                                                QLatin1Char('%')))));
    QNetworkReply *reply = m_network.get(jsonRequest(url));
    m_searchReply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply] { onSearchReply(reply); });
}

void IdLookup::onSearchReply(QNetworkReply *reply)
{
    reply->deleteLater();
    if (reply == m_searchReply)
        m_searchReply = nullptr;
    if (reply->error() == QNetworkReply::OperationCanceledError)
        return;
    if (reply->error() != QNetworkReply::NoError) {
        emit searchFinished(QVariantList(), reply->errorString());
        return;
    }
    const QJsonArray found = QJsonDocument::fromJson(reply->readAll())
            .object().value(QStringLiteral("results")).toArray();
    QVariantList results;
    for (const QJsonValue &value : found) {
        if (results.size() >= kMaxSearchResults)
            break;
        const QJsonObject op = value.toObject();
        remember(op);
        const quint32 id = quint32(op.value(QStringLiteral("id")).toInt());
        results << QVariantMap{{QStringLiteral("id"), id},
                               {QStringLiteral("callsign"), callsign(int(id))},
                               {QStringLiteral("name"), operatorText(int(id))}};
    }
    if (!found.isEmpty()) {
        saveOperators();
        bump();
    }
    emit searchFinished(results, QString());
}

void IdLookup::saveOperators() const
{
    QJsonObject all;
    for (auto it = m_operators.begin(); it != m_operators.end(); ++it) {
        QJsonObject slim;
        for (const char *key : {"callsign", "fname", "city", "country"})
            slim.insert(QLatin1String(key), it.value().value(QLatin1String(key)));
        all.insert(QString::number(it.key()), slim);
    }
    writeJsonFile(m_cacheDir + '/' + kOperatorFile, all);
}

void IdLookup::refreshTalkgroups()
{
    const QFileInfo cache(m_cacheDir + '/' + kTalkgroupFile);
    if (cache.exists()
            && cache.lastModified().daysTo(QDateTime::currentDateTime()) < kTalkgroupMaxAgeDays)
        return;
    QNetworkReply *reply = m_network.get(jsonRequest(kTalkgroupUrl));
    connect(reply, &QNetworkReply::finished, this, [this, reply] { onTalkgroupReply(reply); });
}

void IdLookup::onTalkgroupReply(QNetworkReply *reply)
{
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "IdLookup: talkgroup list" << reply->errorString();
        return;
    }
    const QJsonObject talkgroups = QJsonDocument::fromJson(reply->readAll()).object();
    if (talkgroups.isEmpty())
        return;
    m_talkgroups.clear();
    for (auto it = talkgroups.begin(); it != talkgroups.end(); ++it)
        m_talkgroups.insert(it.key().toUInt(), it.value().toString());
    writeJsonFile(m_cacheDir + '/' + kTalkgroupFile, talkgroups);
    bump();
}
