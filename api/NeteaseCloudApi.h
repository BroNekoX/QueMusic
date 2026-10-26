// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#ifndef NETEASECLOUDAPI_H
#define NETEASECLOUDAPI_H

#include <QList>
#include <QObject>
#include <QString>
#include <QVariant>

#include <atomic>

class CloudWorker;
class QThread;

class NeteaseCloudApi : public QObject
{
    Q_OBJECT
public:
    explicit NeteaseCloudApi(QObject *parent = nullptr);
    ~NeteaseCloudApi();

signals:
    void resultReady(const QString &action, const QVariant &data, int source);
    void setCookieRequest(const QString &cookie);

public slots:
    void setCookie(const QString &cookie);
    void setQuality(int quality);
    void searchSongs(const QString &keyword, int type, int page, int pageSize);
    void getPlaylistMenu(int type);
    void getMusicPlaylists(const QString &tagid, int page, int pageSize);
    void getPlaylistSongs(const QString &listid, int page, int pageSize);
    void getRecommendSongs(int page, int pageSize);
    void getHotPlaylistMenu(int type);
    void getHotPlaylists(int page, int pageSize);
    void getNewSongs(int type, int page, int pageSize);
    void getAllToplist();
    void getMusicToplist(int page, int pageSize, int rankid);
    void getHotSingers(int page, int pageSize);
    void getSingerCategory(int area, int page, int pageSize);
    void getSingerSongs(const QString &singerid, int page, int pageSize);
    void getMusicInfo(const QString &hash, int type);
    void getLyricInfo(const QString &hash, int duration);
    void getComments(const QString &hash, int page, int pageSize);
    void getPersonalFm(int page, int pageSize);
    void getPersonalRadar(int page, int pageSize);

private slots:
    void onWorkerResult(const QString &action, const QVariant &data, int source);

private:
    void enqueue(const QString &action, const QString &member,
                 const QVariantMap &arg, const QVariantMap &extra = {});

    QList<CloudWorker *> m_workers;
    QList<QThread *> m_threads;
    int m_next = 0;
    std::atomic<int> m_quality{1};
};

#endif