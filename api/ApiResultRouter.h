// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// action -> 模型/信号 + 归一化；新增 action 只改这一张表。

#ifndef APIRESULTROUTER_H
#define APIRESULTROUTER_H

#include <QObject>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

class OnlineListModel;
class QualityHashCache;

struct OnlineModels
{
    OnlineListModel *searchResults = nullptr;
    OnlineListModel *newSongs = nullptr;
    OnlineListModel *recommendSongs = nullptr;
    OnlineListModel *musicPlaylists = nullptr;
    OnlineListModel *playlistMenu = nullptr;
    OnlineListModel *playlistSongs = nullptr;
    OnlineListModel *hotPlaylists = nullptr;
    OnlineListModel *hotlistMenu = nullptr;
    OnlineListModel *musicToplist = nullptr;
    OnlineListModel *toplistList = nullptr;
    OnlineListModel *singers = nullptr;
    OnlineListModel *personalFm = nullptr;
    OnlineListModel *personalRadar = nullptr;
    OnlineListModel *comments = nullptr;
};

class ApiResultRouter : public QObject
{
    Q_OBJECT

public:
    ApiResultRouter(const OnlineModels &models, QualityHashCache *qualityCache,
                    QObject *parent = nullptr);
    ~ApiResultRouter() override = default;

    void setSongFilter(int filter) { m_songFilter = filter; }

    // 返回 false 表示未知 action（调用方负责打日志）。
    bool route(const QString &action, const QVariant &data, int source);

signals:
    void commentsHasMoreChanged(bool hasMore);
    void personalFallbackWarning(const QString &text);
    void musicInfoReady(const QVariantMap &info, int source);
    void lyricInfoReady(const QVariantMap &info, int source);
    void lyricSearchReady(const QVariant &info, int source, bool failed);

private:
    QVariantList songList(const QVariant &value) const;

    OnlineModels m_models;
    QualityHashCache *m_qualityCache = nullptr;
    int m_songFilter = 0;
};

#endif // APIRESULTROUTER_H
