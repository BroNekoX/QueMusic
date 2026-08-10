// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#include "NeteaseApi.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QUrl>
#include <QUrlQuery>

#include "ApiCommon.h"

namespace {
const char *kUa = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
                  "(KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36";
// 宏而不是变量：QStringLiteral 要求编译期字面量参数
#define kBase "http://music.163.com"
} // namespace

NeteaseApi::NeteaseApi(QObject *parent)
    : QObject(parent)
{
    m_nam = new QNetworkAccessManager(this);
}

// 通用 GET 请求（带 UA / Referer / Cookie，自动解析 JSON）
void NeteaseApi::get(const QString &url, const Callback &cb)
{
    QNetworkRequest req(url);
    req.setRawHeader("User-Agent", kUa);
    req.setRawHeader("Referer", "https://music.163.com/");
    if (!m_cookie.isEmpty())
        req.setRawHeader("Cookie", m_cookie.toUtf8());

    qDebug() << "正在请求网易云api：" << url;
    QNetworkReply *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, cb] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "[netease] 请求失败:" << reply->errorString()
                       << reply->url().toString();
            cb(QJsonObject()); // 错误也回调，保证 QML 侧 loadState 能复位
            return;
        }
        cb(QJsonDocument::fromJson(reply->readAll()).object());
    });
}

// 工具
QString NeteaseApi::songId(const QJsonValue &v)
{
    // 网易云 id 是 64 位整数，JSON 解析为 double，用 toVariant 转回整数防精度丢失
    return QString::number(v.toVariant().toLongLong());
}

QVariantList NeteaseApi::parseSongList(const QJsonArray &songs)
{
    QVariantList info;
    for (const QJsonValue &v : songs) {
        const QJsonObject s = v.toObject();
        const QJsonArray artists = s.value("artists").toArray();
        const QJsonObject album = s.value("album").toObject();
        const int fee = s.value("fee").toInt();
        const QString id = songId(s.value("id"));
        info << ApiCommon::song(
            s.value("name").toString(),
            artists.isEmpty() ? QStringLiteral("Artist")
                              : artists.first().toObject().value("name").toString(),
            album.value("artist").toObject().value("img1v1Url").toString(),
            id,
            int(s.value("duration").toDouble() / 1000), // 毫秒 → 秒
            album.value("name").toString(),
            id, id,
            fee == 1 ? 1 : fee == 8 ? 0 : 3);
    }
    return info;
}

QVariantList NeteaseApi::parseLyrics(const QString &text)
{
    QVariantList result;
    static const QRegularExpression re(QStringLiteral("^(\\d{2}):(\\d{2})[.:](\\d{2,3})$"));
    const QStringList parts = text.split(QLatin1Char('['));
    for (int i = 1; i < parts.size(); ++i) {
        const QString part = parts.at(i);
        const int close = part.indexOf(QLatin1Char(']'));
        if (close < 0)
            continue;
        const QString lyric = part.mid(close + 1).trimmed();
        if (lyric.isEmpty())
            continue;
        const QRegularExpressionMatch m = re.match(part.left(close));
        if (!m.hasMatch())
            continue;
        const QString msStr = m.captured(3);
        const int ms = msStr.size() == 2 ? msStr.toInt() * 10 : msStr.toInt();
        result << QVariantMap{
            {QStringLiteral("time"), m.captured(1).toInt() * 60000
                                         + m.captured(2).toInt() * 1000 + ms},
            {QStringLiteral("text"), lyric},
            {QStringLiteral("info"), QVariant()},
        };
    }
    std::sort(result.begin(), result.end(), [](const QVariant &a, const QVariant &b) {
        return a.toMap().value(QStringLiteral("time")).toInt()
               < b.toMap().value(QStringLiteral("time")).toInt();
    });
    return result;
}

