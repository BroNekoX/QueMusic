// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#include "OnlineListModel.h"

#include <algorithm>
#include <iterator>
#include <utility>

namespace {

// 预置常用字段，保证 roleNames 在模型第一次被 QML 使用前就稳定。
// 脚本音源返回的额外字段会在 append 时自动追加为新的 role。
const char *const kSeedColumns[] = {
    "title", "artist", "album", "cover", "hash", "hashhq", "hashsq",
    "duration", "source", "playcount", "paytype", "category", "id", "tagid",
    "url", "fileName", "author_name", "user", "avatar", "content", "liked",
    "replies", "time"
};

} // namespace

OnlineListModel::OnlineListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_columns.reserve(int(std::size(kSeedColumns)) + 8);
    for (const char *column : kSeedColumns)
        columnFor(QString::fromLatin1(column));
}

int OnlineListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_rows.size());
}

QVariant OnlineListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    const int column = role - int(Qt::UserRole);
    if (column < 0 || column >= m_columns.size())
        return {};

    const int row = index.row();
    if (row < 0 || row >= m_rows.size())
        return {};

    return m_rows.at(row).at(column);
}

QHash<int, QByteArray> OnlineListModel::roleNames() const
{
    return m_roleNames;
}

int OnlineListModel::columnFor(const QString &name)
{
    const auto it = m_columnIndex.constFind(name);
    if (it != m_columnIndex.constEnd())
        return it.value();

    // 防止恶意/异常插件返回无限多的随机字段把模型撑爆。
    constexpr int kMaxColumns = 64;
    if (m_columns.size() >= kMaxColumns)
        return -1;

    const int column = int(m_columns.size());
    m_columns.append(name);
    m_columnIndex.insert(name, column);
    m_roleNames.insert(int(Qt::UserRole) + column, name.toLatin1());

    // 新列需要把已有行补齐，保持所有行等长。
    for (QVector<QVariant> &row : m_rows)
        row.append(QVariant());
    return column;
}

void OnlineListModel::ensureColumns(const QVector<QVariantMap> &items)
{
    for (const QVariantMap &item : items) {
        for (auto it = item.constBegin(); it != item.constEnd(); ++it) {
            if (!m_columnIndex.contains(it.key()) && m_columns.size() < 64)
                columnFor(it.key());
        }
    }
}

QVector<QVariant> OnlineListModel::rowValues(const QVariantMap &item) const
{
    QVector<QVariant> row(m_columns.size());
    for (auto it = item.constBegin(); it != item.constEnd(); ++it) {
        const auto column = m_columnIndex.constFind(it.key());
        if (column != m_columnIndex.constEnd())
            row[column.value()] = it.value();
    }
    return row;
}

QVariantMap OnlineListModel::get(int index) const
{
    if (index < 0 || index >= m_rows.size())
        return {};

    const QVector<QVariant> &row = m_rows.at(index);
    QVariantMap item;
    for (int column = 0; column < row.size(); ++column) {
        if (row.at(column).isValid())
            item.insert(m_columns.at(column), row.at(column));
    }
    return item;
}

void OnlineListModel::clear()
{
    if (m_rows.isEmpty())
        return;
    beginResetModel();
    m_rows.clear();
    endResetModel();
    emit countChanged();
}

void OnlineListModel::append(const QVariant &items)
{
    QVariantList list;
    if (items.canConvert<QVariantMap>())
        list.append(items.toMap());
    else if (items.canConvert<QVariantList>())
        list = items.toList();
    appendItems(list);
}

void OnlineListModel::appendItems(const QVariantList &items)
{
    if (items.isEmpty())
        return;

    QVector<QVariantMap> maps;
    maps.reserve(items.size());
    for (const QVariant &value : items)
        maps.append(value.toMap());

    ensureColumns(maps);

    const int start = int(m_rows.size());
    beginInsertRows(QModelIndex(), start, start + int(maps.size()) - 1);
    m_rows.reserve(m_rows.size() + maps.size());
    for (const QVariantMap &item : std::as_const(maps))
        m_rows.append(rowValues(item));
    endInsertRows();
    emit countChanged();
}

void OnlineListModel::remove(int index, int count)
{
    if (count <= 0 || index < 0 || index >= m_rows.size())
        return;
    const int end = std::min(index + count, int(m_rows.size())) - 1;
    beginRemoveRows(QModelIndex(), index, end);
    m_rows.remove(index, end - index + 1);
    endRemoveRows();
    emit countChanged();
}
