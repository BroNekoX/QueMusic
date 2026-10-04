// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#include "FolderModel.h"

#include "CoverHelper.h"
#include "DbService.h"

#include <QFutureWatcher>
#include <QPointer>
#include <QSqlError>
#include <QSqlQuery>
#include <QtConcurrent/QtConcurrentRun>

#include <algorithm>

namespace {

// TAG 解析结果（工作线程 → GUI 线程）
struct EnrichResult {
    int row = -1;
    QString title;
    QString artist;
    QString coverUrl;
};

constexpr int kEnrichBatchSize = 64;

const QString kFolderColumns = QStringLiteral("id, name, type, path, created_at");
const QString kSongColumns =
    QStringLiteral("id, folder_id, name, path, singer, duration, tag_title, tag_artist, tag_cover, tagged");

SongItem readSong(const QSqlQuery &query)
{
    SongItem item;
    item.id = query.value(0).toInt();
    item.folderId = query.value(1).toInt();
    item.name = query.value(2).toString();
    item.path = query.value(3).toString();
    item.singer = query.value(4).toString();
    item.duration = query.value(5).toInt();
    item.tagTitle = query.value(6).toString();
    item.tagArtist = query.value(7).toString();
    item.tagCoverUrl = query.value(8).toString();
    item.tagged = query.value(9).toInt() != 0;
    return item;
}

// 转义 LIKE 通配符，避免用户输入的 % _ 被当成模式
QString likePattern(const QString &needle)
{
    QString escaped = needle;
    escaped.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    escaped.replace(QLatin1Char('%'), QStringLiteral("\\%"));
    escaped.replace(QLatin1Char('_'), QStringLiteral("\\_"));
    return QLatin1Char('%') + escaped + QLatin1Char('%');
}

SearchResultModel::Row toSearchRow(const SongItem &item)
{
    SearchResultModel::Row row;
    row.songId = item.id;
    row.folderId = item.folderId;
    row.name = item.name;
    row.path = item.path;
    row.singer = item.singer;
    row.duration = item.duration;
    row.tagTitle = item.tagTitle;
    row.tagArtist = item.tagArtist;
    row.tagCoverUrl = item.tagCoverUrl;
    return row;
}

} // namespace


FolderModel::FolderModel(QObject *parent)
    : QAbstractListModel(parent)
{
    loadFromDatabase();
}

int FolderModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

QVariant FolderModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_items.size())
        return {};

    const FolderItem &item = m_items.at(index.row());
    switch (role) {
    case IdRole:        return item.id;
    case NameRole:      return item.name;
    case TypeRole:      return item.type;
    case PathRole:      return item.path;
    case CreatedAtRole: return item.createdAt;
    default:            return {};
    }
}

QHash<int, QByteArray> FolderModel::roleNames() const
{
    return {
        {IdRole,        "folderId"},
        {NameRole,      "name"},
        {TypeRole,      "type"},
        {PathRole,      "path"},
        {CreatedAtRole, "createdAt"}
    };
}

void FolderModel::loadFromDatabase()
{
    const quint64 generation = m_generation.fetch_add(1) + 1;
    const QString type = m_filterType;
    QPointer<FolderModel> self(this);

    DbService::instance()->submit([self, type, generation](QSqlDatabase &db) {
        QVector<FolderItem> rows;
        QSqlQuery query(db);
        query.prepare(QStringLiteral("SELECT %1 FROM folders WHERE type = :type ORDER BY created_at ASC")
                          .arg(kFolderColumns));
        query.bindValue(QStringLiteral(":type"), type);
        if (query.exec()) {
            while (query.next()) {
                FolderItem item;
                item.id = query.value(0).toInt();
                item.name = query.value(1).toString();
                item.type = query.value(2).toString();
                item.path = query.value(3).toString();
                item.createdAt = query.value(4).toDateTime();
                rows.append(item);
            }
        }
        DbService::post(self, [self, generation, rows] {
            if (self)
                self->applyRows(generation, rows);
        });
    });
}

void FolderModel::applyRows(quint64 generation, const QVector<FolderItem> &rows)
{
    if (generation != m_generation.load())
        return;

    // 行集未变只发 dataChanged，避免整表 reset 重建视图缓存
    if (m_items.size() == rows.size()) {
        bool sameOrder = std::equal(rows.cbegin(), rows.cend(), m_items.cbegin(),
                                    [](const FolderItem &a, const FolderItem &b) { return a.id == b.id; });
        if (sameOrder) {
            m_items = rows;
            if (!m_items.isEmpty())
                emit dataChanged(index(0), index(m_items.size() - 1));
            return;
        }
    }

    beginResetModel();
    m_items = rows;
    endResetModel();
}