// 搜索（type: 0 歌曲 / 1 歌单 / 2 专辑，对应网易云 web 口 type 1/1000/10）
void NeteaseApi::searchSongs(const QString &keyword, int type, int page, int pageSize)
{
    if (type == 3) { // 网易云无歌词搜索接口，返回空
        emit resultReady(QStringLiteral("searchSongs"), ApiCommon::listResult({}), Source);
        return;
    }
    const int ntype = type == 1 ? 1000 : type == 2 ? 10 : 1;
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("csrf_token"), QString());
    q.addQueryItem(QStringLiteral("s"), keyword);
    q.addQueryItem(QStringLiteral("type"), QString::number(ntype));
    q.addQueryItem(QStringLiteral("offset"), QString::number((page - 1) * pageSize));
    q.addQueryItem(QStringLiteral("total"), QStringLiteral("true"));
    q.addQueryItem(QStringLiteral("limit"), QString::number(pageSize));

    QUrl url(QStringLiteral(kBase) + QStringLiteral("/api/search/get/web"));
    url.setQuery(q);
    get(url.toString(), [this, ntype](const QJsonObject &json) {
        const QJsonObject result = json.value(QStringLiteral("result")).toObject();
        QVariantList info;
        if (ntype == 1) { // 歌曲
            info = parseSongList(result.value(QStringLiteral("songs")).toArray());
        } else if (ntype == 1000) { // 歌单
            for (const QJsonValue &v : result.value(QStringLiteral("playlists")).toArray()) {
                const QJsonObject p = v.toObject();
                const QJsonObject creator = p.value(QStringLiteral("creator")).toObject();
                info << ApiCommon::song(
                    p.value(QStringLiteral("name")).toString(),
                    creator.value(QStringLiteral("nickname")).toString(),
                    p.value(QStringLiteral("coverImgUrl")).toString(),
                    songId(p.value(QStringLiteral("id"))),
                    p.value(QStringLiteral("trackCount")).toInt(),
                    p.value(QStringLiteral("description")).toString(),
                    QString(), QString(), 0,
                    qint64(p.value(QStringLiteral("playCount")).toDouble()));
            }
        } else if (ntype == 10) { // 专辑
            for (const QJsonValue &v : result.value(QStringLiteral("albums")).toArray()) {
                const QJsonObject a = v.toObject();
                info << ApiCommon::song(
                    a.value(QStringLiteral("name")).toString(),
                    a.value(QStringLiteral("artist")).toObject()
                        .value(QStringLiteral("name")).toString(),
                    a.value(QStringLiteral("picUrl")).toString(),
                    songId(a.value(QStringLiteral("id"))),
                    0, a.value(QStringLiteral("name")).toString());
            }
        }
        emit resultReady(QStringLiteral("searchSongs"),
                         ApiCommon::listResult(info), Source);
    });
}

// 歌单分类
void NeteaseApi::getPlaylistMenu(int type)
{
    Q_UNUSED(type);
    get(QStringLiteral(kBase) + QStringLiteral("/api/playlist/catlist"),
        [this](const QJsonObject &json) {
            QVariantList info;
            const QJsonObject categories = json.value(QStringLiteral("categories")).toObject();
            for (const QJsonValue &v : json.value(QStringLiteral("sub")).toArray()) {
                const QJsonObject s = v.toObject();
                info << QVariantMap{
                    {QStringLiteral("title"), s.value(QStringLiteral("name")).toString()},
                    {QStringLiteral("id"), s.value(QStringLiteral("id")).toVariant().toLongLong()},
                    {QStringLiteral("category"), categories.value(s.value(QStringLiteral("category")).toString())
                                                     .toObject().value(QStringLiteral("name")).toString()},
                };
            }
            emit resultReady(QStringLiteral("getPlaylistMenu"),
                             ApiCommon::listResult(info), Source);
        });
}

