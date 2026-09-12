// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#include "QueueModel.h"

// 继承QListModel自定义
QueueModel::QueueModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int QueueModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

QVariant QueueModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return {};
    const Track &t = m_items.at(index.row());
    switch (role) {
    case NameRole:   return t.name;
    case PathRole:   return t.path;
    case SongerRole: return t.songer;
    case SourceRole: return t.source;
    }
    return {};
}

QHash<int, QByteArray> QueueModel::roleNames() const
{
    return {
        { NameRole, "name" },
        { PathRole, "path" },
        { SongerRole, "songer" },
        { SourceRole, "source" }
    };
}

QueueModel::Track QueueModel::toTrack(const QVariantMap &item) const
{
    Track t;
    t.name = item.value(QStringLiteral("name")).toString();
    t.path = item.value(QStringLiteral("path")).toString();
    t.songer = item.value(QStringLiteral("songer")).toString();
    t.source = item.value(QStringLiteral("source"), 0).toInt();
    return t;
}

QVariantMap QueueModel::get(int index) const
{
    if (index < 0 || index >= m_items.size())
        return {};
    const Track &t = m_items.at(index);
    return {
        { QStringLiteral("name"), t.name },
        { QStringLiteral("path"), t.path },
        { QStringLiteral("songer"), t.songer },
        { QStringLiteral("source"), t.source }
    };
}

void QueueModel::append(const QVariantMap &item)
{
    insert(m_items.size(), item);
}

void QueueModel::insert(int index, const QVariantMap &item)
{
    index = qBound(0, index, int(m_items.size()));
    beginInsertRows(QModelIndex(), index, index);
    m_items.insert(index, toTrack(item));
    const Track &t = m_items.at(index);
    if (!t.path.isEmpty() && !m_indexOfPath.contains(t.path))
        m_indexOfPath.insert(t.path, index);
    endInsertRows();
    emit countChanged();
}

void QueueModel::remove(int index, int count)
{
    if (index < 0 || count <= 0 || index >= m_items.size())
        return;
    count = qMin(count, int(m_items.size() - index));
    beginRemoveRows(QModelIndex(), index, index + count - 1);
    m_items.remove(index, count);
    endRemoveRows();
    rebuildIndex();
    emit countChanged();
}

void QueueModel::move(int from, int to, int count)
{
    if (from < 0 || to < 0 || count <= 0 || from >= m_items.size())
        return;
    count = qMin(count, int(m_items.size() - from));
    if (to == from || to + count > m_items.size())
        return;
    const int dest = to > from ? to + count : to;
    if (!beginMoveRows(QModelIndex(), from, from + count - 1, QModelIndex(), dest))
        return;
    const QVector<Track> block = m_items.mid(from, count);
    m_items.remove(from, count);
    for (int i = 0; i < block.size(); ++i)
        m_items.insert(to + i, block.at(i));
    endMoveRows();
}

void QueueModel::clear()
{
    if (m_items.isEmpty())
        return;
    beginResetModel();
    m_items.clear();
    m_indexOfPath.clear();
    endResetModel();
    emit countChanged();
}

int QueueModel::indexOfPath(const QString &path) const
{
    return m_indexOfPath.value(path, -1);
}

int QueueModel::indexOfName(const QString &name) const
{
    for (int i = 0; i < m_items.size(); ++i)
        if (m_items.at(i).name == name)
            return i;
    return -1;
}

void QueueModel::rebuildIndex()
{
    m_indexOfPath.clear();
    for (int i = 0; i < m_items.size(); ++i) {
        const QString &p = m_items.at(i).path;
        if (!p.isEmpty() && !m_indexOfPath.contains(p))
            m_indexOfPath.insert(p, i);
    }
}

void QueueModel::setPlayListIndex(int index)
{
    const int size = m_items.size();
    const int clamped = size == 0 ? -1 : qBound(-1, index, size - 1);
    if (m_playListIndex == clamped)
        return;
    m_playListIndex = clamped;
    emit playListIndexChanged();
}
