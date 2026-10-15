// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// KugouApi — 酷狗音乐在线接口（替代 api/kugouapi.mjs + api/pako.mjs）
// 走 mobilecdnbj 系列公开接口（GET 无签名），KRC 歌词解码原生实现。
#ifndef KUGOUAPI_H
#define KUGOUAPI_H

#include <QList>
#include <QObject>
#include <QSharedPointer>
#include <QString>
#include <QStringList>
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

    void setCookie(const QString &cookie)
    {
        if (m_cookie != cookie)
            m_liteUserAuth.clear(); // 登录态变了，概念版用户级 auth 随之失效
        m_cookie = cookie;
    }
    // 设备标识（AccountManager 登录时生成/持久化，播放签名请求需要）
    void setDeviceInfo(const QString &mid, const QString &dfid)
    {
        if (!mid.isEmpty())
            m_mid = mid;
        if (!dfid.isEmpty())
            m_dfid = dfid;
    }
    // 音质档位（0 标准 128k / 1 高清 320k / 2+ 无损 flac），概念版取址要用
    void setQuality(int quality) { m_quality = qBound(0, quality, 3); }

    // 酷狗扫码登录鉴权辅助（原 WeCrypto；QCloudMusicApi 仅含网易云，不含酷狗签名）
    static QByteArray kugouWebSignature(const QJsonObject &params);
    static QString randomGuid();
    static QString kugouMidFromGuid(const QString &guid);

    // 与 QML 侧 action 一一对应的方法
    void searchSongs(const QString &keyword, int type, int page, int pageSize);
    void getPlaylistMenu(int type);
    void getMusicPlaylists(const QString &tagid, int page, int pageSize);
    void getCategoryPlaylists(const QString &categoryid, int page, int pageSize); // 分类歌单
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
    void getComments(const QString &hash, int page, int pageSize);
    void getPersonalFm(int page, int pageSize);     // 私人漫游 → TOP500 热门榜（分页）
    void getPersonalRadar(int page, int pageSize);  // 私人雷达 → 飙升榜（分页）

signals:
    // 统一结果协议：data 为 { info: [...] }（列表）或单条信息 map
    void resultReady(const QString &action, const QVariant &data, int source);

private:
    using Callback = std::function<void(const QJsonObject &)>;
    void get(const QString &url, const Callback &cb); // GET 请求 + JSON 回调
    void getLite(const QString &url, const Callback &cb); // 概念版：Android 头 + 设备头
    // 按候选键名依次查 cookie（扫码登录写 token/userid，网页 cookie 可能是 t/KugooID）
    QString cookieValue(const QStringList &keys) const;
    // 评论网关的安卓端签名：算法与 kugouWebSignature 相同，盐不同
    static QByteArray kugouAndroidSignature(const QVariantMap &params);
    // play/songinfo 的公共参数（设备 + 登录态），hash/encode_album_audio_id 由调用方补
    QJsonObject songInfoBaseParams() const;
    QVariantMap liteBaseParams() const;

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
        // 概念版（lite）auth/tracker 需要，取自 getSongInfo.php
        QString albumId;
        QString albumAudioId;
    };

    void requestPlayMeta(const QString &hash, int type, const QSharedPointer<PlayRequest> &st);
    void requestTrackerUrl(const QString &hash, int type, const QSharedPointer<PlayRequest> &st);
    // 取址顺序：概念版 → web 签名（两条 VIP），都失败才用免费/128k 兜底
    void finishPlayInfo(const QString &hash, int type, const QSharedPointer<PlayRequest> &st);
    void finishWithFallback(const QString &hash, int type,
                            const QSharedPointer<PlayRequest> &st, const QString &reason,
                            int transportError = 0);
    // 概念版（lite）取址：先 /v1/authorization 拿 auth，再 /tracker/v5/url 换地址
    void requestLitePlayInfo(const QString &hash, int type,
                             const QSharedPointer<PlayRequest> &st, const QString &reason);
    // 概念版是三步：/v1/user_verify（用户级 auth）→ /v1/authorization（歌曲级）→ /tracker/v5/url
    void requestLiteSongAuth(const QString &hash, int type,
                             const QSharedPointer<PlayRequest> &st, const QString &userAuth,
                             const QString &reason);
    void requestLiteUrlStep(const QString &hash, int type,
                            const QSharedPointer<PlayRequest> &st, const QString &auth,
                            const QString &openTime, int quality, const QString &reason);
    // web 签名两步取址：hash → encode_album_audio_id → play_url
    void requestSignedPlayInfo(const QString &hash, int type,
                               const QSharedPointer<PlayRequest> &st, const QString &reason);
    void requestSongInfo(const QJsonObject &params, const QString &hash, int type,
                         const QSharedPointer<PlayRequest> &st, const QString &reason,
                         const QJsonObject &metaFallback = QJsonObject());

    // KRC 歌词解码（替代 pako.mjs inflateRaw）
    static QString decodeKrc(const QByteArray &base64);
    static QVariantList krcToLyrics(const QString &krc);      // 字级 {time,text,info}
    static QVariantList krcTranslations(const QString &krc);  // [language:] 翻译

    QNetworkAccessManager *m_nam = nullptr;
    QString m_cookie;
    QString m_mid;
    QString m_dfid;
    int m_quality = 1;
    QString m_liteUserAuth;   // 概念版用户级 auth，同一登录态内复用
    QHash<QString, QString> m_mixIdCache;   // hash → 评论接口所需的数字 mixsongid
};

#endif // KUGOUAPI_H
