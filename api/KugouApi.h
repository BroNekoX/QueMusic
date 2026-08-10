// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// KugouApi — 酷狗音乐在线接口（替代 api/kugouapi.mjs + api/pako.mjs）
// 走 mobilecdnbj 系列公开接口（GET 无签名），KRC 歌词解码原生实现。
#ifndef KUGOUAPI_H
#define KUGOUAPI_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <functional>

class QJsonObject;
class QNetworkAccessManager;
class QNetworkReply;

class KugouApi : public QObject
{
    Q_OBJECT
public:
    static constexpr int Source = 0; // 平台编号（0 酷狗 / 1 网易云）

    explicit KugouApi(QObject *parent = nullptr);

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
    void getAllToplist();                       // 榜单列表（本地固定）
    void getMusicToplist(int rankid);           // 榜单歌曲（rankid: 6666 飙升 / 8888 TOP500）
    void getHotSingers(int page, int pageSize); // 热门歌手
    void getSingerSongs(const QString &singerid, int page, int pageSize); // 歌手歌曲
    void getMusicInfo(const QString &hash, int type);
    void getLyricInfo(const QString &hash, int duration);

signals:
    // 统一结果协议：data 为 { info: [...] }（列表）或单条信息 map
    void resultReady(const QString &action, const QVariant &data, int source);

private:
    using Callback = std::function<void(const QJsonObject &)>;
    void get(const QString &url, const Callback &cb); // GET 请求 + JSON 回调

    // KRC 歌词解码（替代 pako.mjs inflateRaw）
    static QString decodeKrc(const QByteArray &base64);
    static QVariantList krcToLyrics(const QString &krc);      // 字级 {time,text,info}
    static QVariantList krcTranslations(const QString &krc);  // [language:] 翻译

    QNetworkAccessManager *m_nam = nullptr;
    QString m_cookie;
};

#endif // KUGOUAPI_H
