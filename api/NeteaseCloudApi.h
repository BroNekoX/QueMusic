// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// NeteaseCloudApi —— 基于 QCloudMusicApi（第三方网易云音乐在线接口，
// 完整复刻 NeteaseCloudMusicApi 的 weapi 加密协议）的网易云实现，
// 用于替代旧的 NeteaseApi（老 web 接口，已废弃、缺陷多、字段不全）。
//
// 对外接口签名与旧 NeteaseApi 完全一致，对 MusicApiService / QML 完全透明，
// 因此只需替换 MusicApiService 中的 m_netease 实例即可。
//
// 重要：QCloudMusicApi 的 ApiHelper::invoke() 是“同步阻塞”调用（内部用事件
// 循环等待网络返回），若在主线程调用会卡住 UI。因此所有网络请求都放到独立
// 工作线程（CloudWorker）中执行，结果通过信号回传到主线程再发出 resultReady。
#ifndef NETEASECLOUDAPI_H
#define NETEASECLOUDAPI_H

#include <QObject>
#include <QString>
#include <QVariant>

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
    // 仅供内部使用：把请求投递到工作线程执行
    void runRequest(const QString &action, const QVariantMap &call);
    // 仅供内部使用：把登录态 Cookie 投递到工作线程
    void setCookieRequest(const QString &cookie);

public slots:
    void setCookie(const QString &cookie); // 同步 AccountManager 的网易云登录态
    void searchSongs(const QString &keyword, int type, int page, int pageSize);
    void getPlaylistMenu(int type);
    void getMenuInfo(const QString &id);
    void getMusicPlaylists(const QString &tagid, int page, int pageSize);
    void getPlaylistSongs(const QString &listid, int page, int pageSize);
    void getRecommendSongs(int page, int pageSize);
    void getHotPlaylistMenu(int type);
    void getHotPlaylists(int page, int pageSize);
    void getNewSongs(int type, int page, int pageSize);
    void getAllToplist();
    void getMusicToplist(int rankid);
    void getHotSingers(int page, int pageSize);
    void getSingerSongs(const QString &singerid, int page, int pageSize);
    void getMusicInfo(const QString &hash, int type);
    void getLyricInfo(const QString &hash, int duration);

private slots:
    void onWorkerResult(const QString &action, const QVariant &data, int source);

private:
    void enqueue(const QString &action, const QVariantMap &call);

    CloudWorker *m_worker = nullptr;
    QThread *m_thread = nullptr;
};

#endif // NETEASECLOUDAPI_H
