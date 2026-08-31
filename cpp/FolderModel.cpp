// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#include "FolderModel.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDriver>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QMutexLocker>

static QSqlDatabase& sharedDatabase()
{
    static QSqlDatabase db = []() {
        QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(appDataDir);
        QString dbPath = appDataDir + "/player_data.db";

        // 连接到sql数据库
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "shared_player_db");
        db.setDatabaseName(dbPath);
        if (!db.open()) {
            qWarning() << "Failed to open database:" << db.lastError().text();
            return db;
        }

        QSqlQuery query(db);
        query.exec("PRAGMA foreign_keys = ON");
        query.exec(
            "CREATE TABLE IF NOT EXISTS folders ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "name TEXT NOT NULL, "
            "type TEXT NOT NULL DEFAULT 'my', "
            "path TEXT DEFAULT '', "
            "created_at DATETIME DEFAULT CURRENT_TIMESTAMP)"
            );
        query.exec(
            "CREATE TABLE IF NOT EXISTS songs ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "folder_id INTEGER NOT NULL, "
            "name TEXT NOT NULL, "
            "path TEXT NOT NULL, "
            "singer TEXT DEFAULT '', "
            "duration INTEGER DEFAULT 0, "
            "FOREIGN KEY (folder_id) REFERENCES folders(id) ON DELETE CASCADE)"
            );

        // 插入默认文件夹
        query.prepare("SELECT COUNT(*) FROM folders WHERE type='my' AND name='默认文件夹'");
        query.exec();
        if (query.next() && query.value(0).toInt() == 0) {
            query.prepare("INSERT INTO folders (name, type) VALUES (:name, :type)");
            query.bindValue(":name", "默认文件夹");
            query.bindValue(":type", "my");
            query.exec();
        }

        return db;
    }();
    return db;
}

// FolderModel 实现
FolderModel::FolderModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_db = sharedDatabase();   // 使用共享连接，不再创建新连接
    loadFromDatabase();
}


int FolderModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_items.size();
}

QVariant FolderModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_items.size())
        return {};

    const auto &item = m_items.at(index.row());
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
    refreshModel();
}

int FolderModel::addFolder(const QString &name, const QString &type, const QString &path)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO folders (name, type, path) VALUES (:name, :type, :path)");
    query.bindValue(":name", name);
    query.bindValue(":type", type);
    query.bindValue(":path", path);
    if (!query.exec()) {
        emit errorOccurred("添加文件夹失败: " + query.lastError().text());
        return -1;
    }
    int newId = query.lastInsertId().toInt();
    refreshModel();
    return newId;
}

bool FolderModel::deleteFolder(int folderId)
{
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM folders WHERE id = :id");
    query.bindValue(":id", folderId);
    if (!query.exec()) {
        emit errorOccurred("删除文件夹失败: " + query.lastError().text());
        return false;
    }
    refreshModel();
    return true;
}

bool FolderModel::renameFolder(int folderId, const QString &newName)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE folders SET name = :name WHERE id = :id");
    query.bindValue(":name", newName);
    query.bindValue(":id", folderId);
    if (!query.exec()) {
        emit errorOccurred("重命名失败: " + query.lastError().text());
        return false;
    }
    refreshModel();
    return true;
}

void FolderModel::setFilterType(const QString &type)
{
    if (m_filterType != type) {
        m_filterType = type;
        emit filterTypeChanged();
        refreshModel();
    }
}

void FolderModel::refreshModel()
{
    beginResetModel();
    m_items.clear();

    QSqlQuery query(m_db);
    query.prepare("SELECT id, name, type, path, created_at FROM folders WHERE type = :type ORDER BY created_at ASC");
    query.bindValue(":type", m_filterType);
    if (query.exec()) {
        while (query.next()) {
            FolderItem item;
            item.id = query.value(0).toInt();
            item.name = query.value(1).toString();
            item.type = query.value(2).toString();
            item.path = query.value(3).toString();
            item.createdAt = query.value(4).toDateTime();
            m_items.append(item);
        }
    }
    endResetModel();
}

// SongModel 构造函数
SongModel::SongModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_db = sharedDatabase();   // 使用同一个连接
}

int SongModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_items.size();
}

QVariant SongModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_items.size())
        return {};

    const auto &item = m_items.at(index.row());
    switch (role) {
    case IdRole:        return item.id;
    case FolderIdRole:  return item.folderId;
    case NameRole:      return item.name;
    case PathRole:      return item.path;
    case SingerRole:    return item.singer;
    case DurationRole:  return item.duration;
    default:            return {};
    }
}

