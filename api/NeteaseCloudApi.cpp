// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
#include "NeteaseCloudApi.h"

#include "apihelper.h"   // QCloudMusicApi::ApiHelper
#include "ApiCommon.h"

#include <QRegularExpression>
#include <QThread>
#include <QVariantList>
#include <QVariantMap>
#include <algorithm>

namespace {

constexpr int kSource = 1;

QString songId(const QVariant &v)
{
    return QString::number(v.toLongLong());
}


QVariantList parseSongList(const QVariantList &songs)
{
    QVariantList info;
    for (const QVariant &sv : songs) {
        const QVariantMap s = sv.toMap();
        const QVariantList ar = s.value(QStringLiteral("ar")).toList();
        const QVariantList artists = ar.isEmpty()
            ? s.value(QStringLiteral("artists")).toList() : ar;
        const QVariantMap al = s.value(QStringLiteral("al")).toMap();
        const QVariantMap album = al.isEmpty()
            ? s.value(QStringLiteral("album")).toMap() : al;
        const qint64 fee = s.value(QStringLiteral("privilege")).toMap()
                               .value(QStringLiteral("fee"),
                                      s.value(QStringLiteral("fee"))).toLongLong();
        const QString id = songId(s.value(QStringLiteral("id")));
        const QString artist = artists.isEmpty()
            ? QStringLiteral("未知歌手")
            : artists.first().toMap().value(QStringLiteral("name")).toString();
        const qint64 dt = s.value(QStringLiteral("dt")).toLongLong();
        const int duration = dt > 0
            ? int(dt / 1000)
            : int(s.value(QStringLiteral("duration")).toLongLong() / 1000);
        info << ApiCommon::song(
            s.value(QStringLiteral("name")).toString(),
            artist,
            album.value(QStringLiteral("picUrl")).toString(),
            id,
            duration,
            album.value(QStringLiteral("name")).toString(),
            id, id,
            fee == 0 ? 0 : (fee == 1 ? 1 : 3));
    }
    return info;
}

// 解析 LRC 文本为 [{time, text, info}]，与旧 NeteaseApi::parseLyrics 行为一致
QVariantList parseLyrics(const QString &text)
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

// 把 QCloudMusicApi 的歌单对象数组（playlists[]）转换成统一歌单字段
QVariantList parsePlaylists(const QVariantList &playlists)
{
    QVariantList info;
    for (const QVariant &v : playlists) {
        const QVariantMap p = v.toMap();
        const QVariantMap creator = p.value(QStringLiteral("creator")).toMap();
        info << ApiCommon::song(
            p.value(QStringLiteral("name")).toString(),
            creator.value(QStringLiteral("nickname")).toString(),
            p.value(QStringLiteral("coverImgUrl")).toString(),
            songId(p.value(QStringLiteral("id"))),
            p.value(QStringLiteral("trackCount")).toInt(),
            p.value(QStringLiteral("description")).toString(),
            QString(), QString(), 0,
            p.value(QStringLiteral("playCount")).toLongLong());
    }
    return info;
}

} // namespace

// CloudWorker：运行在独立线程，持有 ApiHelper，执行真正的 invoke 并做字段映射
class CloudWorker : public QObject
{
    Q_OBJECT
public:
    ~CloudWorker() { delete m_api; }

public slots:
    void request(const QString &action, const QVariantMap &call)
    {
        if (!m_api)
            m_api = new ApiHelper;

        const QString member = call.value(QStringLiteral("member")).toString();
        const QVariantMap arg = call.value(QStringLiteral("arg")).toMap();
        const QVariantMap raw = m_api->invoke(member, arg);

        QVariant data = mapResult(action, raw, call);
        emit result(action, data, kSource);
    }

