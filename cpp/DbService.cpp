// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#include "DbService.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QThread>

#include <atomic>

namespace {

const QString kConnName = QStringLiteral("quemusic_db_worker");

void addColumnIfMissing(QSqlDatabase &db, const QString &table, const QString &column,
                        const QString &definition)
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

}

class DbWorker;

struct DbService::Impl {
    QThread thread;
    DbWorker *worker = nullptr;
};

class DbWorker : public QObject
{
    Q_OBJECT
public:
    QMutex mutex;
    std::deque<DbService::Job> queue;
    QSqlDatabase db;
    bool drainScheduled = false;
    std::atomic_bool writeFailed{false};

public slots:
    void open()
    {
        if (QSqlDatabase::contains(kConnName))
            db = QSqlDatabase::database(kConnName, true);
        else
            db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), kConnName);
        db.setDatabaseName(DbService::dataDir() + QStringLiteral("/player_data.db"));
        db.setConnectOptions(QStringLiteral("QSQLITE_BUSY_TIMEOUT=8000"));
        if (!db.open()) {
            qWarning() << "[db] 打开失败:" << db.lastError().text();
            return;
        }

        QSqlQuery pragma(db);
        pragma.exec(QStringLiteral("PRAGMA journal_mode = DELETE"));
        pragma.exec(QStringLiteral("PRAGMA synchronous = NORMAL"));
        pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON"));
        DbService::ensureSchema(db);

        QSqlQuery probe(db);
        if (probe.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS db_probe (x INTEGER)"))) {
            probe.exec(QStringLiteral("DROP TABLE db_probe"));
        } else {
            qWarning() << "[db] 数据库不可写:" << probe.lastError().text()
                       << "路径:" << db.databaseName();
        }
    }

    void reopen()
    {
        const QString name = db.connectionName();
        db.close();
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(name);
        open();
    }

    void drain()
    {
        for (;;) {
            DbService::Job job;
            {
                QMutexLocker lock(&mutex);
                if (queue.empty()) {
                    drainScheduled = false;
                    return;
                }
                job = std::move(queue.front());
                queue.pop_front();
            }
            if (!job)
                continue;
            job(db);
            if (writeFailed.exchange(false)) {
                qWarning() << "[db] 写失败，连接已重开，任务重试一次";
                reopen();
                job(db);
            }
        }
    }

    void markWriteFailed(const QSqlError &error)
    {
        const QString text = error.databaseText() + error.driverText();
        if (error.nativeErrorCode() == QLatin1String("8")
            || text.contains(QLatin1String("readonly"), Qt::CaseInsensitive)
            || text.contains(QLatin1String("read-only"), Qt::CaseInsensitive)) {
            writeFailed.store(true, std::memory_order_relaxed);
        }
    }

    void close()
    {
        if (db.isOpen())
            db.close();
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(kConnName);
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

    bool schedule = false;
    {
        QMutexLocker lock(&m_impl->worker->mutex);
        m_impl->worker->queue.push_back(std::move(job));
        if (!m_impl->worker->drainScheduled) {
            m_impl->worker->drainScheduled = true;
            schedule = true;
        }
    }
    if (schedule)
        QMetaObject::invokeMethod(m_impl->worker, "drain", Qt::QueuedConnection);
}

void DbService::markWriteFailed(const QSqlError &error)
{
    instance()->m_impl->worker->markWriteFailed(error);
}

QString DbService::dataDir()
{
    static const QString dir = [] {
        QStringList candidates;
        const QString preferred = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        candidates << preferred;
        candidates << QCoreApplication::applicationDirPath() + QStringLiteral("/data");

        QString chosen;
        for (const QString &candidate : std::as_const(candidates)) {
            if (candidate.isEmpty() || !QDir().mkpath(candidate))
                continue;
            if (dbWritable(candidate + QStringLiteral("/player_data.db"))) {
                chosen = candidate;
                break;
            }
            qWarning() << "[db] 该目录无法写库:" << candidate;
        }
        if (chosen.isEmpty())
            chosen = preferred;

        const QString target = chosen + QStringLiteral("/player_data.db");
        if (chosen != preferred && !QFile::exists(target))
            migrateFrom(preferred + QStringLiteral("/player_data.db"), target);

        qInfo() << "[db] 数据目录:" << chosen;
        return chosen;
    }();
    return dir;
}

bool DbService::dbWritable(const QString &dbPath)
{
    const QString name = QStringLiteral("writable_probe");
    bool ok = false;
    {
        QSqlDatabase probe = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), name);
        probe.setDatabaseName(dbPath);
        if (probe.open()) {
            QSqlQuery q(probe);
            ok = q.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS w (x INTEGER)"))
                 && q.exec(QStringLiteral("DROP TABLE w"));
        }
        probe.close();
    }
    QSqlDatabase::removeDatabase(name);
    return ok;
}

