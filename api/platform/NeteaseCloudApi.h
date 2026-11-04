// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#ifndef NETEASECLOUDAPI_H
#define NETEASECLOUDAPI_H

#include <QList>
#include <QObject>
#include <QString>
#include <QVariant>

#include <atomic>

#include "MusicSource.h"

class CloudWorker;
class QThread;

class NeteaseCloudApi : public MusicSource
{
    Q_OBJECT
public:
    static constexpr int Source = ApiSource::Netease;

    explicit NeteaseCloudApi(QObject *parent = nullptr);
    ~NeteaseCloudApi() override;

    QString sourceName() const override { return QStringLiteral("网易云音乐"); }

    void applyCredentials(const SourceCredentials &credentials) override;

public slots:
    void setCookie(const QString &cookie);
    void setQuality(int quality);

protected:
    void onSearchSongs(const QString &keyword, int type, int page, int pageSize) override;
    void onSearchLyrics(const QString &keyword, int page, int pageSize) override;
    void onPlaylistMenu(int type) override;
    void onMusicPlaylists(const QString &tagid, int page, int pageSize) override;
    void onCategoryPlaylists(const QString &id, int page, int pageSize) override;
    void onPlaylistSongs(const QString &listid, int page, int pageSize) override;
    void onRecommendSongs(int page, int pageSize) override;
    void onHotPlaylistMenu(int type) override;
    void onHotPlaylists(int page, int pageSize) override;
    void onNewSongs(int type, int page, int pageSize) override;
    void onAllToplist() override;
    void onMusicToplist(int page, int pageSize, int rankid) override;
    void onHotSingers(int page, int pageSize) override;
    void onSingerCategory(int area, int page, int pageSize) override;
    void onSingerSongs(const QString &singerid, int page, int pageSize) override;
    void onMusicInfo(const QString &hash, int type) override;
    void onLyricInfo(const QString &hash, int duration) override;
    void onComments(const QString &hash, int page, int pageSize) override;
    void onPersonalFm(int page, int pageSize) override;
    void onPersonalRadar(int page, int pageSize) override;

signals:
    void setCookieRequest(const QString &cookie);

private slots:
    void onWorkerResult(const QString &action, const QVariant &data, int source);

private:
    void enqueue(const QString &action, const QString &member,
                 const QVariantMap &arg, const QVariantMap &extra = {});

    QList<CloudWorker *> m_workers;
    QList<QThread *> m_threads;
    int m_next = 0;
    std::atomic<int> m_quality{1};
    SourceCredentials m_credentials;
};

#endif
