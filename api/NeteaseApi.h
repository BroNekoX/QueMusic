// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// NeteaseApi — 网易云音乐在线接口（替代 api/necloudapi.mjs）
// 走 music.163.com/api/ 老接口（GET 无加密），登录 Cookie 由外部注入。
#ifndef NETEASEAPI_H
#define NETEASEAPI_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <functional>

class QJsonArray;
class QJsonObject;
class QJsonValue;
class QNetworkAccessManager;
class QNetworkReply;

class NeteaseApi : public QObject
{
    Q_OBJECT
public:
    static constexpr int Source = 1; // 平台编号（0 酷狗 / 1 网易云）

    explicit NeteaseApi(QObject *parent = nullptr);

    void setCookie(const QString &cookie) { m_cookie = cookie; }

    // 与 QML 侧 action 一一对应的方法
    void searchSongs(const QString &keyword, int type, int page, int pageSize);
    void getPlaylistMenu(int type);
    void getMenuInfo(const QString &id);
    void getMusicPlaylists(const QString &tagid, int page, int pageSize);
    void getPlaylistSongs(const QString &listid, int page, int pageSize);
    void getRecommendSongs(int page, int pageSize);
    void getHotPlaylistMenu(int type);
    void getHotPlaylists(int page, int pageSize);
    void getNewSongs(int type, int page, int pageSize);
    void getAllToplist();                       // 榜单列表（/api/toplist）
    void getMusicToplist(int rankid);           // 榜单歌曲（榜单本质是歌单）
    void getHotSingers(int page, int pageSize); // 热门歌手（/api/top/artists）
    void getSingerSongs(const QString &singerid, int page, int pageSize); // 歌手歌曲
    void getMusicInfo(const QString &hash, int type);
    void getLyricInfo(const QString &hash, int duration);

signals:
    // 统一结果协议：data 为 { info: [...] }（列表）或单条信息 map
    void resultReady(const QString &action, const QVariant &data, int source);

private:
    using Callback = std::function<void(const QJsonObject &)>;
    void get(const QString &url, const Callback &cb); // GET 请求 + JSON 回调

    static QString songId(const QJsonValue &v);              // id → 字符串（防大数精度丢失）
    static QVariantList parseSongList(const QJsonArray &s);  // 歌曲数组 → 统一字段
    static QVariantList parseLyrics(const QString &text);    // LRC 文本 → [{time,text,info}]

    QNetworkAccessManager *m_nam = nullptr;
    QString m_cookie;
};

#endif // NETEASEAPI_H