void FolderModel::mutate(DbMutator mutator)
{
    QPointer<FolderModel> self(this);
    DbService::instance()->submit([self, mutator = std::move(mutator)](QSqlDatabase &db) {
        QString error;
        mutator(db, error);
        DbService::post(self, [self, error] {
            if (!self)
                return;
            if (!error.isEmpty())
                emit self->errorOccurred(error);
            self->loadFromDatabase();
        });
    });
}

void FolderModel::addFolder(const QString &name, const QString &type, const QString &path)
{
    mutate([name, type, path](QSqlDatabase &db, QString &error) {
        QSqlQuery query(db);
        query.prepare(QStringLiteral("INSERT INTO folders (name, type, path) VALUES (:name, :type, :path)"));
        query.bindValue(QStringLiteral(":name"), name);
        query.bindValue(QStringLiteral(":type"), type);
        query.bindValue(QStringLiteral(":path"), path);
        if (!query.exec())
            error = QStringLiteral("添加文件夹失败: ") + query.lastError().text();
    });
}

void FolderModel::deleteFolder(int folderId)
{
    mutate([folderId](QSqlDatabase &db, QString &error) {
        QSqlQuery query(db);
        query.prepare(QStringLiteral("DELETE FROM folders WHERE id = :id"));
        query.bindValue(QStringLiteral(":id"), folderId);
        if (!query.exec())
            error = QStringLiteral("删除文件夹失败: ") + query.lastError().text();
    });
}

void FolderModel::deleteFolders(const QVariantList &folderIds)
{
    mutate([folderIds](QSqlDatabase &db, QString &error) {
        QSqlQuery query(db);
        query.prepare(QStringLiteral("DELETE FROM folders WHERE id = :id"));
        for (const QVariant &value : folderIds) {
            bool ok = false;
            const int id = value.toInt(&ok);
            if (!ok)
                continue;
            query.bindValue(QStringLiteral(":id"), id);
            if (!query.exec())
                error = QStringLiteral("删除文件夹失败: ") + query.lastError().text();
        }
    });
}

void FolderModel::renameFolder(int folderId, const QString &newName)
{
    mutate([folderId, newName](QSqlDatabase &db, QString &error) {
        QSqlQuery query(db);
        query.prepare(QStringLiteral("UPDATE folders SET name = :name WHERE id = :id"));
        query.bindValue(QStringLiteral(":name"), newName);
        query.bindValue(QStringLiteral(":id"), folderId);
        if (!query.exec())
            error = QStringLiteral("重命名失败: ") + query.lastError().text();
    });
}

void FolderModel::setFilterType(const QString &type)
{
    if (m_filterType == type)
        return;
    m_filterType = type;
    emit filterTypeChanged();
    loadFromDatabase();
}

// SongModel
SongModel::SongModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_searchResults = new SearchResultModel({SearchResultModel::SongIdRole,
                                             SearchResultModel::FolderIdRole,
                                             SearchResultModel::NameRole,
                                             SearchResultModel::PathRole,
                                             SearchResultModel::SingerRole,
                                             SearchResultModel::DurationRole,
                                             SearchResultModel::TagTitleRole,
                                             SearchResultModel::TagArtistRole,
                                             SearchResultModel::TagCoverUrlRole}, this);
}

SongModel::~SongModel() = default;

int SongModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

QVariant SongModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_items.size())
        return {};

    const SongItem &item = m_items.at(index.row());
    switch (role) {
    case IdRole:          return item.id;
    case FolderIdRole:    return item.folderId;
    case NameRole:        return item.name;
    case PathRole:        return item.path;
    case SingerRole:      return item.singer;
    case DurationRole:    return item.duration;
    case TagTitleRole:    return item.tagTitle;
    case TagArtistRole:   return item.tagArtist;
    case TagCoverUrlRole: return item.tagCoverUrl;
    default:              return {};
    }
}

QHash<int, QByteArray> SongModel::roleNames() const
{
    return {
        {IdRole,          "songId"},
        {FolderIdRole,    "folderId"},
        {NameRole,        "name"},
        {PathRole,        "path"},
        {SingerRole,      "singer"},
        {DurationRole,    "duration"},
        {TagTitleRole,    "tagTitle"},
        {TagArtistRole,   "tagArtist"},
        {TagCoverUrlRole, "tagCoverUrl"}
    };
}

