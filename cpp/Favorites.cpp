// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#include "Favorites.h"

#include "DbService.h"
#include "PlayerDatabase.h"

#include <QDebug>
#include <QPointer>
#include <QSqlError>
#include <QSqlQuery>

#include <utility>

namespace {

FavoriteItem readFavorite(const QSqlQuery &query)
{
    FavoriteItem item;
    item.id = query.value(0).toString();
    item.title = query.value(1).toString();
    item.artist = query.value(2).toString();
    item.cover = query.value(3).toString();
    item.source = query.value(4).toInt();
    item.duration = query.value(5).toInt();
    item.type = query.value(6).toString();
    item.createdAt = query.value(7).toDateTime();
    return item;
}

const QString kFavoriteColumns =
    QStringLiteral("id, title, artist, cover, source, duration, type, created_at");

} // namespace

FavoritesModel::FavoritesModel(QObject *parent)
    : QAbstractListModel(parent)
{
    s_instances.append(this);
    refreshModel();
}

FavoritesModel::~FavoritesModel()
{
    s_instances.removeOne(this);
}

int FavoritesModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

QVariant FavoritesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_items.size())
        return {};

    const FavoriteItem &item = m_items.at(index.row());
    switch (role) {
    case IdRole:        return item.id;
    case TitleRole:     return item.title;
    case ArtistRole:    return item.artist;
    case CoverRole:     return item.cover;
    case SourceRole:    return item.source;
    case DurationRole:  return item.duration;
    case TypeRole:      return item.type;
    case CreatedAtRole: return item.createdAt;
    case PaytypeRole:   return 0;   // 收藏表不存付费类型，恒免费
    default:            return {};
    }
}

QHash<int, QByteArray> FavoritesModel::roleNames() const
{
    return {
        {IdRole,        "favId"},
        {TitleRole,     "title"},
        {ArtistRole,    "artist"},
        {CoverRole,     "cover"},
        {SourceRole,    "source"},
        {DurationRole,  "duration"},
        {TypeRole,      "type"},
        {CreatedAtRole, "createdAt"},
        {PaytypeRole,   "paytype"}
        };
}

QVariantMap FavoritesModel::get(int row) const
{
    if (row < 0 || row >= m_items.size())
        return {};

    const FavoriteItem &item = m_items.at(row);
    return {
        {QStringLiteral("id"), item.id},
        {QStringLiteral("title"), item.title},
        {QStringLiteral("artist"), item.artist},
        {QStringLiteral("cover"), item.cover},
        {QStringLiteral("source"), item.source},
        {QStringLiteral("duration"), item.duration},
        {QStringLiteral("type"), item.type},
        {QStringLiteral("createdAt"), item.createdAt},
        {QStringLiteral("paytype"), 0}
    };
}

void FavoritesModel::setFilterType(const QString &type)
{
    if (m_filterType == type)
        return;
    m_filterType = type;
    emit filterTypeChanged();
    refreshModel();
}

void FavoritesModel::setFolderId(int folderId)
{
    if (m_folderId == folderId)
        return;
    m_folderId = folderId;
    emit folderIdChanged();
    refreshModel();
}

void FavoritesModel::reloadAll()
{
    for (FavoritesModel *model : std::as_const(s_instances))
        model->refreshModel();
}

