// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#include "DbService.h"

#include <QDebug>
#include <QDir>
#include <QMutex>
#include <QMutexLocker>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QThread>

namespace {

const QString kConnName = QStringLiteral("quemusic_db_worker");

// 查列，缺了才 ALTER（老库升级无需用户干预）
void addColumnIfMissing(QSqlDatabase &db, const QString &table, const QString &column, const QString &definition)
{
    QSqlQuery info(db);
    if (!info.exec(QStringLiteral("PRAGMA table_info(%1)").arg(table)))
        return;
    while (info.next()) {
        if (info.value(1).toString().compare(column, Qt::CaseInsensitive) == 0)
            return;
    }
    QSqlQuery alter(db);
    if (!alter.exec(QStringLiteral("ALTER TABLE %1 ADD COLUMN %2 %3").arg(table, column, definition)))
        qWarning() << "add column failed:" << table << column << alter.lastError().text();
}

} // namespace

class DbWorker;

struct DbService::Impl {
    QThread thread;
    DbWorker *worker = nullptr;
};

// 只活在 DB 线程上的执行体
class DbWorker : public QObject
{
    Q_OBJECT
public:
    QMutex mutex;
    std::deque<DbService::Job> queue;
    QSqlDatabase db;

public slots:
    void open()
    {
        db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), kConnName);
        db.setDatabaseName(DbService::dataDir() + QStringLiteral("/player_data.db"));
        db.setConnectOptions(QStringLiteral("QSQLITE_BUSY_TIMEOUT=8000"));
        if (!db.open()) {
            qWarning() << "worker db open failed:" << db.lastError().text();
            return;
        }
        QSqlQuery pragma(db);
        // WAL：读写不互斥，与 GUI 线程的连接可以并存
        pragma.exec(QStringLiteral("PRAGMA journal_mode = WAL"));
        pragma.exec(QStringLiteral("PRAGMA synchronous = NORMAL"));
        pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON"));
        DbService::ensureSchema(db);
    }

    void drain()
    {
        for (;;) {
            DbService::Job job;
            {
                QMutexLocker lock(&mutex);
                if (queue.empty())
                    return;
                job = std::move(queue.front());
                queue.pop_front();
            }
            if (job)
                job(db);
        }
    }

    void close()
    {
        if (db.isOpen())
            db.close();
    }
};

DbService::DbService()
    : m_impl(new Impl)
{
    m_impl->worker = new DbWorker;
    m_impl->worker->moveToThread(&m_impl->thread);
    QObject::connect(&m_impl->thread, &QThread::started, m_impl->worker, &DbWorker::open);
    QObject::connect(&m_impl->thread, &QThread::finished, m_impl->worker, &DbWorker::close);
    m_impl->thread.setObjectName(QStringLiteral("quemusic-db"));
    m_impl->thread.start();
}

DbService::~DbService()
{
    m_impl->thread.quit();
    m_impl->thread.wait();
    delete m_impl->worker;
    delete m_impl;
}

DbService *DbService::instance()
{
    static DbService *service = new DbService;
    return service;
}

void DbService::submit(Job job)
{
    if (!job)
        return;
    {
        QMutexLocker lock(&m_impl->worker->mutex);
        m_impl->worker->queue.push_back(std::move(job));
    }
    QMetaObject::invokeMethod(m_impl->worker, "drain", Qt::QueuedConnection);
}

QString DbService::dataDir()
{
    static const QString dir = [] {
        const QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(path);
        return path;
    }();
    return dir;
}

QString DbService::cacheDir()
{
    static const QString dir = [] {
        const QString path = dataDir() + QStringLiteral("/cache");
        QDir().mkpath(path);
        return path;
    }();
    return dir;
}

void DbService::ensureSchema(QSqlDatabase &db)
{
    QSqlQuery query(db);
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
    query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS favorites ("
        "id TEXT NOT NULL,"
        "title TEXT NOT NULL,"
        "artist TEXT,"
        "cover TEXT,"
        "source INTEGER NOT NULL,"
        "duration INTEGER DEFAULT 0,"
        "type TEXT NOT NULL,"
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "PRIMARY KEY (type, id))"));

    // TAG 结果落库，避免每次刷新都重跑 TagLib
    addColumnIfMissing(db, QStringLiteral("songs"), QStringLiteral("tag_title"), QStringLiteral("TEXT DEFAULT ''"));
    addColumnIfMissing(db, QStringLiteral("songs"), QStringLiteral("tag_artist"), QStringLiteral("TEXT DEFAULT ''"));
    addColumnIfMissing(db, QStringLiteral("songs"), QStringLiteral("tag_cover"), QStringLiteral("TEXT DEFAULT ''"));
    addColumnIfMissing(db, QStringLiteral("songs"), QStringLiteral("tagged"), QStringLiteral("INTEGER DEFAULT 0"));
    query.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_songs_folder ON songs(folder_id)"));
    query.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_folders_type ON folders(type)"));
    query.exec(QStringLiteral("CREATE UNIQUE INDEX IF NOT EXISTS idx_songs_path ON songs(folder_id, path)"));

    // 首次运行时补一个默认歌单
    query.prepare(QStringLiteral("SELECT COUNT(*) FROM folders WHERE type='my' AND name='默认文件夹'"));
    if (query.exec() && query.next() && query.value(0).toInt() == 0) {
        query.prepare(QStringLiteral("INSERT INTO folders (name, type) VALUES (:name, :type)"));
        query.bindValue(QStringLiteral(":name"), QStringLiteral("默认文件夹"));
        query.bindValue(QStringLiteral(":type"), QStringLiteral("my"));
        query.exec();
    }
}

#include "DbService.moc"