QVariantMap SongModel::get(int index) const
{
    if (index < 0 || index >= m_items.size())
        return {};
    const SongItem &item = m_items.at(index);
    return {
        {QStringLiteral("songId"), item.id},
        {QStringLiteral("folderId"), item.folderId},
        {QStringLiteral("name"), item.name},
        {QStringLiteral("path"), item.path},
        {QStringLiteral("singer"), item.singer},
        {QStringLiteral("duration"), item.duration},
        {QStringLiteral("tagTitle"), item.tagTitle},
        {QStringLiteral("tagArtist"), item.tagArtist},
        {QStringLiteral("tagCoverUrl"), item.tagCoverUrl}
    };
}

void SongModel::loadByFolder(int folderId)
{
    m_folderId = folderId;
    refreshModel();
}

void SongModel::setFolderId(int folderId)
{
    if (m_folderId == folderId)
        return;
    m_folderId = folderId;
    emit folderIdChanged();
    refreshModel();
}

void SongModel::refreshModel()
{
    clearSearch();
    m_enriching = false;
    m_enrichPending.clear();

    const quint64 generation = m_loadGeneration.fetch_add(1) + 1;
    const int folderId = m_folderId;

    if (folderId < 0) {
        applyRows(generation, {});
        return;
    }

    QPointer<SongModel> self(this);
    DbService::instance()->submit([self, folderId, generation](QSqlDatabase &db) {
        QVector<SongItem> rows;
        QSqlQuery query(db);
        query.prepare(QStringLiteral("SELECT %1 FROM songs WHERE folder_id = :folder ORDER BY id ASC")
                          .arg(kSongColumns));
        query.bindValue(QStringLiteral(":folder"), folderId);
        if (query.exec()) {
            while (query.next())
                rows.append(readSong(query));
        }
        DbService::post(self, [self, generation, rows] {
            if (self)
                self->applyRows(generation, rows);
        });
    });
}

void SongModel::applyRows(quint64 generation, const QVector<SongItem> &rows)
{
    if (generation != m_loadGeneration.load())
        return;

    // 行集一致则原地更新，保留 ListView 缓存
    if (m_items.size() == rows.size()) {
        bool sameOrder = std::equal(rows.cbegin(), rows.cend(), m_items.cbegin(),
                                    [](const SongItem &a, const SongItem &b) { return a.id == b.id; });
        if (sameOrder) {
            m_items = rows;
            if (!m_items.isEmpty())
                emit dataChanged(index(0), index(m_items.size() - 1));
            startEnrichment();
            return;
        }
    }

    beginResetModel();
    m_items = rows;
    endResetModel();
    startEnrichment();
}

void SongModel::startEnrichment()
{
    if (m_enriching)
        return;

    m_enrichPending.clear();
    for (int i = 0; i < m_items.size(); ++i) {
        if (!m_items.at(i).tagged)
            m_enrichPending.append(i);
    }
    if (m_enrichPending.isEmpty())
        return;

    m_enriching = true;
    enrichBatch();
}

void SongModel::enrichBatch()
{
    if (m_enrichPending.isEmpty()) {
        m_enriching = false;
        return;
    }

    const quint64 generation = m_loadGeneration.load();
    const int count = std::min(kEnrichBatchSize, int(m_enrichPending.size()));
    const QList<int> slice = m_enrichPending.mid(0, count);
    m_enrichPending.remove(0, count);

    QVector<QString> paths;
    paths.reserve(slice.size());
    for (int row : slice)
        paths.append(m_items.at(row).path);

    auto *watcher = new QFutureWatcher<QList<EnrichResult>>(this);
    connect(watcher, &QFutureWatcher<QList<EnrichResult>>::finished, this, [this, watcher, generation, slice] {
        watcher->deleteLater();
        if (generation != m_loadGeneration.load()) {
            m_enriching = false;
            m_enrichPending.clear();
            return;
        }

        const QVector<int> roles = {TagTitleRole, TagArtistRole, TagCoverUrlRole};
        QList<int> ids;
        QStringList titles;
        QStringList artists;
        QStringList covers;
        for (const EnrichResult &result : watcher->result()) {
            if (result.row < 0 || result.row >= m_items.size())
                continue;
            SongItem &item = m_items[result.row];
            item.tagTitle = result.title;
            item.tagArtist = result.artist;
            item.tagCoverUrl = result.coverUrl;
            item.tagged = true;
            ids.append(item.id);
            titles.append(result.title);
            artists.append(result.artist);
            covers.append(result.coverUrl);
            emit dataChanged(index(result.row), index(result.row), roles);
        }
        persistTags(ids, titles, artists, covers);
        enrichBatch();
    });

    watcher->setFuture(QtConcurrent::run([slice, paths] {
        QList<EnrichResult> out;
        out.reserve(slice.size());
        const QString cacheDir = DbService::cacheDir();
        for (int i = 0; i < slice.size(); ++i) {
            EnrichResult result;
            result.row = slice.at(i);
            CoverHelper::Metadata meta;
            result.coverUrl = CoverHelper::readCoverFromTag(paths.at(i), cacheDir, &meta,
                                                            CoverHelper::kThumbSize);
            result.title = meta.title;
            result.artist = meta.artist;
            out.append(result);
        }
        return out;
    }));
}