// 分类 tag 信息（网易云 catlist 的 id 就是 tag，直接透传）
void NeteaseApi::getMenuInfo(const QString &id)
{
    QVariantMap data;
    data.insert(QStringLiteral("special_tag_id"), id);
    data.insert(QStringLiteral("id"), id);
    data.insert(QStringLiteral("name"), QString());
    emit resultReady(QStringLiteral("getMenuInfo"), data, Source);
}

// 分类下歌单列表
void NeteaseApi::getMusicPlaylists(const QString &tagid, int page, int pageSize)
{
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("cat"), tagid);
    q.addQueryItem(QStringLiteral("order"), QStringLiteral("hot"));
    q.addQueryItem(QStringLiteral("offset"), QString::number((page - 1) * pageSize));
    q.addQueryItem(QStringLiteral("limit"), QString::number(pageSize));

    QUrl url(QStringLiteral(kBase) + QStringLiteral("/api/playlist/list"));
    url.setQuery(q);
    get(url.toString(), [this](const QJsonObject &json) {
        QVariantList info;
        for (const QJsonValue &v : json.value(QStringLiteral("playlists")).toArray()) {
            const QJsonObject p = v.toObject();
            const QJsonObject creator = p.value(QStringLiteral("creator")).toObject();
            info << ApiCommon::song(
                p.value(QStringLiteral("name")).toString(),
                creator.value(QStringLiteral("nickname")).toString(),
                p.value(QStringLiteral("coverImgUrl")).toString(),
                songId(p.value(QStringLiteral("id"))),
                p.value(QStringLiteral("trackCount")).toInt(),
                p.value(QStringLiteral("description")).toString());
        }
        emit resultReady(QStringLiteral("getMusicPlaylists"),
                         ApiCommon::listResult(info), Source);
    });
}

// 歌单内歌曲
void NeteaseApi::getPlaylistSongs(const QString &listid, int page, int pageSize)
{
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("id"), listid);
    q.addQueryItem(QStringLiteral("limit"), QString::number(pageSize));
    q.addQueryItem(QStringLiteral("offset"), QString::number((page - 1) * pageSize));

    QUrl url(QStringLiteral(kBase) + QStringLiteral("/api/v6/playlist/detail"));
    url.setQuery(q);
    get(url.toString(), [this](const QJsonObject &json) {
        const QJsonArray tracks = json.value(QStringLiteral("playlist"))
                                      .toObject().value(QStringLiteral("tracks")).toArray();
        emit resultReady(QStringLiteral("getPlaylistSongs"),
                         ApiCommon::listResult(parseSongList(tracks)), Source);
    });
}

// 推荐歌曲
void NeteaseApi::getRecommendSongs(int page, int pageSize)
{
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("limit"), QString::number(pageSize));
    q.addQueryItem(QStringLiteral("offset"), QString::number((page - 1) * pageSize));

    QUrl url(QStringLiteral(kBase) + QStringLiteral("/api/personalized/newsong"));
    url.setQuery(q);
    get(url.toString(), [this](const QJsonObject &json) {
        QVariantList info;
        for (const QJsonValue &v : json.value(QStringLiteral("result")).toArray()) {
            const QJsonObject song = v.toObject().value(QStringLiteral("song")).toObject();
            const QJsonArray artists = song.value(QStringLiteral("artists")).toArray();
            const QJsonObject album = song.value(QStringLiteral("album")).toObject();
            const QString id = songId(song.value(QStringLiteral("id")));
            info << ApiCommon::song(
                song.value(QStringLiteral("name")).toString(),
                artists.isEmpty() ? QString()
                                  : artists.first().toObject().value(QStringLiteral("name")).toString(),
                album.value(QStringLiteral("picUrl")).toString(),
                id,
                int(song.value(QStringLiteral("duration")).toDouble() / 1000),
                album.value(QStringLiteral("name")).toString(),
                id, id,
                song.value(QStringLiteral("pay_type")).toInt(1));
        }
        emit resultReady(QStringLiteral("getRecommendSongs"),
                         ApiCommon::listResult(info), Source);
    });
}

