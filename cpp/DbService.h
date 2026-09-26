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

class DbService
{
public:
    using Job = std::function<void(QSqlDatabase &)>;

    // 故意不析构，避开静态销毁顺序问题
    static DbService *instance();

    // FIFO 执行，天然串行
    void submit(Job job);

    // 投递到 target 所属线程；已销毁则丢弃
    template <typename F>
    static void post(QObject *target, F &&fn)
    {
        if (!target)
            return;
        QMetaObject::invokeMethod(target, std::forward<F>(fn), Qt::QueuedConnection);
    }

    static QString dataDir();

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
