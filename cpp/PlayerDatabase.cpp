// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
#include "PlayerDatabase.h"

#include <QDebug>
#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>

namespace {
const QString kConnectionName = QStringLiteral("shared_player_db");

// 基础表：歌单与歌曲。收藏表由 FavoritesModel 自己建（它还需要过滤类型等逻辑）。
void createCoreTables(QSqlDatabase &db)
{
    QSqlQuery query(db);
    query.exec(QStringLiteral("PRAGMA foreign_keys = ON"));
    query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS folders ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT NOT NULL, "
        "type TEXT NOT NULL DEFAULT 'my', "
        "path TEXT DEFAULT '', "
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP)"));
    query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS songs ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "folder_id INTEGER NOT NULL, "
        "name TEXT NOT NULL, "
        "path TEXT NOT NULL, "
        "singer TEXT DEFAULT '', "
        "duration INTEGER DEFAULT 0, "
        "FOREIGN KEY (folder_id) REFERENCES folders(id) ON DELETE CASCADE)"));

    // 首次运行时补一个默认歌单
    query.prepare(QStringLiteral("SELECT COUNT(*) FROM folders WHERE type='my' AND name='默认文件夹'"));
    query.exec();
    if (query.next() && query.value(0).toInt() == 0) {
        query.prepare(QStringLiteral("INSERT INTO folders (name, type) VALUES (:name, :type)"));
        query.bindValue(QStringLiteral(":name"), QStringLiteral("默认文件夹"));
        query.bindValue(QStringLiteral(":type"), QStringLiteral("my"));
        query.exec();
    }
}
} // namespace

QSqlDatabase playerDatabase()
{
    if (!QSqlDatabase::contains(kConnectionName)) {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), kConnectionName);
        const QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(appDataDir);
        db.setDatabaseName(appDataDir + QStringLiteral("/player_data.db"));
        db.setConnectOptions(QStringLiteral("QSQLITE_BUSY_TIMEOUT=5000"));
        if (!db.open())
            qWarning() << "Failed to open database:" << db.lastError().text();
        else
            createCoreTables(db);
        return db;
    }

    QSqlDatabase db = QSqlDatabase::database(kConnectionName);
    if (!db.isOpen())
        db.open();
    return db;
}
