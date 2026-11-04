// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// 在线音源统一接口：request(action,args) -> onXxx()。
// 新平台只需 override 自己支持的钩子，其余 action 自动回 error。

#ifndef MUSICSOURCE_H
#define MUSICSOURCE_H

#include <QHash>
#include <QList>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

#include "ApiActions.h"

// 音源当前需要的账号 / 音质上下文。宿主只在登录态或音质变化时重建，
// 避免每个请求都去拼接 cookie。
struct SourceCredentials
{
    QString cookie;
    QString deviceMid;
    QString deviceDfid;
    bool loggedIn = false;
    int quality = 1;

    bool operator==(const SourceCredentials &other) const
    {
        return cookie == other.cookie && deviceMid == other.deviceMid
            && deviceDfid == other.deviceDfid && loggedIn == other.loggedIn
            && quality == other.quality;
    }
};

class MusicSource : public QObject
{
    Q_OBJECT

public:
    explicit MusicSource(int sourceId, QObject *parent = nullptr);
    ~MusicSource() override = default;

    virtual QString sourceName() const = 0;
    virtual bool isPlugin() const { return false; }

    // 登录态 / 音质变化时由宿主调用；实现方应自行比较，避免重复处理。
    virtual void applyCredentials(const SourceCredentials &credentials)
    {
        Q_UNUSED(credentials)
    }

    // 唯一入口。默认实现按 action 分发到 onXxx()；多音源宿主
    // （ScriptSourceHost）可以整体覆盖。
    virtual void request(int sourceId, const QString &action, const QVariantMap &args);

signals:
    void resultReady(const QString &action, const QVariant &data, int source);

protected:
    // ---- 平台能力钩子（未实现 = 不支持） ----
    virtual void onSearchSongs(const QString &keyword, int type, int page, int pageSize)
    { Q_UNUSED(keyword) Q_UNUSED(type) Q_UNUSED(page) Q_UNUSED(pageSize) unsupported(ApiAction::SearchSongs); }
    virtual void onSearchLyrics(const QString &keyword, int page, int pageSize)
    { Q_UNUSED(keyword) Q_UNUSED(page) Q_UNUSED(pageSize) unsupported(ApiAction::SearchLyrics); }
    virtual void onPlaylistMenu(int type)
    { Q_UNUSED(type) unsupported(ApiAction::PlaylistMenu); }
    virtual void onMusicPlaylists(const QString &tagid, int page, int pageSize)
    { Q_UNUSED(tagid) Q_UNUSED(page) Q_UNUSED(pageSize) unsupported(ApiAction::MusicPlaylists); }
    virtual void onCategoryPlaylists(const QString &id, int page, int pageSize)
    { Q_UNUSED(id) Q_UNUSED(page) Q_UNUSED(pageSize) unsupported(ApiAction::CategoryPlaylists); }
    virtual void onPlaylistSongs(const QString &listid, int page, int pageSize)
    { Q_UNUSED(listid) Q_UNUSED(page) Q_UNUSED(pageSize) unsupported(ApiAction::PlaylistSongs); }
    virtual void onRecommendSongs(int page, int pageSize)
    { Q_UNUSED(page) Q_UNUSED(pageSize) unsupported(ApiAction::RecommendSongs); }
    virtual void onHotPlaylistMenu(int type)
    { Q_UNUSED(type) unsupported(ApiAction::HotPlaylistMenu); }
    virtual void onHotPlaylists(int page, int pageSize)
    { Q_UNUSED(page) Q_UNUSED(pageSize) unsupported(ApiAction::HotPlaylists); }
    virtual void onNewSongs(int type, int page, int pageSize)
    { Q_UNUSED(type) Q_UNUSED(page) Q_UNUSED(pageSize) unsupported(ApiAction::NewSongs); }
    virtual void onAllToplist() { unsupported(ApiAction::AllToplist); }
    virtual void onMusicToplist(int page, int pageSize, int rankid)
    { Q_UNUSED(page) Q_UNUSED(pageSize) Q_UNUSED(rankid) unsupported(ApiAction::MusicToplist); }
    virtual void onHotSingers(int page, int pageSize)
    { Q_UNUSED(page) Q_UNUSED(pageSize) unsupported(ApiAction::HotSingers); }
    virtual void onSingerCategory(int area, int page, int pageSize)
    { Q_UNUSED(area) Q_UNUSED(page) Q_UNUSED(pageSize) unsupported(ApiAction::SingerCategory); }
    virtual void onSingerSongs(const QString &singerid, int page, int pageSize)
    { Q_UNUSED(singerid) Q_UNUSED(page) Q_UNUSED(pageSize) unsupported(ApiAction::SingerSongs); }
    virtual void onMusicInfo(const QString &hash, int type)
    { Q_UNUSED(hash) Q_UNUSED(type) unsupported(ApiAction::MusicInfo); }
    virtual void onLyricInfo(const QString &hash, int duration)
    { Q_UNUSED(hash) Q_UNUSED(duration) unsupported(ApiAction::LyricInfo); }
    virtual void onComments(const QString &hash, int page, int pageSize)
    { Q_UNUSED(hash) Q_UNUSED(page) Q_UNUSED(pageSize) unsupported(ApiAction::Comments); }
    virtual void onPersonalFm(int page, int pageSize)
    { Q_UNUSED(page) Q_UNUSED(pageSize) unsupported(ApiAction::PersonalFm); }
    virtual void onPersonalRadar(int page, int pageSize)
    { Q_UNUSED(page) Q_UNUSED(pageSize) unsupported(ApiAction::PersonalRadar); }

    void deliver(const QString &action, const QVariant &data, int source);
    void deliverError(const QString &action, int source, int code,
                      const QString &message = QString());

private:
    void unsupported(const QString &action);

    int m_sourceId;
};

// 轻量注册表：sourceId -> MusicSource*。同一个实例可以注册到多个 id
// （脚本宿主用这种形式服务所有插件）。
class MusicSourceRegistry
{
public:
    void map(int sourceId, MusicSource *source) { m_sources.insert(sourceId, source); }
    void unmap(int sourceId) { m_sources.remove(sourceId); }
    void clear() { m_sources.clear(); }

    MusicSource *source(int sourceId) const { return m_sources.value(sourceId); }
    bool contains(int sourceId) const { return m_sources.contains(sourceId); }
    QList<int> ids() const { return m_sources.keys(); }

private:
    QHash<int, MusicSource *> m_sources;
};

#endif // MUSICSOURCE_H
