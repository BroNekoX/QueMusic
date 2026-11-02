// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#include "PlaybackHistory.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>

PlaybackHistory::PlaybackHistory(QObject *parent)
    : QAbstractListModel(parent)
{
}

int PlaybackHistory::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_entries.size());
}

QVariant PlaybackHistory::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return {};
    const HistoryEntry &entry = m_entries.at(index.row());
    switch (role) {
    case TitleRole:    return entry.title;
    case ArtistRole:   return entry.artist;
    case PathRole:     return entry.path;
    case SourceRole:   return entry.source;
    case CoverRole:    return entry.cover;
    case DurationRole: return entry.duration;
    case TimeRole:     return entry.time;
    default:           return {};
    }
}

QHash<int, QByteArray> PlaybackHistory::roleNames() const
{
    return {
        {TitleRole, "title"},
        {ArtistRole, "artist"},
        {PathRole, "path"},
        {SourceRole, "source"},
        {CoverRole, "cover"},
        {DurationRole, "duration"},
        {TimeRole, "time"}
    };
}

QVariantMap PlaybackHistory::get(int index) const
{
    if (index < 0 || index >= m_entries.size())
        return {};
    const HistoryEntry &entry = m_entries.at(index);
    return {
        {QStringLiteral("title"), entry.title},
        {QStringLiteral("artist"), entry.artist},
        {QStringLiteral("path"), entry.path},
        {QStringLiteral("source"), entry.source},
        {QStringLiteral("cover"), entry.cover},
        {QStringLiteral("duration"), entry.duration},
        {QStringLiteral("time"), entry.time}
    };
}

void PlaybackHistory::add(const HistoryEntry &entry)
{
    if (entry.path.isEmpty())
        return;
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries.at(i).path != entry.path)
            continue;
        if (i == 0) {
            m_entries[0] = entry;
            emit dataChanged(index(0), index(0));
            return;
        }
        beginRemoveRows(QModelIndex(), i, i);
        m_entries.removeAt(i);
        endRemoveRows();
        break;
    }
    beginInsertRows(QModelIndex(), 0, 0);
    m_entries.prepend(entry);
    endInsertRows();
    trim();
    emit countChanged();
}

void PlaybackHistory::remove(int index, int count)
{
    if (index < 0 || count <= 0 || index >= m_entries.size())
        return;
    count = qMin(count, int(m_entries.size()) - index);
    beginRemoveRows(QModelIndex(), index, index + count - 1);
    m_entries.remove(index, count);
    endRemoveRows();
    emit countChanged();
    save();
}

void PlaybackHistory::clear()
{
    if (m_entries.isEmpty())
        return;
    beginResetModel();
    m_entries.clear();
    endResetModel();
    emit countChanged();
    save();
}

void PlaybackHistory::load()
{
    QSettings settings;
    const QString json = settings.value(QStringLiteral("Options/playHistory")).toString();
    const QJsonDocument document = QJsonDocument::fromJson(json.toUtf8());
    const QJsonArray array = document.isArray() ? document.array() : QJsonArray();

    QVector<HistoryEntry> entries;
    entries.reserve(array.size());
    for (const QJsonValue &value : array) {
        const QJsonObject object = value.toObject();
        HistoryEntry entry;
        entry.title = object.value(QStringLiteral("title")).toString();
        entry.artist = object.value(QStringLiteral("artist")).toString();
        entry.path = object.value(QStringLiteral("path")).toString();
        entry.cover = object.value(QStringLiteral("cover")).toString();
        entry.source = object.value(QStringLiteral("source")).toInt(-1);
        entry.duration = object.value(QStringLiteral("duration")).toInt();
        entry.time = qint64(object.value(QStringLiteral("time")).toDouble());
        if (!entry.path.isEmpty())
            entries.append(entry);
    }

    if (entries.size() > m_limit)
        entries.resize(m_limit);
    beginResetModel();
    m_entries = entries;
    endResetModel();
    emit countChanged();
}

void PlaybackHistory::save()
{
    QJsonArray array;
    for (const HistoryEntry &entry : std::as_const(m_entries)) {
        array.append(QJsonObject{
            {QStringLiteral("title"), entry.title},
            {QStringLiteral("artist"), entry.artist},
            {QStringLiteral("path"), entry.path},
            {QStringLiteral("source"), entry.source},
            {QStringLiteral("cover"), entry.cover},
            {QStringLiteral("duration"), entry.duration},
            {QStringLiteral("time"), entry.time}
        });
    }
    QSettings settings;
    settings.setValue(QStringLiteral("Options/playHistory"),
                      QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact)));
    settings.sync();
}

void PlaybackHistory::trim()
{
    if (m_entries.size() <= m_limit)
        return;
    beginRemoveRows(QModelIndex(), m_limit, int(m_entries.size()) - 1);
    m_entries.resize(m_limit);
    endRemoveRows();
    emit countChanged();
}