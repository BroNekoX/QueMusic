// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#include "SearchResultModel.h"

SearchResultModel::SearchResultModel(const QList<int> &roles, QObject *parent)
    : QAbstractListModel(parent)
    , m_roles(roles)
{
}

QByteArray SearchResultModel::roleName(int role)
{
    switch (role) {
    case FileNameRole:     return "fileName";
    case FileUrlRole:      return "fileUrl";
    case FileSizeRole:     return "fileSize";
    case FileModifiedRole: return "fileModified";
    case TitleRole:        return "title";
    case ArtistRole:       return "artist";
    case CoverUrlRole:     return "coverUrl";
    case SongIdRole:       return "songId";
    case FolderIdRole:     return "folderId";
    case NameRole:         return "name";
    case PathRole:         return "path";
    case SingerRole:       return "singer";
    case DurationRole:     return "duration";
    case TagTitleRole:     return "tagTitle";
    case TagArtistRole:    return "tagArtist";
    case TagCoverUrlRole:  return "tagCoverUrl";
    default:               return QByteArray();
    }
}

int SearchResultModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_rows.size();
}

QVariant SearchResultModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
        return {};
    const Row &r = m_rows.at(index.row());
    switch (role) {
    case FileNameRole:     return r.name;
    case FileUrlRole:      return r.fileUrl;
    case FileSizeRole:     return r.size;
    case FileModifiedRole: return r.modified;
    case TitleRole:        return r.title;
    case ArtistRole:       return r.artist;
    case CoverUrlRole:     return r.coverUrl;
    case SongIdRole:       return r.songId;
    case FolderIdRole:     return r.folderId;
    case NameRole:         return r.name;
    case PathRole:         return r.path;
    case SingerRole:       return r.singer;
    case DurationRole:     return r.duration;
    case TagTitleRole:     return r.tagTitle;
    case TagArtistRole:    return r.tagArtist;
    case TagCoverUrlRole:  return r.tagCoverUrl;
    default:               return {};
    }
}

QHash<int, QByteArray> SearchResultModel::roleNames() const
{
    QHash<int, QByteArray> names;
    for (int role : m_roles)
        names.insert(role, roleName(role));
    return names;
}

QVariant SearchResultModel::get(int index, const QString &role) const
{
    if (index < 0 || index >= m_rows.size())
        return {};
    const QByteArray wanted = role.toUtf8();
    for (int r : m_roles) {
        if (roleName(r) == wanted)
            return data(this->index(index), r);
    }
    return {};
}

QVariantMap SearchResultModel::getRow(int index) const
{
    QVariantMap map;
    if (index < 0 || index >= m_rows.size())
        return map;
    const Row &r = m_rows.at(index);
    map.insert(QStringLiteral("name"), r.name);
    map.insert(QStringLiteral("path"), r.path);
    map.insert(QStringLiteral("fileUrl"), r.fileUrl);
    map.insert(QStringLiteral("fileSize"), r.size);
    map.insert(QStringLiteral("fileModified"), r.modified);
    map.insert(QStringLiteral("title"), r.title);
    map.insert(QStringLiteral("artist"), r.artist);
    map.insert(QStringLiteral("coverUrl"), r.coverUrl);
    map.insert(QStringLiteral("songId"), r.songId);
    map.insert(QStringLiteral("folderId"), r.folderId);
    map.insert(QStringLiteral("singer"), r.singer);
    map.insert(QStringLiteral("duration"), r.duration);
    map.insert(QStringLiteral("tagTitle"), r.tagTitle);
    map.insert(QStringLiteral("tagArtist"), r.tagArtist);
    map.insert(QStringLiteral("tagCoverUrl"), r.tagCoverUrl);
    return map;
}

void SearchResultModel::appendBatch(const QList<Row> &rows)
{
    if (rows.isEmpty())
        return;
    beginInsertRows(QModelIndex(), m_rows.size(), m_rows.size() + rows.size() - 1);
    for (const Row &r : rows)
        m_rows.append(r);
    endInsertRows();
    emit countChanged();
}

void SearchResultModel::clearRows()
{
    if (m_rows.isEmpty())
        return;
    beginResetModel();
    m_rows.clear();
    endResetModel();
    emit countChanged();
}
