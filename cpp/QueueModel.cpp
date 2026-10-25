// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#include "QueueModel.h"

#include "DbService.h"

#include <QDebug>
#include <QPointer>
#include <QSet>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {

QString sqlError(const QString &prefix, const QSqlError &error)
{
    return prefix + QStringLiteral(": ") + error.text();
}

} // namespace

// SavedPlaylistsModel

SavedPlaylistsModel::SavedPlaylistsModel(QObject *parent)
    : QAbstractListModel(parent)
{
    reload();
}

int SavedPlaylistsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

QVariant SavedPlaylistsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return {};
    const Item &item = m_items.at(index.row());
    switch (role) {
    case PlaylistIdRole:  return item.id;
    case NameRole:        return item.name;
    case SongCountRole:   return item.songCount;
    case CreatedAtRole:   return item.createdAt;
    }
    return {};
}

QHash<int, QByteArray> SavedPlaylistsModel::roleNames() const
{
    return {
        { PlaylistIdRole, "playlistId" },
        { NameRole, "name" },
        { SongCountRole, "songCount" },
        { CreatedAtRole, "createdAt" }
    };
}

void SavedPlaylistsModel::reload()
{
    const quint64 generation = m_generation.fetch_add(1) + 1;
    QPointer<SavedPlaylistsModel> self(this);
    DbService::instance()->submit([self, generation](QSqlDatabase &db) {
        QVector<Item> rows;
        QSqlQuery query(db);
        if (query.exec(QStringLiteral(
                "SELECT p.id, p.name, COUNT(s.playlist_id), p.created_at "
                "FROM playlists p "
                "LEFT JOIN playlist_songs s ON s.playlist_id = p.id "
                "GROUP BY p.id "
                "ORDER BY p.updated_at DESC, p.id DESC"))) {
            while (query.next()) {
                Item item;
                item.id = query.value(0).toInt();
                item.name = query.value(1).toString();
                item.songCount = query.value(2).toInt();
                item.createdAt = query.value(3).toDateTime();
                rows.append(item);
            }
        } else {
            qWarning() << "[playlist] 读取保存列表失败:" << query.lastError().text();
        }
        DbService::post(self, [self, generation, rows] {
            if (self)
                self->applyRows(generation, rows);
        });
    });
}

void SavedPlaylistsModel::applyRows(quint64 generation, const QVector<Item> &rows)
{
    if (generation != m_generation.load())
        return;
    const int previousCount = m_items.size();
    beginResetModel();
    m_items = rows;
    endResetModel();
    if (previousCount != rows.size())
        emit countChanged();
    emit loaded();
}

// PlaylistPreviewModel

PlaylistPreviewModel::PlaylistPreviewModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int PlaylistPreviewModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_songs.size();
}

QVariant PlaylistPreviewModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_songs.size())
        return {};
    const Song &song = m_songs.at(index.row());
    switch (role) {
    case NameRole:   return song.name;
    case PathRole:   return song.path;
    case SongerRole: return song.songer;
    case SourceRole: return song.source;
    }
    return {};
}

QHash<int, QByteArray> PlaylistPreviewModel::roleNames() const
{
    return {
        { NameRole, "name" },
        { PathRole, "path" },
        { SongerRole, "songer" },
        { SourceRole, "source" }
    };
}

