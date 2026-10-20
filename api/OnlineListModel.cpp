// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#include "OnlineListModel.h"

#include <algorithm>

// 预注册角色：委托声明 required property 时，缺字段的行取默认值而不是整行不渲染
static const char *const kKnownRoles[] = {
    "title",      "artist",     "album",       "cover",      "hash",
    "duration",   "source",     "playcount",   "paytype",    "category",
    "id",         "tagid",      "url",         "fileName",   "hashhq",
    "hashsq",     "lyric",      "mvid",        "singer",     "singername",
    "songname",   "specialname", "specialid",  "album_name", "albumid",
    "imgurl",     "intro",      "username",    "author_name",
    "user",       "avatar",     "content",     "liked",
};

OnlineListModel::OnlineListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    QVariantMap seed;
    for (const char *role : kKnownRoles)
        seed.insert(QString::fromLatin1(role), QVariant());
    collectRoles(seed);
}

int OnlineListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_items.size());
}

QVariant OnlineListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return {};
    const auto key = m_roleKeys.constFind(role);
    if (key == m_roleKeys.constEnd())
        return {};
    return m_items.at(index.row()).value(key.value());
}

QHash<int, QByteArray> OnlineListModel::roleNames() const
{
    return m_roleNames;
}

void OnlineListModel::collectRoles(const QVariantMap &map)
{
    for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
        // 常见角色已注册：通常只做一次 QString 查找
        const QString &name = it.key();
        if (m_roleIndex.contains(name))
            continue;
        const QByteArray key = name.toLatin1();
        const int role = m_nextRole++;
        m_roleIndex.insert(name, role);
        m_roleNames.insert(role, key);
        m_roleKeys.insert(role, name);
    }
}

QVariantMap OnlineListModel::get(int index) const
{
    if (index < 0 || index >= m_items.size())
        return {};
    return m_items.at(index);
}

void OnlineListModel::clear()
{
    if (m_items.isEmpty())
        return;
    beginResetModel();
    m_items.clear();
    endResetModel();
    emit countChanged();
}

void OnlineListModel::append(const QVariant &items)
{
    QVariantList list;
    if (items.canConvert<QVariantMap>()) {
        list << items.toMap();
    } else if (items.canConvert<QVariantList>()) {
        list = items.toList();
    }
    if (list.isEmpty())
        return;

    // 每项只 toMap() 一次
    QVector<QVariantMap> rows;
    rows.reserve(list.size());
    for (const QVariant &v : list)
        rows.append(v.toMap());
    // 先收集新字段 role（roleNames 变化须在 beginInsertRows 前完成）
    for (const QVariantMap &row : std::as_const(rows))
        collectRoles(row);
    const int start = m_items.size();
    beginInsertRows(QModelIndex(), start, start + rows.size() - 1);
    for (const QVariantMap &row : std::as_const(rows))
        m_items.append(row);
    endInsertRows();
    emit countChanged();
}

void OnlineListModel::remove(int index, int count)
{
    if (count <= 0 || index < 0 || index >= m_items.size())
        return;
    const int end = std::min(index + count, int(m_items.size())) - 1;
    beginRemoveRows(QModelIndex(), index, end);
    m_items.remove(index, end - index + 1);
    endRemoveRows();
    emit countChanged();
}

void OnlineListModel::setItems(const QVariantList &items)
{
    QVector<QVariantMap> rows;
    rows.reserve(items.size());
    for (const QVariant &v : items)
        rows.append(v.toMap());
    beginResetModel();
    m_items.clear();
    m_items.reserve(rows.size());
    for (const QVariantMap &row : std::as_const(rows))
        collectRoles(row);
    for (const QVariantMap &row : std::as_const(rows))
        m_items.append(row);
    endResetModel();
    emit countChanged();
}
