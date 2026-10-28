// Names for DMR IDs and talkgroups, and the station search.
//
// Operators come from radioid.net, one request per unknown ID; talkgroup
// names from BrandMeister's public talkgroup list. Both are cached on disk so
// known stations show up offline and after a restart. search() looks up a
// callsign (with * as wildcard) or a DMR ID for the contacts page.

#ifndef IDLOOKUP_H
#define IDLOOKUP_H

#include <QHash>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QSet>
#include <QVariantList>

class QNetworkReply;

class IdLookup : public QObject
{
    Q_OBJECT
    // Bumped whenever a name becomes known, so QML bindings re-read them.
    Q_PROPERTY(int revision READ revision NOTIFY changed)

public:
    explicit IdLookup(QObject *parent = nullptr);

    int revision() const { return m_revision; }

    Q_INVOKABLE QString callsign(int id) const;
    // e.g. "Holger, Berlin"
    Q_INVOKABLE QString operatorText(int id) const;
    Q_INVOKABLE QString talkgroupName(int id) const;

    void request(quint32 id);
    Q_INVOKABLE void search(const QString &text);
    void refreshTalkgroups();

signals:
    void changed();
    // [{ id, callsign, name, location }], or an error text.
    void searchFinished(const QVariantList &results, const QString &error);

private:
    void onOperatorReply(QNetworkReply *reply, quint32 id);
    void onTalkgroupReply(QNetworkReply *reply);
    void onSearchReply(QNetworkReply *reply);
    void remember(const QJsonObject &op);
    void loadCache();
    void saveOperators() const;
    void bump();

    QNetworkAccessManager m_network;
    QHash<quint32, QJsonObject> m_operators;
    QHash<quint32, QString> m_talkgroups;
    QSet<quint32> m_pending;
    QNetworkReply *m_searchReply = nullptr;
    QString m_cacheDir;
    int m_revision = 0;
};

#endif
