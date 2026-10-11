// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// 串行化 SQLite 工作线程：连接有线程亲和性，同步查询会冻住 GUI。
// 用法：submit() 的任务体里只碰 db，结果用 post() 投回对象所属线程。
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

    // 故意不析构，避开静态销毁顺序问题
    static DbService *instance();

    // FIFO 执行，天然串行
    void submit(Job job);

    // 写操作失败时调用：让工作线程重开连接并重试本轮任务。
    // SQLite 的 readonly 是连接级粘性状态，不重开的话后续所有写都会一直失败。
    static void markWriteFailed(const QSqlError &error);

    // 投递到 target 所属线程；已销毁则丢弃
    template <typename F>
    static void post(QObject *target, F &&fn)
    {
        if (!target)
            return;
        QMetaObject::invokeMethod(target, std::forward<F>(fn), Qt::QueuedConnection);
    }

    static QString dataDir();

    // 真建一条连接试写一次（光看目录权限不够，安全软件会在 SQLite 写入时拦）
    static bool dbWritable(const QString &dbPath);

    // 换了数据目录后把旧库搬过去，避免用户看到空收藏
    static void migrateFrom(const QString &oldDbPath, const QString &newDbPath);

    // 封面等缓存目录（已确保存在）
    static QString cacheDir();

    // 建表 + 增量迁移，只在 DB 线程调用
    static void ensureSchema(QSqlDatabase &db);

private:
    DbService();
    ~DbService();
    DbService(const DbService &) = delete;
    DbService &operator=(const DbService &) = delete;

    struct Impl;
    Impl *m_impl = nullptr;
};

#endif // DBSERVICE_H