void PlaylistPreviewModel::load(int playlistId)
{
    if (playlistId <= 0) {
        clear();
        return;
    }

    const quint64 generation = m_generation.fetch_add(1) + 1;
    if (!m_songs.isEmpty()) {
        beginResetModel();
        m_songs.clear();
        endResetModel();
        emit countChanged();
    }

    QPointer<PlaylistPreviewModel> self(this);
    DbService::instance()->submit([self, generation, playlistId](QSqlDatabase &db) {
        QString name;
        QVector<Song> songs;
        QSqlQuery meta(db);
        meta.prepare(QStringLiteral("SELECT name FROM playlists WHERE id = :id"));
        meta.bindValue(QStringLiteral(":id"), playlistId);
        if (meta.exec() && meta.next()) {
            name = meta.value(0).toString();
            QSqlQuery query(db);
            query.prepare(QStringLiteral(
                "SELECT name, path, songer, source FROM playlist_songs "
                "WHERE playlist_id = :id ORDER BY position ASC"));
            query.bindValue(QStringLiteral(":id"), playlistId);
            if (query.exec()) {
                while (query.next()) {
                    Song song;
                    song.name = query.value(0).toString();
                    song.path = query.value(1).toString();
                    song.songer = query.value(2).toString();
                    song.source = query.value(3).toInt();
                    songs.append(song);
                }
            } else {
                qWarning() << "[playlist] 读取预览失败:" << query.lastError().text();
            }
        }
        DbService::post(self, [self, generation, playlistId, name, songs] {
            if (self)
                self->applySongs(generation, playlistId, name, songs);
        });
    });
}

void PlaylistPreviewModel::clear()
{
    m_generation.fetch_add(1);
    if (!m_songs.isEmpty()) {
        beginResetModel();
        m_songs.clear();
        endResetModel();
        emit countChanged();
    }
    m_name.clear();
    m_playlistId = -1;
    emit loaded();
}

void PlaylistPreviewModel::applySongs(quint64 generation, int playlistId,
                                      const QString &name, const QVector<Song> &songs)
{
    if (generation != m_generation.load())
        return;
    beginResetModel();
    m_songs = songs;
    m_name = name;
    m_playlistId = playlistId;
    endResetModel();
    emit countChanged();
    emit loaded();
}

// QueueModel

QueueModel::QueueModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_savedPlaylists(new SavedPlaylistsModel(this))
    , m_preview(new PlaylistPreviewModel(this))
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

int QueueModel::appendBatch(const QVariantList &items, bool skipExisting)
{
    if (items.isEmpty())
        return 0;

    QVector<Track> tracks;
    tracks.reserve(items.size());
    QSet<QString> seen;
    for (const QVariant &entry : items) {
        const Track t = toTrack(entry.toMap());
        if (t.path.isEmpty())
            continue;
        if (skipExisting) {
            if (m_indexOfPath.contains(t.path) || seen.contains(t.path))
                continue;
            seen.insert(t.path);
        }
        tracks.append(t);
    }
    if (tracks.isEmpty())
        return 0;

    const int first = m_items.size();
    beginInsertRows(QModelIndex(), first, first + tracks.size() - 1);
    m_items += tracks;
    endInsertRows();
    rebuildIndex();
    emit countChanged();
    return tracks.size();
}

void QueueModel::insert(int index, const QVariantMap &item)
{
    index = qBound(0, index, int(m_items.size()));
    beginInsertRows(QModelIndex(), index, index);
    m_items.insert(index, toTrack(item));
    endInsertRows();
    rebuildIndex();
    if (index <= m_playListIndex) {
        m_playListIndex += 1;
        emit playListIndexChanged();
    }
    emit countChanged();
}

void QueueModel::moveTo(int from, int to)
{
    if (from < 0 || from >= m_items.size())
        return;
    const int last = int(m_items.size()) - 1;
    const int dest = qBound(0, to > from ? to - 1 : to, last);
    if (dest == from)
        return;
    if (!beginMoveRows(QModelIndex(), from, from, QModelIndex(), dest > from ? dest + 1 : dest))
        return;
    m_items.move(from, dest);
    endMoveRows();
    rebuildIndex();

    if (m_playListIndex >= 0) {
        int moved = m_playListIndex;
        if (moved == from)
            moved = dest;
        else if (from < moved && dest >= moved)
            moved -= 1;
        else if (from > moved && dest <= moved)
            moved += 1;
        if (moved != m_playListIndex) {
            m_playListIndex = moved;
            emit playListIndexChanged();
        }
    }
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
    if (m_playListIndex >= index) {
        m_playListIndex = qMax(0, qMin(m_playListIndex - count, int(m_items.size()) - 1));
        if (m_items.isEmpty())
            m_playListIndex = -1;
        emit playListIndexChanged();
    }
    emit countChanged();
}