// 热门歌单分类
void NeteaseApi::getHotPlaylistMenu(int type)
{
    Q_UNUSED(type);
    get(QStringLiteral(kBase) + QStringLiteral("/api/playlist/hot"),
        [this](const QJsonObject &json) {
            QVariantList info;
            for (const QJsonValue &v : json.value(QStringLiteral("tags")).toArray()) {
                const QJsonObject t = v.toObject();
                info << QVariantMap{
                    {QStringLiteral("title"), t.value(QStringLiteral("name")).toString()},
                    {QStringLiteral("id"), t.value(QStringLiteral("id")).toVariant().toLongLong()},
                };
            }
            emit resultReady(QStringLiteral("getHotPlaylistMenu"),
                             ApiCommon::listResult(info), Source);
        });
}

// 热门歌单
void NeteaseApi::getHotPlaylists(int page, int pageSize)
{
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("cat"), QStringLiteral("'全部'"));
    q.addQueryItem(QStringLiteral("limit"), QString::number(pageSize));
    q.addQueryItem(QStringLiteral("offset"), QString::number((page - 1) * pageSize));

    QUrl url(QStringLiteral(kBase) + QStringLiteral("/api/playlist/list"));
    url.setQuery(q);
    get(url.toString(), [this](const QJsonObject &json) {
        QVariantList info;
        for (const QJsonValue &v : json.value(QStringLiteral("playlists")).toArray()) {
            const QJsonObject p = v.toObject();
            const QJsonObject creator = p.value(QStringLiteral("creator")).toObject();
            info << ApiCommon::song(
                p.value(QStringLiteral("name")).toString(),
                creator.value(QStringLiteral("nickname")).toString(),
                p.value(QStringLiteral("coverImgUrl")).toString(),
                songId(p.value(QStringLiteral("id"))),
                p.value(QStringLiteral("trackCount")).toInt(),
                p.value(QStringLiteral("description")).toString(),
                QString(), QString(), 0,
                qint64(p.value(QStringLiteral("playCount")).toDouble()));
        }
        emit resultReady(QStringLiteral("getHotPlaylists"),
                         ApiCommon::listResult(info), Source);
    });
}

// 新歌（type: 1 华语 / 2 欧美 / 3 日韩 / 4 韩国，映射到网易云 areaId）
void NeteaseApi::getNewSongs(int type, int page, int pageSize)
{
    int ntype = 0;
    switch (type) {
    case 1: ntype = 7; break;
    case 2: ntype = 96; break;
    case 3: ntype = 8; break;
    case 4: ntype = 16; break;
    default: ntype = 8; break;
    }
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("areaId"), QString::number(ntype));
    q.addQueryItem(QStringLiteral("limit"), QString::number(pageSize));
    q.addQueryItem(QStringLiteral("offset"), QString::number((page - 1) * pageSize));

    QUrl url(QStringLiteral(kBase) + QStringLiteral("/api/v1/discovery/new/songs"));
    url.setQuery(q);
    get(url.toString(), [this](const QJsonObject &json) {
        emit resultReady(QStringLiteral("getNewSongs"),
                         ApiCommon::listResult(parseSongList(json.value(QStringLiteral("data")).toArray())),
                         Source);
    });
}

// 榜单列表（/api/toplist）
void NeteaseApi::getAllToplist()
{
    get(QStringLiteral(kBase) + QStringLiteral("/api/toplist"),
        [this](const QJsonObject &json) {
            QVariantList info;
            for (const QJsonValue &v : json.value(QStringLiteral("list")).toArray()) {
                const QJsonObject t = v.toObject();
                info << ApiCommon::song(
                    t.value(QStringLiteral("name")).toString(),
                    t.value(QStringLiteral("updateFrequency")).toString(),
                    t.value(QStringLiteral("coverImgUrl")).toString(),
                    songId(t.value(QStringLiteral("id"))),
                    0, t.value(QStringLiteral("description")).toString());
            }
            emit resultReady(QStringLiteral("getAllToplist"),
                             ApiCommon::listResult(info), Source);
        });
}

