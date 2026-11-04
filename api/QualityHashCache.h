// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// base -> {hq,sq} 与反向表：按音质解析播放 hash，并把响应 hash 还原成 base。

#ifndef QUALITYHASHCACHE_H
#define QUALITYHASHCACHE_H

#include <QHash>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariantList>

class QualityHashCache : public QObject
{
    Q_OBJECT

public:
    explicit QualityHashCache(QObject *parent = nullptr);
    ~QualityHashCache() override;

    // 从列表结果里学习 hashhq / hashsq。
    void remember(const QVariantList &items);

    // 按音质档位选择实际请求的 hash；未知 hash 原样返回。
    QString resolve(const QString &hash, int quality) const;

    // 播放响应里的 hash -> 列表里展示用的 base hash。
    QString baseOf(const QString &hash) const;

private:
    struct Alternates
    {
        QString hq;
        QString sq;
    };

    static QString cachePath();
    void load();
    void save();
    void scheduleSave();
    void pruneIfNeeded();

    QHash<QString, Alternates> m_alternates;
    QHash<QString, QString> m_baseOf;
    QTimer m_saveTimer;
    bool m_dirty = false;
};

#endif // QUALITYHASHCACHE_H