void FavoritesModel::refreshModel()
{
    setLoading(true);
    const int generation = m_generation.fetch_add(1) + 1;
    const QString filter = m_filterType;
    const int folder = m_folderId;
    QPointer<FavoritesModel> self(this);

    DbService::instance()->submit([self, filter, folder, generation](QSqlDatabase &db) {
        QVector<FavoriteItem> items;
        QSet<QString> ids;
        QSqlQuery query(db);
        QString where;
        if (!filter.isEmpty())
            where = QStringLiteral("type = :type");
        if (folder >= 0)
            where += where.isEmpty() ? QStringLiteral("folder_id = :folder")
                                     : QStringLiteral(" AND folder_id = :folder");
        QString sql = QStringLiteral("SELECT %1 FROM favorites").arg(kFavoriteColumns);
        if (!where.isEmpty())
            sql += QStringLiteral(" WHERE ") + where;
        sql += QStringLiteral(" ORDER BY created_at DESC");
        query.prepare(sql);
        if (!filter.isEmpty())
            query.bindValue(QStringLiteral(":type"), filter);
        if (folder >= 0)
            query.bindValue(QStringLiteral(":folder"), folder);
        if (query.exec()) {
            while (query.next()) {
                const FavoriteItem item = readFavorite(query);
                ids.insert(item.id);
                items.append(item);
            }
        }

        // 顺带把所有类型（song/playlist/artist）的 id 都取回来：isFavorite 在委托里逐行调用，
        // 不能在 GUI 线程查库（同一连接名被 DB 线程持有），只能靠这份内存答案
        const QHash<QString, QSet<QString>> allIds = [&db] {
            QHash<QString, QSet<QString>> out;
            QSqlQuery q(db);
            if (q.exec(QStringLiteral("SELECT type, id FROM favorites"))) {
                while (q.next())
                    out[q.value(0).toString()].insert(q.value(1).toString());
            }
            return out;
        }();

        DbService::post(self, [self, filter, generation, items, ids, allIds] {
            if (!self || generation != self->m_generation.load())
                return;
            self->m_idsByType = allIds;
            self->applyItems(items);
        });
    });
}

void FavoritesModel::applyItems(const QVector<FavoriteItem> &items)
{
    m_allItems = items;
    applyFilter();
    setLoading(false);
}

void FavoritesModel::setSearchText(const QString &text)
{
    const QString trimmed = text.trimmed();
    if (m_searchText == trimmed)
        return;
    m_searchText = trimmed;
    emit searchTextChanged();
    applyFilter();
}

// 过滤在内存完成：当前夹已全量在手，不产生任何 SQL
void FavoritesModel::applyFilter()
{
    QVector<FavoriteItem> filtered;
    if (m_searchText.isEmpty()) {
        filtered = m_allItems;
    } else {
        filtered.reserve(m_allItems.size());
        for (const FavoriteItem &item : std::as_const(m_allItems)) {
            if (item.title.contains(m_searchText, Qt::CaseInsensitive)
                || item.artist.contains(m_searchText, Qt::CaseInsensitive))
                filtered.append(item);
        }
    }
    beginResetModel();
    m_items = filtered;
    endResetModel();
    emit countChanged();
}

void FavoritesModel::setLoading(bool loading)
{
    if (m_loading == loading)
        return;
    m_loading = loading;
    emit loadingChanged();
}

// 三个收藏单例共用一张表：任一实例改动都让所有列表的收藏图标重算
void FavoritesModel::bumpRevision()
{
    ++s_revision;
    for (FavoritesModel *model : std::as_const(s_instances))
        emit model->favoriteStateChanged();
}

void FavoritesModel::addFavorite(const QString &id, const QString &title,
                                 const QString &artist, const QString &cover,
                                 int source, int duration, const QString &type)
{
    writeFavorite(id, title, artist, cover, source, duration, type, -1, false);
}

void FavoritesModel::addToFolder(const QString &id, const QString &title,
                                 const QString &artist, const QString &cover,
                                 int source, int duration, int folderId)
{
    writeFavorite(id, title, artist, cover, source, duration, QStringLiteral("song"),
                  folderId, true);
}