void QueueModel::clear()
{
    if (m_items.isEmpty())
        return;
    beginResetModel();
    m_items.clear();
    m_indexOfPath.clear();
    endResetModel();
    if (m_playListIndex != -1) {
        m_playListIndex = -1;
        emit playListIndexChanged();
    }
    emit countChanged();
}

int QueueModel::indexOfPath(const QString &path) const
{
    return m_indexOfPath.value(path, -1);
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

void QueueModel::savePlaylist(const QString &name)
{
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty()) {
        emit playlistError(QStringLiteral("请输入播放列表名称"));
        return;
    }
    if (m_items.isEmpty()) {
        emit playlistError(QStringLiteral("当前播放列表为空"));
        return;
    }

    const QVector<Track> songs = m_items;
    const int count = songs.size();
    QPointer<QueueModel> self(this);
    DbService::instance()->submit([self, trimmed, songs, count](QSqlDatabase &db) {
        QString error;
        int id = -1;
        const bool inTransaction = db.transaction();

        if (!inTransaction) {
            DbService::markWriteFailed(db.lastError());
            error = sqlError(QStringLiteral("保存播放列表失败"), db.lastError());
        }

        if (error.isEmpty()) {
            QSqlQuery query(db);
            if (!query.prepare(QStringLiteral("INSERT INTO playlists (name) VALUES (:name)"))) {
                error = sqlError(QStringLiteral("保存播放列表失败"), query.lastError());
            } else {
                query.bindValue(QStringLiteral(":name"), trimmed);
                if (!query.exec()) {
                    DbService::markWriteFailed(query.lastError());
                    error = sqlError(QStringLiteral("保存播放列表失败"), query.lastError());
                } else {
                    id = int(query.lastInsertId().toLongLong());
                    if (id <= 0)
                        error = QStringLiteral("保存播放列表失败");
                }
            }
        }

        if (error.isEmpty()) {
            QSqlQuery query(db);
            if (!query.prepare(QStringLiteral(
                    "INSERT INTO playlist_songs "
                    "(playlist_id, position, name, path, songer, source) "
                    "VALUES (:playlist, :position, :name, :path, :songer, :source)"))) {
                error = sqlError(QStringLiteral("保存播放列表失败"), query.lastError());
            } else {
                for (int i = 0; i < songs.size(); ++i) {
                    const Track &track = songs.at(i);
                    query.bindValue(QStringLiteral(":playlist"), id);
                    query.bindValue(QStringLiteral(":position"), i);
                    query.bindValue(QStringLiteral(":name"), track.name);
                    query.bindValue(QStringLiteral(":path"), track.path);
                    query.bindValue(QStringLiteral(":songer"), track.songer);
                    query.bindValue(QStringLiteral(":source"), track.source);
                    if (!query.exec()) {
                        DbService::markWriteFailed(query.lastError());
                        error = sqlError(QStringLiteral("保存播放列表失败"), query.lastError());
                        break;
                    }
                }
            }
        }

        if (error.isEmpty() && !db.commit()) {
            DbService::markWriteFailed(db.lastError());
            error = sqlError(QStringLiteral("保存播放列表失败"), db.lastError());
        }
        if (!error.isEmpty() && inTransaction)
            db.rollback();

        DbService::post(self, [self, id, trimmed, count, error] {
            if (!self)
                return;
            if (!error.isEmpty()) {
                emit self->playlistError(error);
                return;
            }
            self->m_savedPlaylists->reload();
            emit self->playlistSaved(id, trimmed, count);
        });
    });
}