void DbService::migrateFrom(const QString &oldDbPath, const QString &newDbPath)
{
    if (oldDbPath == newDbPath || !QFile::exists(oldDbPath) || QFile::exists(newDbPath))
        return;

    int favoriteRows = -1;
    int playlistRows = -1;
    {
        QSqlDatabase old = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
                                                     QStringLiteral("migrate_probe"));
        old.setDatabaseName(oldDbPath);
        old.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
        if (old.open()) {
            QSqlQuery q(old);
            if (q.exec(QStringLiteral("SELECT count(*) FROM favorites")) && q.next())
                favoriteRows = q.value(0).toInt();
            if (q.exec(QStringLiteral("SELECT count(*) FROM playlists")) && q.next())
                playlistRows = q.value(0).toInt();
        }
        old.close();
        QSqlDatabase::removeDatabase(QStringLiteral("migrate_probe"));
    }

    if (favoriteRows <= 0 && playlistRows <= 0)
        return;

    if (QFile::copy(oldDbPath, newDbPath)) {
        qWarning() << "[db] 已把旧数据搬到可写目录:" << newDbPath
                   << "收藏条数:" << favoriteRows << "保存列表数:" << playlistRows;
        qWarning() << "[db] 旧库仍留在:" << oldDbPath;
    } else {
        qWarning() << "[db] 旧库搬迁失败，源:" << oldDbPath << "目标:" << newDbPath;
    }
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
        "folder_id INTEGER,"
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "PRIMARY KEY (type, id))"));
    query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS playlists ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT NOT NULL, "
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP, "
        "updated_at DATETIME DEFAULT CURRENT_TIMESTAMP)"));
    query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS playlist_songs ("
        "playlist_id INTEGER NOT NULL, "
        "position INTEGER NOT NULL, "
        "name TEXT NOT NULL, "
        "path TEXT NOT NULL, "
        "songer TEXT DEFAULT '', "
        "source INTEGER DEFAULT 0, "
        "PRIMARY KEY (playlist_id, position), "
        "FOREIGN KEY (playlist_id) REFERENCES playlists(id) ON DELETE CASCADE)"));

    addColumnIfMissing(db, QStringLiteral("songs"), QStringLiteral("tag_title"), QStringLiteral("TEXT DEFAULT ''"));
    addColumnIfMissing(db, QStringLiteral("songs"), QStringLiteral("tag_artist"), QStringLiteral("TEXT DEFAULT ''"));
    addColumnIfMissing(db, QStringLiteral("songs"), QStringLiteral("tag_cover"), QStringLiteral("TEXT DEFAULT ''"));
    addColumnIfMissing(db, QStringLiteral("songs"), QStringLiteral("tagged"), QStringLiteral("INTEGER DEFAULT 0"));
    addColumnIfMissing(db, QStringLiteral("favorites"), QStringLiteral("folder_id"), QStringLiteral("INTEGER"));
    addColumnIfMissing(db, QStringLiteral("folders"), QStringLiteral("sort_order"), QStringLiteral("INTEGER DEFAULT -1"));

    query.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_songs_folder ON songs(folder_id)"));
    query.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_folders_type ON folders(type)"));
    query.exec(QStringLiteral("CREATE UNIQUE INDEX IF NOT EXISTS idx_songs_path ON songs(folder_id, path)"));
    query.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_favorites_folder ON favorites(folder_id)"));
    query.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_favorites_type_created ON favorites(type, created_at DESC)"));

    query.prepare(QStringLiteral("SELECT COUNT(*) FROM folders WHERE type='my' AND name='默认文件夹'"));
    if (query.exec() && query.next() && query.value(0).toInt() == 0) {
        query.prepare(QStringLiteral("INSERT INTO folders (name, type) VALUES (:name, :type)"));
        query.bindValue(QStringLiteral(":name"), QStringLiteral("默认文件夹"));
        query.bindValue(QStringLiteral(":type"), QStringLiteral("my"));
        query.exec();
    }

    int favRoot = -1;
    if (query.exec(QStringLiteral("SELECT id FROM folders WHERE type='fav' ORDER BY id ASC LIMIT 1")) && query.next())
        favRoot = query.value(0).toInt();
    if (favRoot < 0) {
        QSqlQuery insert(db);
        insert.prepare(QStringLiteral("INSERT INTO folders (name, type) VALUES (:name, 'fav')"));
        insert.bindValue(QStringLiteral(":name"), QStringLiteral("默认收藏夹"));
        if (insert.exec())
            favRoot = int(insert.lastInsertId().toLongLong());
    }
    if (favRoot >= 0) {
        QSqlQuery adopt(db);
        adopt.prepare(QStringLiteral("UPDATE favorites SET folder_id = :folder "
                                     "WHERE type = 'song' AND folder_id IS NULL"));
        adopt.bindValue(QStringLiteral(":folder"), favRoot);
        adopt.exec();
    }

    query.exec(QStringLiteral("UPDATE folders SET sort_order = id WHERE sort_order < 0"));
}

#include "DbService.moc"