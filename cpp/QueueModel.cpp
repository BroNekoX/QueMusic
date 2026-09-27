// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#include "QueueModel.h"

#include <QSet>

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

int QueueModel::appendBatch(const QVariantList &items)
{
    if (items.isEmpty())
        return 0;

    QVector<Track> tracks;
    tracks.reserve(items.size());
    QSet<QString> seen;
    for (const QVariant &entry : items) {
        const Track t = toTrack(entry.toMap());
        if (t.path.isEmpty() || m_indexOfPath.contains(t.path) || seen.contains(t.path))
            continue;
        seen.insert(t.path);
        tracks.append(t);
    }
    if (tracks.isEmpty())
        return 0;

    const int first = m_items.size();
    beginInsertRows(QModelIndex(), first, first + tracks.size() - 1);
    m_items += tracks;
    for (int i = 0; i < tracks.size(); ++i)
        m_indexOfPath.insert(tracks.at(i).path, first + i);
    endInsertRows();
    emit countChanged();
    return tracks.size();
}

void QueueModel::insert(int index, const QVariantMap &item)
{
    index = qBound(0, index, int(m_items.size()));
    beginInsertRows(QModelIndex(), index, index);
    m_items.insert(index, toTrack(item));
    endInsertRows();
    // 缓存永远与 m_items 行号一致：插入会让 index 及其后每一行的行号整体 +1，
    // 只记录新行会让后续行的缓存索引全部失效，所以统一在模型变更信号之后整体重建。
    // （rebuildIndex() 对重复 path 取首次出现的行，与 indexOfName() 的首次匹配语义一致。）
    rebuildIndex();
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
    // 与 insert()/move() 同一套策略：删除后 index 之后的行号整体前移，统一整体重建。
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
    // 缓存永远与 m_items 行号一致：前移与后移会改动 [from, ...] 到目标区间之间所有行的行号，
    // 分段增量修正很容易漏改，这里同样统一整体重建，保证每种修改操作只走一套索引维护策略。
    rebuildIndex();
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

// 唯一的不变量：缓存永远与 m_items 行号一致。
// 即 indexOfPath(p) 必须等于「m_items 中 path == p 的第一行」的行号；重复 path 取首次出现
// （与 indexOfName() 的首次匹配语义一致），空 path 不入缓存，查不到时返回 -1。
// 所有修改操作（insert/remove/move）都在模型变更信号之后整体重建，不做增量维护。
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