void QueueModel::loadPlaylist(int id)
{
    if (id <= 0) {
        emit playlistError(QStringLiteral("播放列表不存在"));
        return;
    }

    QPointer<QueueModel> self(this);
    DbService::instance()->submit([self, id](QSqlDatabase &db) {
        QString error;
        QString name;
        QVector<Track> songs;

        QSqlQuery meta(db);
        meta.prepare(QStringLiteral("SELECT name FROM playlists WHERE id = :id"));
        meta.bindValue(QStringLiteral(":id"), id);
        if (!meta.exec()) {
            error = sqlError(QStringLiteral("加载播放列表失败"), meta.lastError());
        } else if (!meta.next()) {
            error = QStringLiteral("播放列表不存在");
        } else {
            name = meta.value(0).toString();
            QSqlQuery query(db);
            query.prepare(QStringLiteral(
                "SELECT name, path, songer, source FROM playlist_songs "
                "WHERE playlist_id = :id ORDER BY position ASC"));
            query.bindValue(QStringLiteral(":id"), id);
            if (!query.exec()) {
                error = sqlError(QStringLiteral("加载播放列表失败"), query.lastError());
            } else {
                while (query.next()) {
                    Track track;
                    track.name = query.value(0).toString();
                    track.path = query.value(1).toString();
                    track.songer = query.value(2).toString();
                    track.source = query.value(3).toInt();
                    songs.append(track);
                }
            }
        }

        DbService::post(self, [self, id, name, songs, error] {
            if (!self)
                return;
            if (!error.isEmpty()) {
                emit self->playlistError(error);
                return;
            }
            self->beginResetModel();
            self->m_items = songs;
            self->rebuildIndex();
            self->endResetModel();
            if (self->m_playListIndex != -1) {
                self->m_playListIndex = -1;
                emit self->playListIndexChanged();
            }
            emit self->countChanged();
            emit self->playlistLoaded(id, name, songs.size());
        });
    });
}

void QueueModel::renamePlaylist(int id, const QString &name)
{
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty()) {
        emit playlistError(QStringLiteral("请输入播放列表名称"));
        return;
    }

    QPointer<QueueModel> self(this);
    DbService::instance()->submit([self, id, trimmed](QSqlDatabase &db) {
        QString error;
        QSqlQuery query(db);
        query.prepare(QStringLiteral(
            "UPDATE playlists SET name = :name, updated_at = CURRENT_TIMESTAMP "
            "WHERE id = :id"));
        query.bindValue(QStringLiteral(":name"), trimmed);
        query.bindValue(QStringLiteral(":id"), id);
        if (!query.exec()) {
            DbService::markWriteFailed(query.lastError());
            error = sqlError(QStringLiteral("重命名失败"), query.lastError());
        } else if (query.numRowsAffected() == 0) {
            error = QStringLiteral("播放列表不存在");
        }

        DbService::post(self, [self, id, trimmed, error] {
            if (!self)
                return;
            if (!error.isEmpty()) {
                emit self->playlistError(error);
                return;
            }
            self->m_savedPlaylists->reload();
            emit self->playlistRenamed(id, trimmed);
        });
    });
}

void QueueModel::deletePlaylist(int id)
{
    QPointer<QueueModel> self(this);
    DbService::instance()->submit([self, id](QSqlDatabase &db) {
        QString error;
        QSqlQuery query(db);
        query.prepare(QStringLiteral("DELETE FROM playlists WHERE id = :id"));
        query.bindValue(QStringLiteral(":id"), id);
        if (!query.exec()) {
            DbService::markWriteFailed(query.lastError());
            error = sqlError(QStringLiteral("删除失败"), query.lastError());
        } else if (query.numRowsAffected() == 0) {
            error = QStringLiteral("播放列表不存在");
        }

        DbService::post(self, [self, id, error] {
            if (!self)
                return;
            if (!error.isEmpty()) {
                emit self->playlistError(error);
                return;
            }
            self->m_savedPlaylists->reload();
            if (self->m_preview->playlistId() == id)
                self->m_preview->clear();
            emit self->playlistRemoved(id);
        });
    });
}

void QueueModel::previewPlaylist(int id)
{
    m_preview->load(id);
}
