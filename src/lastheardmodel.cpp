// Recent calls, see lastheardmodel.h.

#include "lastheardmodel.h"

namespace {

constexpr int kMaxCalls = 200;

} // namespace

LastHeardModel::LastHeardModel(QObject *parent) : QAbstractListModel(parent)
{
}

int LastHeardModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_calls.size();
}

QVariant LastHeardModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_calls.size())
        return QVariant();
    const Call &call = m_calls.at(index.row());
    switch (role) {
    case SourceIdRole: return call.sourceId;
    case SourceCallRole: return call.sourceCall;
    case DestinationIdRole: return call.destinationId;
    case PrivateRole: return call.isPrivate;
    case OutgoingRole: return call.outgoing;
    case TimeRole: return call.started.toString(QStringLiteral("HH:mm:ss"));
    case DurationRole: return call.durationSecs;
    case ActiveRole: return call.active;
    default: return QVariant();
    }
}

QHash<int, QByteArray> LastHeardModel::roleNames() const
{
    return {
        {SourceIdRole, "sourceId"},
        {SourceCallRole, "sourceCall"},
        {DestinationIdRole, "destinationId"},
        {PrivateRole, "isPrivate"},
        {OutgoingRole, "outgoing"},
        {TimeRole, "time"},
        {DurationRole, "duration"},
        {ActiveRole, "active"},
    };
}

void LastHeardModel::beginCall(const Call &call)
{
    endCall();
    beginInsertRows(QModelIndex(), 0, 0);
    m_calls.prepend(call);
    endInsertRows();
    if (m_calls.size() > kMaxCalls) {
        beginRemoveRows(QModelIndex(), kMaxCalls, m_calls.size() - 1);
        m_calls.resize(kMaxCalls);
        endRemoveRows();
    }
}

void LastHeardModel::updateActiveCall(const Call &call)
{
    if (m_calls.isEmpty() || !m_calls.first().active)
        return;
    m_calls.first() = call;
    const QModelIndex top = index(0);
    emit dataChanged(top, top);
}

void LastHeardModel::endCall()
{
    if (m_calls.isEmpty() || !m_calls.first().active)
        return;
    Call &call = m_calls.first();
    call.active = false;
    call.durationSecs = int(call.started.secsTo(QDateTime::currentDateTime()));
    const QModelIndex top = index(0);
    emit dataChanged(top, top, {DurationRole, ActiveRole});
}
