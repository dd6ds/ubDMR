// List of recent calls, newest first, for the "Last heard" page.

#ifndef LASTHEARDMODEL_H
#define LASTHEARDMODEL_H

#include <QAbstractListModel>
#include <QDateTime>
#include <QVector>

class LastHeardModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        SourceIdRole = Qt::UserRole + 1,
        SourceCallRole,
        DestinationIdRole,
        PrivateRole,
        OutgoingRole,
        TimeRole,
        DurationRole,
        ActiveRole,
    };

    struct Call {
        quint32 sourceId = 0;
        QString sourceCall;
        quint32 destinationId = 0;
        bool isPrivate = false;
        bool outgoing = false;
        QDateTime started;
        int durationSecs = 0;
        bool active = true;
    };

    explicit LastHeardModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void beginCall(const Call &call);
    // Replaces the running call at the top (its IDs became known late).
    void updateActiveCall(const Call &call);
    // Ends the call at the top of the list, if one is running.
    void endCall();

private:
    QVector<Call> m_calls;
};

#endif