QHash<int, QByteArray> SongModel::roleNames() const
{
    return {
        {IdRole,        "songId"},
        {FolderIdRole,  "folderId"},
        {NameRole,      "name"},
        {PathRole,      "path"},
        {SingerRole,    "singer"},
        {DurationRole,  "duration"}
    };
}

QVariantMap SongModel::get(int index) const
{
    if (index < 0 || index >= m_items.size())
        return {};
    const SongItem &item = m_items.at(index);
    QVariantMap map;
    map.insert(QStringLiteral("songId"), item.id);
    map.insert(QStringLiteral("folderId"), item.folderId);
    map.insert(QStringLiteral("name"), item.name);
    map.insert(QStringLiteral("path"), item.path);
    map.insert(QStringLiteral("singer"), item.singer);
    map.insert(QStringLiteral("duration"), item.duration);
    return map;
}

void SongModel::loadByFolder(int folderId)
{
    m_folderId = folderId;
    refreshModel();
}

int SongModel::addSong(int folderId, const QString &name, const QString &path, const QString &singer)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO songs (folder_id, name, path, singer) VALUES (:folder_id, :name, :path, :singer)");
    query.bindValue(":folder_id", folderId);
    query.bindValue(":name", name);
    query.bindValue(":path", path);
    query.bindValue(":singer", singer);
    if (!query.exec()) {
        emit errorOccurred("添加歌曲失败: " + query.lastError().text());
        return -1;
    }
    int newId = query.lastInsertId().toInt();
    if (folderId == m_folderId) {
        refreshModel();
    }
    return newId;
}

int SongModel::addSongs(int folderId, const QVariantList &songs)
{
    if (songs.isEmpty())
        return 0;

    // 批量导入时用单个事务承载全部 INSERT，最后统一刷新一次模型，
    // 避免逐条 addSong 带来的事务/刷新开销把 UI 卡成假死。
    bool ownTransaction = false;
    if (m_db.driver() && m_db.driver()->hasFeature(QSqlDriver::Transactions)) {
        ownTransaction = m_db.transaction();
    }

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO songs (folder_id, name, path, singer) VALUES (:folder_id, :name, :path, :singer)");
    int added = 0;
    for (const QVariant &entry : songs) {
        const QVariantMap song = entry.toMap();
        const QString name = song.value(QStringLiteral("name")).toString();
        const QString path = song.value(QStringLiteral("path")).toString();
        if (name.isEmpty() || path.isEmpty())
            continue;
        query.bindValue(":folder_id", folderId);
        query.bindValue(":name", name);
        query.bindValue(":path", path);
        query.bindValue(":singer", song.value(QStringLiteral("singer")).toString());
        if (query.exec()) {
            ++added;
        } else {
            emit errorOccurred(QStringLiteral("添加歌曲失败: ") + query.lastError().text());
        }
    }

    if (ownTransaction && !m_db.commit()) {
        m_db.rollback();
        emit errorOccurred(QStringLiteral("提交批量导入失败: ") + m_db.lastError().text());
        return 0;
    }

    if (folderId == m_folderId) {
        refreshModel();
    }
    return added;
}

bool SongModel::deleteSong(int songId)
{
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM songs WHERE id = :id");
    query.bindValue(":id", songId);
    if (!query.exec()) {
        emit errorOccurred("删除歌曲失败: " + query.lastError().text());
        return false;
    }
    refreshModel();
    return true;
}

void SongModel::setFolderId(int folderId)
{
    if (m_folderId != folderId) {
        m_folderId = folderId;
        emit folderIdChanged();
        refreshModel();
    }
}

void SongModel::refreshModel()
{
    beginResetModel();
    m_items.clear();

    if (m_folderId < 0) {
        endResetModel();
        return;
    }

    QSqlQuery query(m_db);
    query.prepare("SELECT id, folder_id, name, path, singer, duration FROM songs WHERE folder_id = :folder_id ORDER BY id ASC");
    query.bindValue(":folder_id", m_folderId);
    if (query.exec()) {
        while (query.next()) {
            SongItem item;
            item.id = query.value(0).toInt();
            item.folderId = query.value(1).toInt();
            item.name = query.value(2).toString();
            item.path = query.value(3).toString();
            item.singer = query.value(4).toString();
            item.duration = query.value(5).toInt();
            m_items.append(item);
        }
    }
    endResetModel();
}