// 榜单歌曲（网易云榜单本质是歌单，用歌单详情拉取）
void NeteaseApi::getMusicToplist(int rankid)
{
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("id"), QString::number(rankid));
    q.addQueryItem(QStringLiteral("limit"), QStringLiteral("100"));
    q.addQueryItem(QStringLiteral("offset"), QStringLiteral("0"));

    QUrl url(QStringLiteral(kBase) + QStringLiteral("/api/v6/playlist/detail"));
    url.setQuery(q);
    get(url.toString(), [this](const QJsonObject &json) {
        const QJsonArray tracks = json.value(QStringLiteral("playlist"))
                                      .toObject().value(QStringLiteral("tracks")).toArray();
        emit resultReady(QStringLiteral("getMusicToplist"),
                         ApiCommon::listResult(parseSongList(tracks)), Source);
    });
}

// 热门歌手（/api/top/artists）
void NeteaseApi::getHotSingers(int page, int pageSize)
{
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("offset"), QString::number((page - 1) * pageSize));
    q.addQueryItem(QStringLiteral("limit"), QString::number(pageSize));

    QUrl url(QStringLiteral(kBase) + QStringLiteral("/api/top/artists"));
    url.setQuery(q);
    get(url.toString(), [this](const QJsonObject &json) {
        QVariantList info;
        for (const QJsonValue &v : json.value(QStringLiteral("artists")).toArray()) {
            const QJsonObject a = v.toObject();
            info << ApiCommon::song(
                a.value(QStringLiteral("name")).toString(),
                QString(),
                a.value(QStringLiteral("picUrl")).toString(),
                songId(a.value(QStringLiteral("id"))));
        }
        emit resultReady(QStringLiteral("getHotSingers"),
                         ApiCommon::listResult(info), Source);
    });
}

// 歌手歌曲（/api/artist/top/song）
void NeteaseApi::getSingerSongs(const QString &singerid, int page, int pageSize)
{
    Q_UNUSED(page);
    Q_UNUSED(pageSize);
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("id"), singerid);

    QUrl url(QStringLiteral(kBase) + QStringLiteral("/api/artist/top/song"));
    url.setQuery(q);
    get(url.toString(), [this](const QJsonObject &json) {
        const QJsonArray songs = json.value(QStringLiteral("songs")).toArray();
        emit resultReady(QStringLiteral("getSingerSongs"),
                         ApiCommon::listResult(parseSongList(songs)), Source);
    });
}

