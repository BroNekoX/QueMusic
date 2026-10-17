// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
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
    // SQLite 的 readonly 是「连接级」粘性状态：某次写失败后这条连接会一直报 readonly，
    // 之后所有写操作（收藏、歌单…）全部失效。所以写失败时要重开连接，而不是继续用它。
    bool writeFailed = false;

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
        // 所有写都走这一条线程（GUI 侧只读），不需要 WAL 的读写并发；
        // 而 WAL 依赖 -wal/-shm 两个附属文件，被杀软/同步盘拦住时会让整库写不进去
        //（表现为 attempt to write a readonly database）。所以用单文件的 DELETE 日志。
        QSqlQuery pragma(db);
        pragma.exec(QStringLiteral("PRAGMA journal_mode = DELETE"));
        pragma.exec(QStringLiteral("PRAGMA synchronous = NORMAL"));
        pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON"));
        DbService::ensureSchema(db);

        // 建表也是写操作：拿不到写锁就是一整库只读，开机就说清楚，别等用户点收藏才发现
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
                if (queue.empty())
                    return;
                job = std::move(queue.front());
                queue.pop_front();
            }
            if (!job)
                continue;
            job(db);
            if (writeFailed) {
                writeFailed = false;
                qWarning() << "[db] 写失败（连接已转为只读），重开连接后重试一次";
                reopen();
                job(db);
            }
        }
    }

    void markWriteFailed(const QSqlError &error)
    {
        const QString text = error.databaseText() + error.driverText();
        if (error.nativeErrorCode() == QLatin1String("8")   // SQLITE_READONLY
            || text.contains(QLatin1String("readonly"), Qt::CaseInsensitive)
            || text.contains(QLatin1String("read-only"), Qt::CaseInsensitive))
            writeFailed = true;
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

void DbService::markWriteFailed(const QSqlError &error)
{
    instance()->m_impl->worker->markWriteFailed(error);
}

QString DbService::dataDir()
{
    // 路径语义保持 QStandardPaths 原样（AppDataLocation），不人为改写；
    // 只多加一层保险：该目录若写不进 SQLite（受限令牌/安全软件拦截），
    // 就退到程序目录，避免收藏这类写操作直接失效。
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

        // 换了目录就把旧库搬过去，别让用户的收藏凭空消失
        const QString target = chosen + QStringLiteral("/player_data.db");
        if (chosen != preferred && !QFile::exists(target))
            migrateFrom(preferred + QStringLiteral("/player_data.db"), target);

        qInfo() << "[db] 数据目录:" << chosen;
        return chosen;
    }();
    return dir;
}

// 只判断目录权限不够：安全软件会在 SQLite 真正写入时才拦，所以建个表试一次
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

// 换目录后旧库里的收藏/歌单要跟着走，否则用户看到的是空列表
void DbService::migrateFrom(const QString &oldDbPath, const QString &newDbPath)
{
    if (oldDbPath == newDbPath || !QFile::exists(oldDbPath) || QFile::exists(newDbPath))
        return;

    int rows = -1;
    {
        // 只读打开旧库探内容：它多半就是那个写不进去的库，读是没问题的
        QSqlDatabase old = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("migrate_probe"));
        old.setDatabaseName(oldDbPath);
        old.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
        if (old.open()) {
            QSqlQuery q(old);
            if (q.exec(QStringLiteral("SELECT count(*) FROM favorites")) && q.next())
                rows = q.value(0).toInt();
        }
        old.close();
        QSqlDatabase::removeDatabase(QStringLiteral("migrate_probe"));
    }

    if (rows <= 0)
        return;   // 旧库是空的，没什么可搬

    if (QFile::copy(oldDbPath, newDbPath)) {
        qWarning() << "[db] 已把旧数据搬到可写目录:" << newDbPath << "收藏条数:" << rows;
        qWarning() << "[db] 旧库仍留在:" << oldDbPath << "（确认无误后可自行删除）";
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

    // TAG 结果落库，避免每次刷新都重跑 TagLib
    addColumnIfMissing(db, QStringLiteral("songs"), QStringLiteral("tag_title"), QStringLiteral("TEXT DEFAULT ''"));
    addColumnIfMissing(db, QStringLiteral("songs"), QStringLiteral("tag_artist"), QStringLiteral("TEXT DEFAULT ''"));
    addColumnIfMissing(db, QStringLiteral("songs"), QStringLiteral("tag_cover"), QStringLiteral("TEXT DEFAULT ''"));
    addColumnIfMissing(db, QStringLiteral("songs"), QStringLiteral("tagged"), QStringLiteral("INTEGER DEFAULT 0"));
    addColumnIfMissing(db, QStringLiteral("favorites"), QStringLiteral("folder_id"), QStringLiteral("INTEGER"));
    query.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_songs_folder ON songs(folder_id)"));
    query.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_folders_type ON folders(type)"));
    query.exec(QStringLiteral("CREATE UNIQUE INDEX IF NOT EXISTS idx_songs_path ON songs(folder_id, path)"));
    query.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_favorites_folder ON favorites(folder_id)"));

    // 首次运行时补一个默认歌单
    query.prepare(QStringLiteral("SELECT COUNT(*) FROM folders WHERE type='my' AND name='默认文件夹'"));
    if (query.exec() && query.next() && query.value(0).toInt() == 0) {
        query.prepare(QStringLiteral("INSERT INTO folders (name, type) VALUES (:name, :type)"));
        query.bindValue(QStringLiteral(":name"), QStringLiteral("默认文件夹"));
        query.bindValue(QStringLiteral(":type"), QStringLiteral("my"));
        query.exec();
    }

    // 收藏夹：没有就建默认夹，并把历史上没归属的歌曲收藏迁进去（保证老收藏不丢）
    int favRoot = -1;
    if (query.exec(QStringLiteral("SELECT id FROM folders WHERE type='fav' ORDER BY id ASC LIMIT 1")) && query.next())
        favRoot = query.value(0).toInt();
    if (favRoot < 0) {
        QSqlQuery insert(db);
        insert.prepare(QStringLiteral("INSERT INTO folders (name, type) VALUES (:name, 'fav')"));
        insert.bindValue(QStringLiteral(":name"), QStringLiteral("默认收藏夹"));
        if (insert.exec())
            favRoot = insert.lastInsertId().toInt();
    }
    if (favRoot >= 0) {
        QSqlQuery adopt(db);
        adopt.prepare(QStringLiteral("UPDATE favorites SET folder_id = :folder "
                                     "WHERE type = 'song' AND folder_id IS NULL"));
        adopt.bindValue(QStringLiteral(":folder"), favRoot);
        adopt.exec();
    }
}

#include "DbService.moc"
