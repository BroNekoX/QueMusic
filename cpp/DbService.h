// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#ifndef DBSERVICE_H
#define DBSERVICE_H

#include <QMetaObject>
#include <QPointer>
#include <QSqlDatabase>
#include <QString>

#include <deque>
#include <functional>
#include <utility>

class QObject;
class QSqlError;

class DbService
{
public:
    using Job = std::function<void(QSqlDatabase &)>;

    static DbService *instance();

    void submit(Job job);

    static void markWriteFailed(const QSqlError &error);

    template <typename F>
    static void post(QObject *target, F &&fn)
    {
        if (!target)
            return;
        QMetaObject::invokeMethod(target, std::forward<F>(fn), Qt::QueuedConnection);
    }

    static QString dataDir();
    static bool dbWritable(const QString &dbPath);
    static void migrateFrom(const QString &oldDbPath, const QString &newDbPath);
    static QString cacheDir();
    static void ensureSchema(QSqlDatabase &db);

private:
    DbService();
    ~DbService();
    DbService(const DbService &) = delete;
    DbService &operator=(const DbService &) = delete;

    struct Impl;
    Impl *m_impl = nullptr;
};

#endif