void SongModel::persistTags(const QList<int> &ids, const QStringList &titles,
                            const QStringList &artists, const QStringList &covers)
{
    if (ids.isEmpty())
        return;

    DbService::instance()->submit([ids, titles, artists, covers](QSqlDatabase &db) {
        QSqlQuery query(db);
        query.prepare(QStringLiteral(
            "UPDATE songs SET tag_title = :title, tag_artist = :artist, tag_cover = :cover, tagged = 1 "
            "WHERE id = :id"));
        for (int i = 0; i < ids.size(); ++i) {
            query.bindValue(QStringLiteral(":title"), titles.value(i));
            query.bindValue(QStringLiteral(":artist"), artists.value(i));
            query.bindValue(QStringLiteral(":cover"), covers.value(i));
            query.bindValue(QStringLiteral(":id"), ids.at(i));
            query.exec();
        }
    });
}

void SongModel::addSong(int folderId, const QString &name, const QString &path, const QString &singer)
{
    mutate([folderId, name, path, singer](QSqlDatabase &db, QString &error) {
        QSqlQuery query(db);
        query.prepare(QStringLiteral(
            "INSERT OR IGNORE INTO songs (folder_id, name, path, singer) VALUES (:folder, :name, :path, :singer)"));
        query.bindValue(QStringLiteral(":folder"), folderId);
        query.bindValue(QStringLiteral(":name"), name);
        query.bindValue(QStringLiteral(":path"), path);
        query.bindValue(QStringLiteral(":singer"), singer);
        if (!query.exec())
            error = QStringLiteral("添加歌曲失败: ") + query.lastError().text();
    });
}

void SongModel::addSongs(int folderId, const QVariantList &songs)
{
    QPointer<SongModel> self(this);
    DbService::instance()->submit([self, folderId, songs](QSqlDatabase &db) {
        QSqlQuery query(db);
        query.prepare(QStringLiteral(
            "INSERT OR IGNORE INTO songs (folder_id, name, path, singer) VALUES (:folder, :name, :path, :singer)"));
        db.transaction();
        int added = 0;
        QString error;
        for (const QVariant &entry : songs) {
            const QVariantMap song = entry.toMap();
            const QString name = song.value(QStringLiteral("name")).toString();
            const QString path = song.value(QStringLiteral("path")).toString();
            if (name.isEmpty() || path.isEmpty())
                continue;
            query.bindValue(QStringLiteral(":folder"), folderId);
            query.bindValue(QStringLiteral(":name"), name);
            query.bindValue(QStringLiteral(":path"), path);
            query.bindValue(QStringLiteral(":singer"), song.value(QStringLiteral("singer")).toString());
            if (query.exec()) {
                if (query.numRowsAffected() > 0)
                    ++added;
            } else {
                error = QStringLiteral("添加歌曲失败: ") + query.lastError().text();
            }
        }
        db.commit();

        DbService::post(self, [self, folderId, added, error] {
            if (!self)
                return;
            if (!error.isEmpty())
                emit self->errorOccurred(error);
            emit self->songsAdded(added);
            if (folderId == self->m_folderId)
                self->refreshModel();
        });
    });
}