// 歌曲播放信息（type: 0 播放 / 1 下载）
void NeteaseApi::getMusicInfo(const QString &hash, int type)
{
    get(QStringLiteral(kBase) + QStringLiteral("/api/song/detail/?ids=[") + hash + QStringLiteral("]"),
        [this, hash, type](const QJsonObject &json) {
            const QJsonArray songs = json.value(QStringLiteral("songs")).toArray();
            const QJsonObject s = songs.isEmpty() ? QJsonObject() : songs.first().toObject();
            const QString playUrl = QStringLiteral("http://music.163.com/song/media/outer/url?id=")
                                    + hash + QStringLiteral(".mp3");

            // 与 JS 版一致：兼容 ar/al（新接口）与 artists/album（老接口）两种字段名
            QString author;
            const QJsonArray ar = s.value(QStringLiteral("ar")).toArray();
            const QJsonArray artists = s.value(QStringLiteral("artists")).toArray();
            if (!ar.isEmpty())
                author = ar.first().toObject().value(QStringLiteral("name")).toString();
            if (author.isEmpty() && !artists.isEmpty())
                author = artists.first().toObject().value(QStringLiteral("name")).toString();

            QString singerImg;
            if (!ar.isEmpty())
                singerImg = ar.first().toObject().value(QStringLiteral("img1v1Url")).toString();
            if (singerImg.isEmpty() && !artists.isEmpty())
                singerImg = artists.first().toObject().value(QStringLiteral("img1v1Url")).toString();
            if (singerImg.isEmpty() && !artists.isEmpty())
                singerImg = artists.first().toObject().value(QStringLiteral("picUrl")).toString();

            QString albumImg;
            const QJsonObject al = s.value(QStringLiteral("al")).toObject();
            if (al.contains(QStringLiteral("picUrl")))
                albumImg = al.value(QStringLiteral("picUrl")).toString();
            else
                albumImg = s.value(QStringLiteral("album")).toObject()
                               .value(QStringLiteral("picUrl")).toString();

            QVariantMap data;
            data.insert(QStringLiteral("backup_url"), playUrl);
            data.insert(QStringLiteral("url"), playUrl);
            data.insert(QStringLiteral("songName"), s.value(QStringLiteral("name")).toString());
            data.insert(QStringLiteral("author_name"), author);
            data.insert(QStringLiteral("singer_img"), singerImg);
            data.insert(QStringLiteral("album_img"), albumImg);
            data.insert(QStringLiteral("timeLength"),
                        int(s.value(QStringLiteral("duration")).toDouble() / 1000));
            data.insert(QStringLiteral("fileName"),
                        s.value(QStringLiteral("name")).toString(QStringLiteral("unknown"))
                        + QStringLiteral(".mp3"));
            data.insert(QStringLiteral("hash"), hash);
            data.insert(QStringLiteral("type"), type);
            emit resultReady(QStringLiteral("getMusicInfo"), data, Source);
        });
}

// 歌词（LRC + 翻译）
void NeteaseApi::getLyricInfo(const QString &hash, int duration)
{
    Q_UNUSED(duration);
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("id"), hash);
    q.addQueryItem(QStringLiteral("cp"), QStringLiteral("false"));
    q.addQueryItem(QStringLiteral("tv"), QStringLiteral("0"));
    q.addQueryItem(QStringLiteral("lv"), QStringLiteral("0"));
    q.addQueryItem(QStringLiteral("rv"), QStringLiteral("0"));
    q.addQueryItem(QStringLiteral("kv"), QStringLiteral("0"));
    q.addQueryItem(QStringLiteral("yv"), QStringLiteral("0"));
    q.addQueryItem(QStringLiteral("ytv"), QStringLiteral("0"));
    q.addQueryItem(QStringLiteral("yrv"), QStringLiteral("0"));

    QUrl url(QStringLiteral(kBase) + QStringLiteral("/api/song/lyric/v1"));
    url.setQuery(q);
    get(url.toString(), [this](const QJsonObject &json) {
        const QString lrc = json.value(QStringLiteral("lrc"))
                                .toObject().value(QStringLiteral("lyric")).toString();
        const QVariantList info = parseLyrics(lrc);

        // 翻译行按同时间对齐到原文行
        QVariantList translate;
        const QString trc = json.value(QStringLiteral("tlyric"))
                                .toObject().value(QStringLiteral("lyric")).toString();
        if (!trc.isEmpty()) {
            const QVariantList trans = parseLyrics(trc);
            for (int i = 0; i < info.size(); ++i) {
                QString text;
                const int t = info.at(i).toMap().value(QStringLiteral("time")).toInt();
                for (int j = i; j >= 0; --j) {
                    if (j < trans.size()
                        && trans.at(j).toMap().value(QStringLiteral("time")).toInt() == t) {
                        text = trans.at(j).toMap().value(QStringLiteral("text")).toString();
                        break;
                    }
                }
                translate << text;
            }
        }
        QVariantMap data;
        data.insert(QStringLiteral("info"), info);
        data.insert(QStringLiteral("translate"), translate);
        emit resultReady(QStringLiteral("getLyricInfo"), data, Source);
    });
}
