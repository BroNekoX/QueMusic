// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// 远端文件缓存：播放时后台把「音频 + 同名歌词 + 同目录封面」落到同一目录，
// 落地后标签/歌词/封面直接复用本地那套（CoverHelper + LocalLyricsReader）
#pragma once

#include <QElapsedTimer>
#include <QList>
#include <QObject>
#include <QString>
#include <QtQml/qqml.h>

class QNetworkAccessManager;

class WebDavCache : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit WebDavCache(QObject *parent = nullptr);
    static WebDavCache *create(QQmlEngine *, QJSEngine *) { return new WebDavCache(); }

    // 已落地则返回本地音频路径；未落地返回空（调用方直接流式播）
    Q_INVOKABLE QString localPathFor(const QString &url) const { return cachedAudioFor(url); }
    // 落地目录 / 落地音频 / 落地封面：模型侧也要按同一规则查，统一放静态实现，避免两处算路径
    static QString cachedDirFor(const QString &url);
    static QString cachedAudioFor(const QString &url);
    static QString cachedCoverFor(const QString &url);
    Q_INVOKABLE void cache(const QString &url, const QString &authHeader,
                           const QString &lyricsUrl = QString(),
                           const QString &coverUrl = QString());
    Q_INVOKABLE void clear();

signals:
    void cached(const QString &url, const QString &localPath);

private:
    struct Task {
        QString url;
        QString authHeader;
        QString dir;
        QString audioName;
        QString lyricsUrl;
        QString coverUrl;
        int step = 0;             // 0 音频 / 1 同名歌词 / 2 封面
    };

    void startNext();
    void downloadNext(Task *task);
    void finish(const QString &url, const QString &localPath);
    void prune();

    QNetworkAccessManager *m_nam = nullptr;
    QList<Task> m_queue;
    bool m_busy = false;
    QElapsedTimer m_pruneStamp;      // prune 要扫全目录，限频
};