void SongModel::deleteSong(int songId)
{
    mutate([songId](QSqlDatabase &db, QString &error) {
        QSqlQuery query(db);
        query.prepare(QStringLiteral("DELETE FROM songs WHERE id = :id"));
        query.bindValue(QStringLiteral(":id"), songId);
        if (!query.exec())
            error = QStringLiteral("删除歌曲失败: ") + query.lastError().text();
    });
}

void SongModel::deleteSongs(const QVariantList &songIds)
{
    mutate([songIds](QSqlDatabase &db, QString &error) {
        QSqlQuery query(db);
        query.prepare(QStringLiteral("DELETE FROM songs WHERE id = :id"));
        for (const QVariant &value : songIds) {
            bool ok = false;
            const int id = value.toInt(&ok);
            if (!ok)
                continue;
            query.bindValue(QStringLiteral(":id"), id);
            if (!query.exec())
                error = QStringLiteral("删除歌曲失败: ") + query.lastError().text();
        }
    });
}

void SongModel::rescanTags(const QString &path)
{
    const int folderId = m_folderId;
    mutate([path, folderId](QSqlDatabase &db, QString &error) {
        QSqlQuery query(db);
        if (path.isEmpty()) {
            query.prepare(QStringLiteral("UPDATE songs SET tagged = 0 WHERE folder_id = :folder"));
            query.bindValue(QStringLiteral(":folder"), folderId);
        } else {
            query.prepare(QStringLiteral("UPDATE songs SET tagged = 0 WHERE path = :path"));
            query.bindValue(QStringLiteral(":path"), path);
        }
        if (!query.exec())
            error = QStringLiteral("重置标签缓存失败: ") + query.lastError().text();
    });
}

void SongModel::mutate(DbMutator mutator)
{
    QPointer<SongModel> self(this);
    const int folderId = m_folderId;
    DbService::instance()->submit([self, folderId, mutator = std::move(mutator)](QSqlDatabase &db) {
        QString error;
        mutator(db, error);
        DbService::post(self, [self, folderId, error] {
            if (!self)
                return;
            if (!error.isEmpty())
                emit self->errorOccurred(error);
            if (folderId == self->m_folderId)
                self->refreshModel();
        });
    });
}

void SongModel::startSearch(const QString &text)
{
    m_searchText = text.trimmed();
    m_searchResults->clearRows();

    if (m_searchText.isEmpty() || m_folderId < 0) {
        m_searchGeneration.fetch_add(1);
        if (m_searchActive) {
            m_searchActive = false;
            emit searchActiveChanged();
        }
        emit searchFinished(0);
        return;
    }

    if (!m_searchActive) {
        m_searchActive = true;
        emit searchActiveChanged();
    }
    submitSearch();
}

void SongModel::clearSearch()
{
    m_searchText.clear();
    m_searchGeneration.fetch_add(1);
    m_searchResults->clearRows();
    if (m_searchActive) {
        m_searchActive = false;
        emit searchActiveChanged();
    }
}

void SongModel::submitSearch()
{
    if (m_folderId < 0)
        return;

    const quint64 generation = m_searchGeneration.fetch_add(1) + 1;
    const int folderId = m_folderId;
    const QString pattern = likePattern(m_searchText);
    QPointer<SongModel> self(this);

    // 匹配交给 SQL，不占 GUI 线程
    DbService::instance()->submit([self, folderId, pattern, generation](QSqlDatabase &db) {
        QList<SearchResultModel::Row> rows;
        QSqlQuery query(db);
        query.prepare(QStringLiteral(
            "SELECT %1 FROM songs WHERE folder_id = :folder AND ("
            "name LIKE :p1 ESCAPE '\\' OR singer LIKE :p2 ESCAPE '\\' "
            "OR tag_title LIKE :p3 ESCAPE '\\' OR tag_artist LIKE :p4 ESCAPE '\\') ORDER BY id ASC")
                          .arg(kSongColumns));
        query.bindValue(QStringLiteral(":folder"), folderId);
        query.bindValue(QStringLiteral(":p1"), pattern);
        query.bindValue(QStringLiteral(":p2"), pattern);
        query.bindValue(QStringLiteral(":p3"), pattern);
        query.bindValue(QStringLiteral(":p4"), pattern);
        if (query.exec()) {
            while (query.next())
                rows.append(toSearchRow(readSong(query)));
        }
        DbService::post(self, [self, generation, rows] {
            if (!self || generation != self->m_searchGeneration.load())
                return;
            self->m_searchResults->appendBatch(rows);
            emit self->searchFinished(rows.size());
        });
    });
}
