// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
#include "PlayerDatabase.h"

#include "DbService.h"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>

namespace {
const QString kConnectionName = QStringLiteral("shared_player_db");
} // namespace

QSqlDatabase playerDatabase()
{
    if (!QSqlDatabase::contains(kConnectionName)) {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), kConnectionName);
        db.setDatabaseName(DbService::dataDir() + QStringLiteral("/player_data.db"));
        db.setConnectOptions(QStringLiteral("QSQLITE_BUSY_TIMEOUT=8000"));
        if (!db.open())
            qWarning() << "Failed to open database:" << db.lastError().text();
        else
            DbService::ensureSchema(db);
        return db;
    }

    QSqlDatabase db = QSqlDatabase::database(kConnectionName);
    if (!db.isOpen())
        db.open();
    return db;
}
