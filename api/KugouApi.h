// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// KugouApi — 酷狗音乐在线接口（替代 api/kugouapi.mjs + api/pako.mjs）
// 走 mobilecdnbj 系列公开接口（GET 无签名），KRC 歌词解码原生实现。
#ifndef KUGOUAPI_H
#define KUGOUAPI_H

#include <QObject>
#include <QSharedPointer>
#include <QString>
#include <QVariant>
#include <QVariantMap>
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
    // 设备标识（AccountManager 登录时生成/持久化，播放签名请求需要）
    void setDeviceInfo(const QString &mid, const QString &dfid)
    {
        if (!mid.isEmpty())
            m_mid = mid;
        if (!dfid.isEmpty())
            m_dfid = dfid;
    }

    // 酷狗扫码登录鉴权辅助（原 WeCrypto；QCloudMusicApi 仅含网易云，不含酷狗签名）
    static QByteArray kugouWebSignature(const QJsonObject &params);
    static QString randomGuid();
    static QString kugouMidFromGuid(const QString &guid);

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
    void getAllToplist();                       // 榜单列表（rank/list 动态接口）
    void getMusicToplist(int page, int pageSize, int rankid);           // 榜单歌曲（rankid: 6666 飙升 / 8888 TOP500）
    void getHotSingers(int page, int pageSize); // 热门歌手（singer/rank）
    void getSingerCategory(int area, int page, int pageSize); // 歌手分类（singer/list）
    void getSingerSongs(const QString &singerid, int page, int pageSize); // 歌手歌曲
    void getMusicInfo(const QString &hash, int type);
    void getLyricInfo(const QString &hash, int duration);
    void getPersonalFm(int page, int pageSize);     // 私人漫游 → TOP500 热门榜（分页）
    void getPersonalRadar(int page, int pageSize);  // 私人雷达 → 飙升榜（分页）

signals:
    // 统一结果协议：data 为 { info: [...] }（列表）或单条信息 map
    void resultReady(const QString &action, const QVariant &data, int source);

private:
    using Callback = std::function<void(const QJsonObject &)>;
    void get(const QString &url, const Callback &cb); // GET 请求 + JSON 回调
    QString cookieValue(const QString &key) const;
    // 登录态取 VIP 播放地址；reason 为无地址原因（"vip" 需要会员/购买，供上层提示）
    void requestSignedPlayInfo(const QString &hash, int type, const QString &reason = QString());

    // getMusicInfo 两路并行请求的合并状态：
    //   meta    → getSongInfo.php（歌名/歌手/封面/时长 + 128k 兜底地址）
    //   tracker → trackercdn v2（按 hash 精确取音质：320hash → 320k，sqhash → flac）
    struct PlayRequest {
        QVariantMap meta;
        QString metaUrl;
        QString metaBackupUrl;
        bool metaDone = false;
        QString trackerUrl;
        QString trackerExt;
        int trackerRate = 0;
        int trackerStatus = 0; // 2 = 该曲/该音质需要会员或已购买
        bool trackerDone = false;
    };

    void requestPlayMeta(const QString &hash, int type, const QSharedPointer<PlayRequest> &st);
    void requestTrackerUrl(const QString &hash, int type, const QSharedPointer<PlayRequest> &st);
    void finishPlayInfo(const QString &hash, int type, const QSharedPointer<PlayRequest> &st);

    // KRC 歌词解码（替代 pako.mjs inflateRaw）
    static QString decodeKrc(const QByteArray &base64);
    static QVariantList krcToLyrics(const QString &krc);      // 字级 {time,text,info}
    static QVariantList krcTranslations(const QString &krc);  // [language:] 翻译

    QNetworkAccessManager *m_nam = nullptr;
    QString m_cookie;
    QString m_mid;
    QString m_dfid;
};

#endif // KUGOUAPI_H