    void setCookie(const QString &cookie)
    {
        if (!m_api)
            m_api = new ApiHelper;
        m_api->set_cookie(cookie);
    }

signals:
    void result(const QString &action, const QVariant &data, int source);

private:
    // 把 QCloudMusicApi 原始返回映射为 MusicApiService 期望的 data
    static QVariant mapResult(const QString &action, const QVariantMap &raw,
                              const QVariantMap &call)
    {
        // QCloudMusicApi 的 invoke() 返回 { status, body, cookie }，真正的业务
        // 数据在 body 里（顶层还有 code 等）。这里统一解包，后续一律从 body 读。
        const QVariantMap body = raw.value(QStringLiteral("body")).toMap();
        const QVariantMap &data = body.isEmpty() ? raw : body;
        QVariantList info;

        if (action == QLatin1String("searchSongs")) {
            const QVariantMap result = data.value(QStringLiteral("result")).toMap();
            const int ntype = call.value(QStringLiteral("ntype")).toInt();
            if (ntype == 1) {
                info = parseSongList(result.value(QStringLiteral("songs")).toList());
            } else if (ntype == 1000) {
                info = parsePlaylists(result.value(QStringLiteral("playlists")).toList());
            } else if (ntype == 10) {
                for (const QVariant &v : result.value(QStringLiteral("albums")).toList()) {
                    const QVariantMap a = v.toMap();
                    info << ApiCommon::song(
                        a.value(QStringLiteral("name")).toString(),
                        a.value(QStringLiteral("artist")).toMap().value(QStringLiteral("name")).toString(),
                        a.value(QStringLiteral("picUrl")).toString(),
                        songId(a.value(QStringLiteral("id"))),
                        0, a.value(QStringLiteral("name")).toString());
                }
            }
        }
        else if (action == QLatin1String("getPlaylistMenu")) {
            const QVariantMap categories = data.value(QStringLiteral("categories")).toMap();
            for (const QVariant &v : data.value(QStringLiteral("sub")).toList()) {
                const QVariantMap s = v.toMap();
                info << QVariantMap{
                    {QStringLiteral("title"), s.value(QStringLiteral("name")).toString()},
                    {QStringLiteral("id"), songId(s.value(QStringLiteral("id")))},
                    {QStringLiteral("category"),
                     categories.value(s.value(QStringLiteral("category")).toString()).toString()},
                };
            }
        }
        else if (action == QLatin1String("getMusicPlaylists")
                 || action == QLatin1String("getHotPlaylists")) {
            info = parsePlaylists(data.value(QStringLiteral("playlists")).toList());
        }
        else if (action == QLatin1String("getPlaylistSongs")
                 || action == QLatin1String("getMusicToplist")
                 || action == QLatin1String("getSingerSongs")) {
            // playlist_track_all 返回 songs[]；top_list 返回 playlist.tracks[]；
            // artist_top_song 返回 songs[]。统一兜底三种结构。
            QVariantList songs = data.value(QStringLiteral("songs")).toList();
            if (songs.isEmpty())
                songs = data.value(QStringLiteral("playlist")).toMap()
                            .value(QStringLiteral("tracks")).toList();
            info = parseSongList(songs);
        }
        else if (action == QLatin1String("getPersonalFm")) {
            // personal_fm 返回 data[]（标准歌曲对象，部分场景带 song 包装，统一解包）
            QVariantList songs;
            for (const QVariant &v : data.value(QStringLiteral("data")).toList()) {
                const QVariantMap w = v.toMap();
                songs << (w.contains(QStringLiteral("song"))
                              ? w.value(QStringLiteral("song")) : w);
            }
            info = parseSongList(songs);
        }
        else if (action == QLatin1String("getPersonalRadar")) {
            // recommend_songs 返回 data.dailySongs[]（标准歌曲对象）
            const QVariantList daily = data.value(QStringLiteral("data")).toMap()
                                           .value(QStringLiteral("dailySongs")).toList();
            info = parseSongList(daily);
        }
        else if (action == QLatin1String("getRecommendSongs")) {
            const QVariantList list = data.contains(QStringLiteral("data"))
                ? data.value(QStringLiteral("data")).toList()
                : data.value(QStringLiteral("result")).toList();
            for (const QVariant &v : list) {
                const QVariantMap song = v.toMap().value(QStringLiteral("song")).toMap();
                // personalized_newsong 的内嵌 song 同样用 ar / al / dt
                const QVariantList ar = song.value(QStringLiteral("ar")).toList();
                const QVariantList artists = ar.isEmpty()
                    ? song.value(QStringLiteral("artists")).toList() : ar;
                const QVariantMap al = song.value(QStringLiteral("al")).toMap();
                const QVariantMap album = al.isEmpty()
                    ? song.value(QStringLiteral("album")).toMap() : al;
                const QString id = songId(song.value(QStringLiteral("id")));
                const qint64 dt = song.value(QStringLiteral("dt")).toLongLong();
                const int duration = dt > 0
                    ? int(dt / 1000)
                    : int(song.value(QStringLiteral("duration")).toLongLong() / 1000);
                info << ApiCommon::song(
                    song.value(QStringLiteral("name")).toString(),
                    artists.isEmpty() ? QString()
                                      : artists.first().toMap().value(QStringLiteral("name")).toString(),
                    album.value(QStringLiteral("picUrl")).toString(),
                    id,
                    duration,
                    album.value(QStringLiteral("name")).toString(),
                    id, id,
                    [&]() -> int {
                        bool ok = false;
                        const int v = song.value(QStringLiteral("pay_type")).toInt(&ok);
                        return ok ? v : 1;
                    }());
            }
        }
        else if (action == QLatin1String("getHotPlaylistMenu")) {
            for (const QVariant &v : data.value(QStringLiteral("tags")).toList()) {
                const QVariantMap t = v.toMap();
                // tagid 复用分类名：网易云 top_playlist 的 cat 参数需要分类名，
                // 首页分类卡片点击后直接 getMusicPlaylists(model.tagid)，故 tagid=分类名。
                info << QVariantMap{
                    {QStringLiteral("title"), t.value(QStringLiteral("name")).toString()},
                    {QStringLiteral("id"), songId(t.value(QStringLiteral("id")))},
                    {QStringLiteral("tagid"), t.value(QStringLiteral("name")).toString()},
                };
            }
        }
        else if (action == QLatin1String("getNewSongs")) {
            // top_song 返回 data[]，每个元素是 { alg, id, name, song:{...} } 包装对象，
            // 真正的歌曲在 song 字段里。先解包再交给 parseSongList。
            QVariantList songs;
            for (const QVariant &v : data.value(QStringLiteral("data")).toList()) {
                const QVariantMap w = v.toMap();
                if (w.contains(QStringLiteral("song")))
                    songs << w.value(QStringLiteral("song"));
                else
                    songs << w;
            }
            info = parseSongList(songs);
        }
        else if (action == QLatin1String("getAllToplist")) {
            for (const QVariant &v : data.value(QStringLiteral("list")).toList()) {
                const QVariantMap t = v.toMap();
                info << ApiCommon::song(
                    t.value(QStringLiteral("name")).toString(),
                    t.value(QStringLiteral("updateFrequency")).toString(),
                    t.value(QStringLiteral("coverImgUrl")).toString(),
                    songId(t.value(QStringLiteral("id"))),
                    0, t.value(QStringLiteral("description")).toString());
            }
        }
        else if (action == QLatin1String("getHotSingers")
                 || action == QLatin1String("getSingerCategory")) {
            // top_artists / artist_list 均返回 artists[]（name/picUrl/id）
            for (const QVariant &v : data.value(QStringLiteral("artists")).toList()) {
                const QVariantMap a = v.toMap();
                info << ApiCommon::song(
                    a.value(QStringLiteral("name")).toString(),
                    QString(),
                    a.value(QStringLiteral("picUrl")).toString(),
                    songId(a.value(QStringLiteral("id"))));
            }
        }
        else if (action == QLatin1String("getMusicInfo")) {
            const QString hash = call.value(QStringLiteral("hash")).toString();
            const int type = call.value(QStringLiteral("type")).toInt();
            // 播放地址沿用 163 公开外链（稳定、免加密），元数据来自 QCloudMusicApi
            const QString playUrl = QStringLiteral("http://music.163.com/song/media/outer/url?id=")
                                    + hash + QStringLiteral(".mp3");

            const QVariantList songs = data.value(QStringLiteral("songs")).toList();
            const QVariantMap s = songs.isEmpty() ? QVariantMap() : songs.first().toMap();

            QString author;
            QString singerImg;
            const QVariantList ar = s.value(QStringLiteral("ar")).toList();
            if (!ar.isEmpty()) {
                const QVariantMap a0 = ar.first().toMap();
                author = a0.value(QStringLiteral("name")).toString();
                singerImg = a0.value(QStringLiteral("img1v1Url")).toString();
                if (singerImg.isEmpty())
                    singerImg = a0.value(QStringLiteral("picUrl")).toString();
            }
            const QString albumImg = s.value(QStringLiteral("al")).toMap()
                                         .value(QStringLiteral("picUrl")).toString();

            QVariantMap d;
            d.insert(QStringLiteral("backup_url"), playUrl);
            d.insert(QStringLiteral("url"), playUrl);
            d.insert(QStringLiteral("songName"), s.value(QStringLiteral("name")).toString());
            d.insert(QStringLiteral("author_name"), author);
            d.insert(QStringLiteral("singer_img"), singerImg);
            d.insert(QStringLiteral("album_img"), albumImg);
            const qint64 dt = s.value(QStringLiteral("dt")).toLongLong();
            d.insert(QStringLiteral("timeLength"),
                     dt > 0 ? int(dt / 1000)
                            : int(s.value(QStringLiteral("duration")).toLongLong() / 1000));
            const QString fileNameBase = [&]() -> QString {
                const QString nm = s.value(QStringLiteral("name")).toString();
                return nm.isEmpty() ? QStringLiteral("unknown") : nm;
            }();
            d.insert(QStringLiteral("fileName"), fileNameBase + QStringLiteral(".mp3"));
            d.insert(QStringLiteral("hash"), hash);
            d.insert(QStringLiteral("type"), type);
            return d;
        }
        else if (action == QLatin1String("getLyricInfo")) {
            const QString lrc = data.value(QStringLiteral("lrc"))
                                    .toMap().value(QStringLiteral("lyric")).toString();
            const QVariantList lrcInfo = parseLyrics(lrc);

            QVariantList translate;
            const QString trc = data.value(QStringLiteral("tlyric"))
                                   .toMap().value(QStringLiteral("lyric")).toString();
            if (!trc.isEmpty()) {
                const QVariantList trans = parseLyrics(trc);
                for (int i = 0; i < lrcInfo.size(); ++i) {
                    QString text;
                    const int t = lrcInfo.at(i).toMap().value(QStringLiteral("time")).toInt();
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
            QVariantMap d;
            d.insert(QStringLiteral("info"), lrcInfo);
            d.insert(QStringLiteral("translate"), translate);
            return d;
        }
        else {
            qWarning() << "[NeteaseCloudApi] 未知 action:" << action;
        }

        return ApiCommon::listResult(info);
    }

    ApiHelper *m_api = nullptr;
};

// NeteaseCloudApi
NeteaseCloudApi::NeteaseCloudApi(QObject *parent)
    : QObject(parent)
{
    m_thread = new QThread(this);
    m_worker = new CloudWorker;
    m_worker->moveToThread(m_thread);
    connect(this, &NeteaseCloudApi::runRequest, m_worker, &CloudWorker::request);
    connect(this, &NeteaseCloudApi::setCookieRequest, m_worker, &CloudWorker::setCookie);
    connect(m_worker, &CloudWorker::result, this, &NeteaseCloudApi::onWorkerResult);
    m_thread->start();
}

NeteaseCloudApi::~NeteaseCloudApi()
{
    m_thread->quit();
    m_thread->wait();
    delete m_worker; // 工作线程已结束，安全释放 ApiHelper
}

void NeteaseCloudApi::onWorkerResult(const QString &action, const QVariant &data, int source)
{
    emit resultReady(action, data, source);
}

void NeteaseCloudApi::enqueue(const QString &action, const QVariantMap &call)
{
    emit runRequest(action, call);
}

void NeteaseCloudApi::setCookie(const QString &cookie)
{
    emit setCookieRequest(cookie);
}

void NeteaseCloudApi::searchSongs(const QString &keyword, int type, int page, int pageSize)
{
    if (type == 3) { // 网易云无歌词搜索接口
        emit resultReady(QStringLiteral("searchSongs"),
                         ApiCommon::listResult(QVariantList()), kSource);
        return;
    }
    const int ntype = type == 1 ? 1000 : type == 2 ? 10 : 1; // 1 单曲 / 1000 歌单 / 10 专辑
    QVariantMap arg;
    arg.insert(QStringLiteral("keywords"), keyword);
    arg.insert(QStringLiteral("type"), ntype);
    arg.insert(QStringLiteral("limit"), pageSize);
    arg.insert(QStringLiteral("offset"), (page - 1) * pageSize);
    QVariantMap call;
    call.insert(QStringLiteral("member"), QStringLiteral("cloudsearch"));
    call.insert(QStringLiteral("arg"), arg);
    call.insert(QStringLiteral("ntype"), ntype);
    enqueue(QStringLiteral("searchSongs"), call);
}

void NeteaseCloudApi::getPlaylistMenu(int type)
{
    Q_UNUSED(type);
    QVariantMap call;
    call.insert(QStringLiteral("member"), QStringLiteral("playlist_catlist"));
    call.insert(QStringLiteral("arg"), QVariantMap());
    enqueue(QStringLiteral("getPlaylistMenu"), call);
}

void NeteaseCloudApi::getMenuInfo(const QString &id)
{
    QVariantMap data;
    data.insert(QStringLiteral("special_tag_id"), id);
    data.insert(QStringLiteral("id"), id);
    data.insert(QStringLiteral("name"), QString());
    emit resultReady(QStringLiteral("getMenuInfo"), data, kSource);
}

void NeteaseCloudApi::getMusicPlaylists(const QString &tagid, int page, int pageSize)
{
    QVariantMap arg;
    arg.insert(QStringLiteral("cat"), tagid);
    arg.insert(QStringLiteral("order"), QStringLiteral("hot"));
    arg.insert(QStringLiteral("offset"), (page - 1) * pageSize);
    arg.insert(QStringLiteral("limit"), pageSize);
    QVariantMap call;
    call.insert(QStringLiteral("member"), QStringLiteral("top_playlist"));
    call.insert(QStringLiteral("arg"), arg);
    enqueue(QStringLiteral("getMusicPlaylists"), call);
}

void NeteaseCloudApi::getPlaylistSongs(const QString &listid, int page, int pageSize)
{
    QVariantMap arg;
    arg.insert(QStringLiteral("id"), listid);
    arg.insert(QStringLiteral("limit"), pageSize);
    arg.insert(QStringLiteral("offset"), (page - 1) * pageSize);
    QVariantMap call;
    call.insert(QStringLiteral("member"), QStringLiteral("playlist_track_all"));
    call.insert(QStringLiteral("arg"), arg);
    enqueue(QStringLiteral("getPlaylistSongs"), call);
}

void NeteaseCloudApi::getRecommendSongs(int page, int pageSize)
{
    Q_UNUSED(page);
    QVariantMap arg;
    arg.insert(QStringLiteral("limit"), pageSize);
    QVariantMap call;
    call.insert(QStringLiteral("member"), QStringLiteral("personalized_newsong"));
    call.insert(QStringLiteral("arg"), arg);
    enqueue(QStringLiteral("getRecommendSongs"), call);
}

void NeteaseCloudApi::getHotPlaylistMenu(int type)
{
    Q_UNUSED(type);
    QVariantMap call;
    call.insert(QStringLiteral("member"), QStringLiteral("playlist_hot"));
    call.insert(QStringLiteral("arg"), QVariantMap());
    enqueue(QStringLiteral("getHotPlaylistMenu"), call);
}

void NeteaseCloudApi::getHotPlaylists(int page, int pageSize)
{
    QVariantMap arg;
    arg.insert(QStringLiteral("cat"), QStringLiteral("全部"));
    arg.insert(QStringLiteral("order"), QStringLiteral("hot"));
    arg.insert(QStringLiteral("offset"), (page - 1) * pageSize);
    arg.insert(QStringLiteral("limit"), pageSize);
    QVariantMap call;
    call.insert(QStringLiteral("member"), QStringLiteral("top_playlist"));
    call.insert(QStringLiteral("arg"), arg);
    enqueue(QStringLiteral("getHotPlaylists"), call);
}

void NeteaseCloudApi::getNewSongs(int type, int page, int pageSize)
{
    int ntype = 0; // 全部
    switch (type) {
    case 1: ntype = 7;  break; // 华语
    case 2: ntype = 96; break; // 欧美
    case 3: ntype = 8;  break; // 日本
    case 4: ntype = 16; break; // 韩国
    case 5: ntype = 8;  break; // 日本
    default: ntype = 0;  break;
    }
    QVariantMap arg;
    arg.insert(QStringLiteral("type"), ntype);
    arg.insert(QStringLiteral("limit"), pageSize);
    arg.insert(QStringLiteral("offset"), (page - 1) * pageSize);
    QVariantMap call;
    call.insert(QStringLiteral("member"), QStringLiteral("top_song"));
    call.insert(QStringLiteral("arg"), arg);
    enqueue(QStringLiteral("getNewSongs"), call);
}

void NeteaseCloudApi::getAllToplist()
{
    QVariantMap call;
    call.insert(QStringLiteral("member"), QStringLiteral("toplist"));
    call.insert(QStringLiteral("arg"), QVariantMap());
    enqueue(QStringLiteral("getAllToplist"), call);
}

void NeteaseCloudApi::getMusicToplist(int page, int pageSize, int rankid)
{
    // 榜单 id 本质是歌单 id，用 top_list（playlist/v4/detail）拉取榜单歌曲
    QVariantMap arg;
    arg.insert(QStringLiteral("id"), rankid);
    arg.insert(QStringLiteral("offset"), (page - 1) * pageSize);
    arg.insert(QStringLiteral("limit"), pageSize);
    QVariantMap call;
    call.insert(QStringLiteral("member"), QStringLiteral("top_list"));
    call.insert(QStringLiteral("arg"), arg);
    enqueue(QStringLiteral("getMusicToplist"), call);
}

void NeteaseCloudApi::getHotSingers(int page, int pageSize)
{
    QVariantMap arg;
    arg.insert(QStringLiteral("offset"), (page - 1) * pageSize);
    arg.insert(QStringLiteral("limit"), pageSize);
    QVariantMap call;
    call.insert(QStringLiteral("member"), QStringLiteral("top_artists"));
    call.insert(QStringLiteral("arg"), arg);
    enqueue(QStringLiteral("getHotSingers"), call);
}

void NeteaseCloudApi::getSingerCategory(int area, int page, int pageSize)
{
    // artist_list：area 为地区编码（7 华语 / 96 欧美 / 8 日本 / 16 韩国）
    QVariantMap arg;
    arg.insert(QStringLiteral("offset"), (page - 1) * pageSize);
    arg.insert(QStringLiteral("limit"), pageSize);
    arg.insert(QStringLiteral("type"), QStringLiteral("1"));
    arg.insert(QStringLiteral("area"), area);
    QVariantMap call;
    call.insert(QStringLiteral("member"), QStringLiteral("artist_list"));
    call.insert(QStringLiteral("arg"), arg);
    enqueue(QStringLiteral("getSingerCategory"), call);
}

void NeteaseCloudApi::getSingerSongs(const QString &singerid, int page, int pageSize)
{
    Q_UNUSED(page);
    Q_UNUSED(pageSize);
    QVariantMap arg;
    arg.insert(QStringLiteral("id"), singerid);
    QVariantMap call;
    call.insert(QStringLiteral("member"), QStringLiteral("artist_top_song"));
    call.insert(QStringLiteral("arg"), arg);
    enqueue(QStringLiteral("getSingerSongs"), call);
}

void NeteaseCloudApi::getMusicInfo(const QString &hash, int type)
{
    QVariantMap arg;
    // 注意 ids 必须是字符串（QCloudMusicApi 的 song_detail 内部会 split(",")），
    // 之前传 QVariantList 会被 toString() 转成空串，导致详情拉取返回空。
    arg.insert(QStringLiteral("ids"), hash);
    arg.insert(QStringLiteral("id"), hash);
    QVariantMap call;
    call.insert(QStringLiteral("member"), QStringLiteral("song_detail"));
    call.insert(QStringLiteral("arg"), arg);
    call.insert(QStringLiteral("hash"), hash);
    call.insert(QStringLiteral("type"), type);
    enqueue(QStringLiteral("getMusicInfo"), call);
}

void NeteaseCloudApi::getLyricInfo(const QString &hash, int duration)
{
    Q_UNUSED(duration);
    QVariantMap arg;
    arg.insert(QStringLiteral("id"), hash);
    QVariantMap call;
    call.insert(QStringLiteral("member"), QStringLiteral("lyric"));
    call.insert(QStringLiteral("arg"), arg);
    enqueue(QStringLiteral("getLyricInfo"), call);
}

void NeteaseCloudApi::getPersonalFm(int page, int pageSize)
{
    // personal_fm（私人漫游）本身不分页，每次返回一批，忽略分页参数
    Q_UNUSED(page);
    Q_UNUSED(pageSize);
    QVariantMap call;
    call.insert(QStringLiteral("member"), QStringLiteral("personal_fm"));
    call.insert(QStringLiteral("arg"), QVariantMap());
    enqueue(QStringLiteral("getPersonalFm"), call);
}

void NeteaseCloudApi::getPersonalRadar(int page, int pageSize)
{
    Q_UNUSED(pageSize);
    // recommend_songs（每日推荐）不支持分页，仅第一页有数据；后续页返回空避免重复追加
    if (page > 1) {
        emit resultReady(QStringLiteral("getPersonalRadar"),
                         ApiCommon::listResult(QVariantList()), kSource);
        return;
    }
    QVariantMap call;
    call.insert(QStringLiteral("member"), QStringLiteral("recommend_songs"));
    call.insert(QStringLiteral("arg"), QVariantMap());
    enqueue(QStringLiteral("getPersonalRadar"), call);
}

#include "NeteaseCloudApi.moc"
