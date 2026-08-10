// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#include "OnlineListModel.h"

#include <algorithm>

OnlineListModel::OnlineListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int OnlineListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_items.size();
}

QVariant OnlineListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return {};
    const QByteArray name = m_roleNames.value(role);
    if (name.isEmpty())
        return {};
    return m_items.at(index.row()).value(QString::fromLatin1(name));
}

QHash<int, QByteArray> OnlineListModel::roleNames() const
{
    return m_roleNames;
}

void OnlineListModel::collectRoles(const QVariantMap &map)
{
    for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
        const QByteArray key = it.key().toLatin1();
        if (!m_roleIndex.contains(key)) {
            const int role = m_nextRole++;
            m_roleIndex.insert(key, role);
            m_roleNames.insert(role, key);
        }
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

    // 先收集新字段 role（roleNames 变化须在 beginInsertRows 前完成）
    for (const QVariant &v : list)
        collectRoles(v.toMap());
    const int start = m_items.size();
    beginInsertRows(QModelIndex(), start, start + list.size() - 1);
    for (const QVariant &v : list)
        m_items.append(v.toMap());
    endInsertRows();
    emit countChanged();
}

void OnlineListModel::remove(int index, int count)
{
    if (index < 0 || index >= m_items.size())
        return;
    const int end = static_cast<int>(
        std::min<qsizetype>(index + count - 1, m_items.size() - 1));
    beginRemoveRows(QModelIndex(), index, end);
    for (int i = end; i >= index; --i)
        m_items.removeAt(i);
    endRemoveRows();
    emit countChanged();
}

void OnlineListModel::setItems(const QVariantList &items)
{
    beginResetModel();
    m_items.clear();
    for (const QVariant &v : items)
        collectRoles(v.toMap());
    for (const QVariant &v : items)
        m_items.append(v.toMap());
    endResetModel();
    emit countChanged();
}