// 一条 UPSERT 取代「先查再写」；moveExisting=false 时新收藏进默认夹，已归属的保持原夹
void FavoritesModel::writeFavorite(const QString &id, const QString &title,
                                   const QString &artist, const QString &cover,
                                   int source, int duration, const QString &type,
                                   int folderId, bool moveExisting)
{
    if (id.isEmpty())
        return;

    QPointer<FavoritesModel> self(this);
    DbService::instance()->submit([self, id, title, artist, cover, source, duration,
                                   type, folderId, moveExisting](QSqlDatabase &db) {
        const bool song = (type == QLatin1String("song"));
        const QString folderValue = moveExisting
            ? QStringLiteral(":folder")
            : (song ? QStringLiteral("(SELECT id FROM folders WHERE type='fav' ORDER BY id ASC LIMIT 1)")
                    : QStringLiteral("NULL"));
        const QString folderUpdate = moveExisting
            ? QStringLiteral("folder_id = :folder")
            : QStringLiteral("folder_id = COALESCE(favorites.folder_id, excluded.folder_id)");
        QSqlQuery query(db);
        query.prepare(QStringLiteral(
            "INSERT INTO favorites (id, title, artist, cover, source, duration, type, folder_id) "
            "VALUES (:id, :title, :artist, :cover, :source, :duration, :type, %1) "
            "ON CONFLICT(type, id) DO UPDATE SET title = :title, artist = :artist, "
            "cover = :cover, source = :source, duration = :duration, %2").arg(folderValue, folderUpdate));
        query.bindValue(QStringLiteral(":id"), id);
        query.bindValue(QStringLiteral(":title"), title);
        query.bindValue(QStringLiteral(":artist"), artist);
        query.bindValue(QStringLiteral(":cover"), cover);
        query.bindValue(QStringLiteral(":source"), source);
        query.bindValue(QStringLiteral(":duration"), duration);
        query.bindValue(QStringLiteral(":type"), type);
        if (moveExisting)
            query.bindValue(QStringLiteral(":folder"), folderId);
        const bool ok = query.exec();
        if (!ok) {
            DbService::markWriteFailed(query.lastError());
            qWarning() << "[收藏] 写库失败 id=" << id << "type=" << type
                       << "原因=" << query.lastError().text() << "库=" << db.databaseName();
        }
        const QString error = ok ? QString() : QStringLiteral("收藏失败：") + query.lastError().text();

        DbService::post(self, [self, id, type, error] {
            if (!self)
                return;
            if (!error.isEmpty()) {
                // 不更新内存缓存：图标保持未收藏，用户看到的就是真实状态
                qWarning() << "[favorites]" << error;
                emit self->errorOccurred(error);
                return;
            }
            self->m_idsByType[type].insert(id);
            self->bumpRevision();
            if (self->m_filterType == type)
                self->refreshModel();
        });
    });
}

void FavoritesModel::removeFavorite(const QString &id, const QString &type)
{
    QPointer<FavoritesModel> self(this);
    DbService::instance()->submit([self, id, type](QSqlDatabase &db) {
        QSqlQuery query(db);
        query.prepare(QStringLiteral("DELETE FROM favorites WHERE type = :type AND id = :id"));
        query.bindValue(QStringLiteral(":type"), type);
        query.bindValue(QStringLiteral(":id"), id);
        const bool ok = query.exec();
        if (!ok) {
            DbService::markWriteFailed(query.lastError());
            qWarning() << "[收藏] 删库失败 id=" << id << "type=" << type
                       << "原因=" << query.lastError().text() << "库=" << db.databaseName();
        }
        const QString error = ok ? QString() : QStringLiteral("取消收藏失败：") + query.lastError().text();

        DbService::post(self, [self, id, type, error] {
            if (!self)
                return;
            if (!error.isEmpty()) {
                qWarning() << "[favorites]" << error;
                emit self->errorOccurred(error);
                return;
            }
            self->m_idsByType[type].remove(id);
            self->bumpRevision();
            if (self->m_filterType == type)
                self->refreshModel();
        });
    });
}

bool FavoritesModel::isFavorite(const QString &id, const QString &type) const
{
    // 只读内存答案：委托每行都会调用，绝不能在这里查库
    const auto it = m_idsByType.constFind(type);
    if (it == m_idsByType.cend()) {
        // 该类型还没装进内存（refreshModel 失败或被换了数据目录）：补一次后台加载，
        // 否则界面会一直显示成「未收藏」，而用户其实收藏过
        const_cast<FavoritesModel *>(this)->refreshModel();
        return false;
    }
    return it->contains(id);
}

// FavoriteFoldersModel
FavoriteFoldersModel::FavoriteFoldersModel(QObject *parent)
    : QAbstractListModel(parent)
{
    reload();
}

int FavoriteFoldersModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

QVariant FavoriteFoldersModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_items.size())
        return {};

    const FolderItem &item = m_items.at(index.row());
    switch (role) {
    case IdRole:        return item.id;
    case NameRole:      return item.name;
    case IsDefaultRole: return item.id == m_defaultId;
    default:            return {};
    }
}

QHash<int, QByteArray> FavoriteFoldersModel::roleNames() const
{
    return {
        {IdRole,        "folderId"},
        {NameRole,      "name"},
        {IsDefaultRole, "isDefault"}
    };
}

void FavoriteFoldersModel::reload()
{
    const quint64 generation = m_generation.fetch_add(1) + 1;
    QPointer<FavoriteFoldersModel> self(this);
    DbService::instance()->submit([self, generation](QSqlDatabase &db) {
        QVector<FolderItem> rows;
        QSqlQuery query(db);
        if (query.exec(QStringLiteral("SELECT id, name FROM folders WHERE type='fav' ORDER BY id ASC"))) {
            while (query.next()) {
                FolderItem item;
                item.id = query.value(0).toInt();
                item.name = query.value(1).toString();
                rows.append(item);
            }
        }
        DbService::post(self, [self, generation, rows] {
            if (self)
                self->applyRows(generation, rows);
        });
    });
}

void FavoriteFoldersModel::applyRows(quint64 generation, const QVector<FolderItem> &rows)
{
    if (generation != m_generation.load())
        return;

    const int def = rows.isEmpty() ? -1 : rows.first().id;
    const bool countChanged = m_items.size() != rows.size();
    beginResetModel();
    m_items = rows;
    endResetModel();
    if (countChanged)
        emit this->countChanged();
    if (def != m_defaultId) {
        m_defaultId = def;
        emit foldersChanged();
    }
}

void FavoriteFoldersModel::mutate(DbMutator mutator, bool reloadFavorites)
{
    QPointer<FavoriteFoldersModel> self(this);
    DbService::instance()->submit([self, mutator = std::move(mutator), reloadFavorites](QSqlDatabase &db) {
        QString error;
        mutator(db, error);
        DbService::post(self, [self, error, reloadFavorites] {
            if (!self)
                return;
            if (!error.isEmpty())
                emit self->errorOccurred(error);
            self->reload();
            if (reloadFavorites)   // 只有删夹会动到 favorites 行
                FavoritesModel::reloadAll();
        });
    });
}

void FavoriteFoldersModel::addFolder(const QString &name)
{
    mutate([name](QSqlDatabase &db, QString &error) {
        QSqlQuery query(db);
        query.prepare(QStringLiteral("INSERT INTO folders (name, type) VALUES (:name, 'fav')"));
        query.bindValue(QStringLiteral(":name"), name);
        if (!query.exec())
            error = QStringLiteral("新建收藏夹失败: ") + query.lastError().text();
    });
}

void FavoriteFoldersModel::renameFolder(int folderId, const QString &newName)
{
    mutate([folderId, newName](QSqlDatabase &db, QString &error) {
        QSqlQuery query(db);
        query.prepare(QStringLiteral("UPDATE folders SET name = :name WHERE id = :id AND type = 'fav'"));
        query.bindValue(QStringLiteral(":name"), newName);
        query.bindValue(QStringLiteral(":id"), folderId);
        if (!query.exec())
            error = QStringLiteral("重命名收藏夹失败: ") + query.lastError().text();
    });
}

void FavoriteFoldersModel::deleteFolder(int folderId)
{
    mutate([folderId](QSqlDatabase &db, QString &error) {
        db.transaction();
        QSqlQuery songs(db);
        songs.prepare(QStringLiteral("DELETE FROM favorites WHERE type = 'song' AND folder_id = :id"));
        songs.bindValue(QStringLiteral(":id"), folderId);
        songs.exec();
        QSqlQuery folder(db);
        folder.prepare(QStringLiteral("DELETE FROM folders WHERE id = :id AND type = 'fav'"));
        folder.bindValue(QStringLiteral(":id"), folderId);
        if (!folder.exec())
            error = QStringLiteral("删除收藏夹失败: ") + folder.lastError().text();
        db.commit();
    }, true);
}
