// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#include "QQMusicApi.h"

#include "ApiCommon.h"
#include "ApiHttp.h"

#include <QByteArray>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QList>
#include <QNetworkAccessManager>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QStringList>
#include <QUrlQuery>

#include <algorithm>
#include <utility>

namespace {

const QByteArray kUa = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
                       "(KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36";
const QByteArray kReferer = "https://y.qq.com/";
const QString kApi = QStringLiteral("https://u.y.qq.com/cgi-bin/musicu.fcg");

QString pickString(const QJsonObject &object, std::initializer_list<const char *> keys)
{
    for (const char *key : keys) {
        const QVariant value = object.value(QLatin1String(key)).toVariant();
        const QString text = value.toString();
        if (!text.isEmpty())
            return text;
    }
    return {};
}

int pickInt(const QJsonObject &object, std::initializer_list<const char *> keys, int fallback = 0)
{
    for (const char *key : keys) {
        const QJsonValue value = object.value(QLatin1String(key));
        if (!value.isUndefined() && !value.isNull())
            return value.toVariant().toInt();
    }
    return fallback;
}

int payOf(const QJsonObject &song)
{
    const QJsonObject pay = song.value(QStringLiteral("pay")).toObject();
    for (const char *key : {"pay_play", "payplay", "payPlay"}) {
        const QJsonValue value = pay.value(QLatin1String(key));
        if (!value.isUndefined() && !value.isNull())
            return value.toInt();
    }
    return pickInt(song, {"pay_play", "payplay"});
}

int playCountOf(const QJsonObject &song)
{
    return pickInt(song, {"listennum", "listenNum", "playcount", "play_count"});
}

QString coverUrl(const QString &albumMid, int size)
{
    if (albumMid.isEmpty())
        return {};
    return QStringLiteral("https://y.gtimg.cn/music/photo_new/T002R%1x%1M000%2.jpg")
        .arg(size)
        .arg(albumMid);
}

QString httpsUrl(QString url)
{
    if (url.startsWith(QLatin1String("//")))
        return QStringLiteral("https:") + url;
    if (url.startsWith(QLatin1String("http://")))
        url.replace(0, 7, QStringLiteral("https://"));
    return url;
}

QString stripHtml(QString text)
{
    static const QRegularExpression tag(QStringLiteral("<[^>]*>"));
    text.remove(tag);
    text.replace(QLatin1String("&#38;"), QLatin1String("&"));
    text.replace(QLatin1String("&amp;"), QLatin1String("&"));
    return text.trimmed();
}

QVariantList pageOf(const QVariantList &all, int page, int pageSize)
{
    const int start = (qMax(1, page) - 1) * qBound(1, pageSize, 100);
    if (start >= all.size())
        return {};
    return all.mid(start, qBound(1, pageSize, 100));
}

QVariantMap menuItem(const QString &title, int id, const QString &category)
{
    QVariantMap item = ApiCommon::song(title, QString(), QString(), QString::number(id),
                                       0, category);
    item.insert(QStringLiteral("id"), QString::number(id));
    item.insert(QStringLiteral("tagid"), QString::number(id));
    item.insert(QStringLiteral("category"), category);
    return item;
}

QList<QPair<QString, int>> hotCategories()
{
    return {
        {QStringLiteral("全部"), 10000000},
        {QStringLiteral("国语"), 165},
        {QStringLiteral("英语"), 167},
        {QStringLiteral("日语"), 169},
        {QStringLiteral("韩语"), 168},
        {QStringLiteral("流行"), 6},
        {QStringLiteral("电子"), 24},
        {QStringLiteral("ACG"), 39}
    };
}

QVariantList fallbackToplists()
{
    struct Entry { const char *title; int id; const char *group; };
    static const Entry entries[] = {
        {"热歌榜", 26, "巅峰榜"}, {"新歌榜", 27, "巅峰榜"},
        {"飙升榜", 62, "巅峰榜"}, {"巅峰指数榜", 4, "巅峰榜"},
        {"内地榜", 5, "地区榜"}, {"香港地区榜", 59, "地区榜"},
        {"台湾地区榜", 61, "地区榜"}, {"欧美榜", 3, "地区榜"},
        {"韩国榜", 16, "地区榜"}, {"日本榜", 17, "地区榜"}
    };
    QVariantList list;
    for (const Entry &entry : entries) {
        list << ApiCommon::song(QString::fromUtf8(entry.title), QString::fromUtf8(entry.group),
                                QString(), QString::number(entry.id));
    }
    return list;
}

int topIdForNewType(int type)
{
    switch (type) {
    case 1: return 5;
    case 2: return 3;
    case 3:
    case 5: return 17;
    case 4: return 16;
    default: return 27;
    }
}

int singerArea(int area)
{
    switch (area) {
    case 7: return 200;
    case 96: return 5;
    case 8: return 4;
    case 16: return 3;
    default: return area > 0 ? area : -100;
    }
}

} // namespace

QQMusicApi::QQMusicApi(QObject *parent)
    : MusicSource(Source, parent), m_nam(new QNetworkAccessManager(this))
{
}

void QQMusicApi::applyCredentials(const SourceCredentials &credentials)
{
    m_quality = qBound(0, credentials.quality, 3);
}

void QQMusicApi::getJson(const QUrl &url, const JsonCallback &callback)
{
    ApiHttp::getJson(m_nam, this, url, kUa, kReferer, {}, callback);
}

void QQMusicApi::postJson(const QUrl &url, const QJsonObject &body, const JsonCallback &callback)
{
    ApiHttp::postJson(m_nam, this, url, QJsonDocument(body).toJson(QJsonDocument::Compact),
                      kUa, kReferer, {}, callback);
}

void QQMusicApi::emitList(const QString &action, const QVariantList &info, const QJsonObject &json)
{
    QVariantMap result = ApiCommon::listResult(info);
    if (json.contains(QStringLiteral("error")))
        result.insert(QStringLiteral("error"), json.value(QStringLiteral("error")));
    emit resultReady(action, result, Source);
}

QString QQMusicApi::makeSearchId()
{
    return QString::number(QDateTime::currentMSecsSinceEpoch())
           + QString::number(QRandomGenerator::global()->bounded(100000, 999999));
}

QString QQMusicApi::singerNames(const QJsonValue &value) const
{
    if (!value.isArray())
        return value.toString();
    QStringList names;
    for (const QJsonValue &entry : value.toArray()) {
        const QString name = pickString(entry.toObject(), {"name", "title", "singer_name"});
        if (!name.isEmpty())
            names << name;
    }
    return names.join(QLatin1Char('/'));
}

QQMusicApi::SongDetail QQMusicApi::detailFromJson(const QJsonObject &song) const
{
    SongDetail detail;
    detail.name = pickString(song, {"name", "title", "songname", "songName"});
    detail.artist = singerNames(song.value(QStringLiteral("singer")));
    if (detail.artist.isEmpty())
        detail.artist = pickString(song, {"singerName", "singername"});

    const QJsonObject album = song.value(QStringLiteral("album")).toObject();
    detail.albumMid = pickString(album, {"mid"});
    if (detail.albumMid.isEmpty())
        detail.albumMid = pickString(song, {"albummid", "albumMid"});

    detail.mediaMid = song.value(QStringLiteral("file")).toObject()
                          .value(QStringLiteral("media_mid")).toString();
    if (detail.mediaMid.isEmpty())
        detail.mediaMid = pickString(song, {"media_mid", "mediaMid"});

    detail.id = pickInt(song, {"id", "songid", "songId"});
    detail.interval = pickInt(song, {"interval", "duration"});
    if (detail.interval > 10000)
        detail.interval /= 1000;
    detail.paid = payOf(song) > 0;
    return detail;
}

void QQMusicApi::rememberSong(const QJsonObject &song)
{
    const QString mid = pickString(song, {"mid", "songmid", "songMid"});
    if (!mid.isEmpty())
        rememberDetail(mid, detailFromJson(song));
}

void QQMusicApi::rememberDetail(const QString &mid, const SongDetail &detail)
{
    auto it = m_details.find(mid);
    if (it == m_details.end()) {
        if (m_details.size() > 512)
            m_details.clear();
        m_details.insert(mid, detail);
        return;
    }

    if (!detail.name.isEmpty())
        it->name = detail.name;
    if (!detail.artist.isEmpty())
        it->artist = detail.artist;
    if (!detail.albumMid.isEmpty())
        it->albumMid = detail.albumMid;
    if (!detail.mediaMid.isEmpty())
        it->mediaMid = detail.mediaMid;
    if (detail.id > 0)
        it->id = detail.id;
    if (detail.interval > 0)
        it->interval = detail.interval;
    it->paid = it->paid || detail.paid;
}

QVariantMap QQMusicApi::parseSong(const QJsonObject &song, const QString &fallbackArtist)
{
    const QString mid = pickString(song, {"mid", "songmid", "songMid"});
    const QString rawTitle = pickString(song, {"name", "title", "songname", "songName"});
    if (mid.isEmpty() || rawTitle.isEmpty())
        return {};

    const SongDetail detail = detailFromJson(song);
    const QString artist = detail.artist.isEmpty() ? fallbackArtist : detail.artist;
    QString cover = coverUrl(detail.albumMid, 300);
    if (cover.isEmpty())
        cover = httpsUrl(pickString(song, {"pic", "cover"}));

    QString album = pickString(song.value(QStringLiteral("album")).toObject(),
                               {"name", "title"});
    if (album.isEmpty())
        album = pickString(song, {"albumname", "albumName"});
    QVariantMap item = ApiCommon::song(
        stripHtml(rawTitle), stripHtml(artist), cover, mid, detail.interval, stripHtml(album),
        mid, mid, detail.paid ? ApiCommon::kPaytypePaid : ApiCommon::kPaytypeFree,
        playCountOf(song));
    rememberDetail(mid, detail);
    return item;
}

QVariantMap QQMusicApi::parsePlaylist(const QJsonObject &playlist) const
{
    const QString id = pickString(playlist, {"dissid", "id"});
    const QString title = pickString(playlist, {"dissname", "name", "title"});
    if (id.isEmpty() || title.isEmpty())
        return {};

    QString author = pickString(playlist, {"nickname"});
    if (author.isEmpty())
        author = pickString(playlist.value(QStringLiteral("creator")).toObject(), {"name"});
    const QString cover = httpsUrl(pickString(playlist, {"logo", "imgurl", "img"}));
    QVariantMap item = ApiCommon::song(
        stripHtml(title), stripHtml(author), cover, id, 0,
        stripHtml(pickString(playlist, {"description"})), QString(), QString(), 0,
        playCountOf(playlist));
    item.insert(QStringLiteral("id"), id);
    item.insert(QStringLiteral("tagid"), id);
    return item;
}

QVariantMap QQMusicApi::parseAlbum(const QJsonObject &album) const
{
    const QString id = pickString(album, {"albummid", "mid"});
    const QString title = pickString(album, {"name", "title"});
    if (id.isEmpty() || title.isEmpty())
        return {};

    QString singer = pickString(album, {"singer"});
    if (singer.isEmpty())
        singer = singerNames(album.value(QStringLiteral("singer_list")));
    const QString cover = httpsUrl(pickString(album, {"pic", "cover"}));
    QVariantMap item = ApiCommon::song(
        stripHtml(title), stripHtml(singer), cover, id, 0,
        stripHtml(pickString(album, {"description"})), QString(), QString(), 0,
        playCountOf(album));
    item.insert(QStringLiteral("id"), id);
    return item;
}

void QQMusicApi::onSearchSongs(const QString &keyword, int type, int page, int pageSize)
{
    requestSearch(keyword, type, page, pageSize, ApiAction::SearchSongs);
}

void QQMusicApi::onSearchLyrics(const QString &keyword, int page, int pageSize)
{
    requestSearch(keyword, 7, page, pageSize, ApiAction::SearchLyrics);
}

void QQMusicApi::requestSearch(const QString &keyword, int type, int page, int pageSize,
                               const QString &action)
{
    if (keyword.trimmed().isEmpty()) {
        emit resultReady(action, ApiCommon::listResult({}), Source);
        return;
    }

    int searchType = 0;
    if (type == 1)
        searchType = 3;
    else if (type == 2)
        searchType = 2;
    else if (type >= 3)
        searchType = 7;

    const QJsonObject comm{
        {QStringLiteral("ct"), QStringLiteral("11")},
        {QStringLiteral("cv"), QStringLiteral("1003006")},
        {QStringLiteral("v"), QStringLiteral("1003006")},
        {QStringLiteral("uin"), QStringLiteral("0")},
        {QStringLiteral("format"), QStringLiteral("json")},
        {QStringLiteral("platform"), QStringLiteral("android")}
    };
    const QJsonObject param{
        {QStringLiteral("searchid"), makeSearchId()},
        {QStringLiteral("query"), keyword},
        {QStringLiteral("search_type"), searchType},
        {QStringLiteral("num_per_page"), qBound(1, pageSize, 50)},
        {QStringLiteral("page_num"), qMax(1, page)},
        {QStringLiteral("highlight"), true},
        {QStringLiteral("grp"), true}
    };
    const QJsonObject body{
        {QStringLiteral("comm"), comm},
        {QStringLiteral("req"), QJsonObject{
             {QStringLiteral("module"), QStringLiteral("music.search.SearchCgiService")},
             {QStringLiteral("method"), QStringLiteral("DoSearchForQQMusicMobile")},
             {QStringLiteral("param"), param}
         }}
    };

    postJson(QUrl(kApi), body, [this, searchType, action](const QJsonObject &json) {
        QVariantList info;
        const QJsonObject req = json.value(QStringLiteral("req")).toObject();
        if (!json.contains(QStringLiteral("error"))
            && json.value(QStringLiteral("code")).toInt() == 0
            && req.value(QStringLiteral("code")).toInt() == 0) {
            const QJsonObject data = req.value(QStringLiteral("data")).toObject()
                                         .value(QStringLiteral("body")).toObject();
            if (searchType == 3) {
                for (const QJsonValue &entry : data.value(QStringLiteral("item_songlist")).toArray()) {
                    const QVariantMap item = parsePlaylist(entry.toObject());
                    if (!item.isEmpty())
                        info << item;
                }
            } else if (searchType == 2) {
                for (const QJsonValue &entry : data.value(QStringLiteral("item_album")).toArray()) {
                    const QVariantMap item = parseAlbum(entry.toObject());
                    if (!item.isEmpty())
                        info << item;
                }
            } else {
                for (const QJsonValue &entry : data.value(QStringLiteral("item_song")).toArray()) {
                    const QVariantMap item = parseSong(entry.toObject());
                    if (!item.isEmpty())
                        info << item;
                }
            }
        }
        emitList(action, info, json);
    });
}

void QQMusicApi::onPlaylistMenu(int type)
{
    Q_UNUSED(type)

    if (!m_menuCache.isEmpty()) {
        emit resultReady(ApiAction::PlaylistMenu, ApiCommon::listResult(m_menuCache), Source);
        return;
    }

    QUrl url(QStringLiteral("https://c.y.qq.com/splcloud/fcgi-bin/fcg_get_diss_tag_conf.fcg"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("format"), QStringLiteral("json"));
    query.addQueryItem(QStringLiteral("inCharset"), QStringLiteral("utf8"));
    query.addQueryItem(QStringLiteral("outCharset"), QStringLiteral("utf-8"));
    query.addQueryItem(QStringLiteral("notice"), QStringLiteral("0"));
    query.addQueryItem(QStringLiteral("platform"), QStringLiteral("yqq.json"));
    query.addQueryItem(QStringLiteral("needNewCode"), QStringLiteral("0"));
    url.setQuery(query);

    getJson(url, [this](const QJsonObject &json) {
        QVariantList info;
        bool fromNetwork = false;
        for (const QJsonValue &groupValue : json.value(QStringLiteral("data")).toObject()
                                               .value(QStringLiteral("categories")).toArray()) {
            const QJsonObject group = groupValue.toObject();
            const QString category = stripHtml(pickString(group, {"categoryGroupName"}));
            for (const QJsonValue &itemValue : group.value(QStringLiteral("items")).toArray()) {
                const QJsonObject item = itemValue.toObject();
                const int id = pickInt(item, {"categoryId"});
                const QString title = stripHtml(pickString(item, {"categoryName"}));
                if (id > 0 && !title.isEmpty()) {
                    info << menuItem(title, id, category);
                    fromNetwork = true;
                }
            }
        }
        if (info.isEmpty()) {
            for (const auto &entry : hotCategories())
                info << menuItem(entry.first, entry.second, QStringLiteral("热门"));
        } else if (fromNetwork) {
            m_menuCache = info;
        }
        emitList(ApiAction::PlaylistMenu, info, json);
    });
}

void QQMusicApi::onHotPlaylistMenu(int type)
{
    Q_UNUSED(type)

    QVariantList info;
    for (const auto &entry : hotCategories())
        info << menuItem(entry.first, entry.second, QStringLiteral("热门"));
    emit resultReady(ApiAction::HotPlaylistMenu, ApiCommon::listResult(info), Source);
}

void QQMusicApi::onMusicPlaylists(const QString &tagid, int page, int pageSize)
{
    requestPlaylists(tagid, page, pageSize, ApiAction::MusicPlaylists);
}

void QQMusicApi::onCategoryPlaylists(const QString &categoryid, int page, int pageSize)
{
    requestPlaylists(categoryid, page, pageSize, ApiAction::CategoryPlaylists);
}

void QQMusicApi::onHotPlaylists(int page, int pageSize)
{
    requestPlaylists(QStringLiteral("10000000"), page, pageSize, ApiAction::HotPlaylists);
}

void QQMusicApi::requestPlaylists(const QString &categoryId, int page, int pageSize,
                                  const QString &action)
{
    const QString id = categoryId.isEmpty() ? QStringLiteral("10000000") : categoryId;
    const int size = qBound(1, pageSize, 50);
    const int start = (qMax(1, page) - 1) * size;

    QUrl url(QStringLiteral("https://c.y.qq.com/splcloud/fcgi-bin/fcg_get_diss_by_tag.fcg"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("picmid"), QStringLiteral("1"));
    query.addQueryItem(QStringLiteral("rnd"), QString::number(QDateTime::currentMSecsSinceEpoch()));
    query.addQueryItem(QStringLiteral("g_tk"), QStringLiteral("5381"));
    query.addQueryItem(QStringLiteral("format"), QStringLiteral("json"));
    query.addQueryItem(QStringLiteral("inCharset"), QStringLiteral("utf8"));
    query.addQueryItem(QStringLiteral("outCharset"), QStringLiteral("utf-8"));
    query.addQueryItem(QStringLiteral("notice"), QStringLiteral("0"));
    query.addQueryItem(QStringLiteral("platform"), QStringLiteral("yqq.json"));
    query.addQueryItem(QStringLiteral("needNewCode"), QStringLiteral("0"));
    query.addQueryItem(QStringLiteral("categoryId"), id);
    query.addQueryItem(QStringLiteral("sortId"), QStringLiteral("5"));
    query.addQueryItem(QStringLiteral("sin"), QString::number(start));
    query.addQueryItem(QStringLiteral("ein"), QString::number(start + size - 1));
    query.addQueryItem(QStringLiteral("tagId"), QStringLiteral("0"));
    url.setQuery(query);

    getJson(url, [this, action](const QJsonObject &json) {
        QVariantList info;
        for (const QJsonValue &entry : json.value(QStringLiteral("data")).toObject()
                                           .value(QStringLiteral("list")).toArray()) {
            const QVariantMap item = parsePlaylist(entry.toObject());
            if (!item.isEmpty())
                info << item;
        }
        emitList(action, info, json);
    });
}

void QQMusicApi::onPlaylistSongs(const QString &listid, int page, int pageSize)
{
    const QString id = listid.trimmed();
    const int size = qBound(1, pageSize, 50);
    if (id.isEmpty()) {
        emit resultReady(ApiAction::PlaylistSongs, ApiCommon::listResult({}), Source);
        return;
    }

    const auto cached = m_playlistCache.constFind(id);
    if (cached != m_playlistCache.constEnd()) {
        emit resultReady(ApiAction::PlaylistSongs,
                         ApiCommon::listResult(pageOf(cached.value(), page, size)), Source);
        return;
    }

    QUrl url(QStringLiteral("https://i.y.qq.com/qzone-music/fcg-bin/fcg_ucc_getcdinfo_byids_cp.fcg"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("type"), QStringLiteral("1"));
    query.addQueryItem(QStringLiteral("json"), QStringLiteral("1"));
    query.addQueryItem(QStringLiteral("utf8"), QStringLiteral("1"));
    query.addQueryItem(QStringLiteral("onlysong"), QStringLiteral("0"));
    query.addQueryItem(QStringLiteral("nosign"), QStringLiteral("1"));
    query.addQueryItem(QStringLiteral("disstid"), id);
    query.addQueryItem(QStringLiteral("g_tk"), QStringLiteral("5381"));
    query.addQueryItem(QStringLiteral("loginUin"), QStringLiteral("0"));
    query.addQueryItem(QStringLiteral("hostUin"), QStringLiteral("0"));
    query.addQueryItem(QStringLiteral("format"), QStringLiteral("json"));
    query.addQueryItem(QStringLiteral("inCharset"), QStringLiteral("GB2312"));
    query.addQueryItem(QStringLiteral("outCharset"), QStringLiteral("utf-8"));
    query.addQueryItem(QStringLiteral("notice"), QStringLiteral("0"));
    query.addQueryItem(QStringLiteral("platform"), QStringLiteral("yqq"));
    query.addQueryItem(QStringLiteral("needNewCode"), QStringLiteral("0"));
    url.setQuery(query);

    getJson(url, [this, id, page, size](const QJsonObject &json) {
        QVariantList all;
        const QJsonArray cdlist = json.value(QStringLiteral("cdlist")).toArray();
        if (!cdlist.isEmpty()) {
            const QJsonArray songs = cdlist.first().toObject()
                                         .value(QStringLiteral("songlist")).toArray();
            for (const QJsonValue &entry : songs) {
                const QVariantMap item = parseSong(entry.toObject());
                if (!item.isEmpty())
                    all << item;
            }
        }
        if (all.size() <= 2000 && m_playlistCache.size() < 8)
            m_playlistCache.insert(id, all);

        emitList(ApiAction::PlaylistSongs, pageOf(all, page, size), json);
    });
}

void QQMusicApi::requestTopSongs(int topId, int page, int pageSize, const QString &action)
{
    const int size = qBound(1, pageSize, 50);
    const int start = (qMax(1, page) - 1) * size;

    QUrl url(QStringLiteral("https://c.y.qq.com/v8/fcg-bin/fcg_v8_toplist_cp.fcg"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("format"), QStringLiteral("json"));
    query.addQueryItem(QStringLiteral("inCharset"), QStringLiteral("utf8"));
    query.addQueryItem(QStringLiteral("outCharset"), QStringLiteral("utf-8"));
    query.addQueryItem(QStringLiteral("notice"), QStringLiteral("0"));
    query.addQueryItem(QStringLiteral("platform"), QStringLiteral("yqq.json"));
    query.addQueryItem(QStringLiteral("needNewCode"), QStringLiteral("0"));
    query.addQueryItem(QStringLiteral("tpl"), QStringLiteral("3"));
    query.addQueryItem(QStringLiteral("page"), QStringLiteral("detail"));
    query.addQueryItem(QStringLiteral("type"), QStringLiteral("top"));
    query.addQueryItem(QStringLiteral("topid"), QString::number(topId));
    query.addQueryItem(QStringLiteral("song_begin"), QString::number(start));
    query.addQueryItem(QStringLiteral("song_num"), QString::number(size));
    url.setQuery(query);

    getJson(url, [this, action](const QJsonObject &json) {
        QVariantList info;
        for (const QJsonValue &entry : json.value(QStringLiteral("songlist")).toArray()) {
            const QJsonObject wrapper = entry.toObject();
            QJsonObject song = wrapper.value(QStringLiteral("data")).toObject();
            if (song.isEmpty())
                song = wrapper;
            const QVariantMap item = parseSong(song);
            if (!item.isEmpty())
                info << item;
        }
        emitList(action, info, json);
    });
}

void QQMusicApi::onRecommendSongs(int page, int pageSize)
{
    requestTopSongs(26, page, pageSize, ApiAction::RecommendSongs);
}

void QQMusicApi::onNewSongs(int type, int page, int pageSize)
{
    requestTopSongs(topIdForNewType(type), page, pageSize, ApiAction::NewSongs);
}

void QQMusicApi::onMusicToplist(int page, int pageSize, int rankid)
{
    requestTopSongs(rankid > 0 ? rankid : 26, page, pageSize, ApiAction::MusicToplist);
}

void QQMusicApi::onPersonalFm(int page, int pageSize)
{
    requestTopSongs(26, page, pageSize, ApiAction::PersonalFm);
}

void QQMusicApi::onPersonalRadar(int page, int pageSize)
{
    requestTopSongs(26, page, pageSize, ApiAction::PersonalRadar);
}

void QQMusicApi::onAllToplist()
{
    const QJsonObject body{
        {QStringLiteral("comm"), QJsonObject{
             {QStringLiteral("ct"), 24},
             {QStringLiteral("cv"), 0}
         }},
        {QStringLiteral("topList"), QJsonObject{
             {QStringLiteral("module"), QStringLiteral("musicToplist.ToplistInfoServer")},
             {QStringLiteral("method"), QStringLiteral("GetAll")},
             {QStringLiteral("param"), QJsonObject{}}
         }}
    };

    postJson(QUrl(kApi), body, [this](const QJsonObject &json) {
        QVariantList info;
        const QJsonArray groups = json.value(QStringLiteral("topList")).toObject()
                                      .value(QStringLiteral("data")).toObject()
                                      .value(QStringLiteral("group")).toArray();
        for (const QJsonValue &groupValue : groups) {
            const QJsonObject group = groupValue.toObject();
            const QString groupName = stripHtml(pickString(group, {"groupName"}));
            for (const QJsonValue &topValue : group.value(QStringLiteral("toplist")).toArray()) {
                const QJsonObject top = topValue.toObject();
                const int id = pickInt(top, {"topId"});
                const QString title = stripHtml(pickString(top, {"title"}));
                if (id <= 0 || title.isEmpty())
                    continue;
                QString cover = pickString(top, {"headPicUrl", "frontPicUrl", "mbFrontPicUrl",
                                                "mbHeadPicUrl"});
                QVariantMap item = ApiCommon::song(
                    title, groupName, httpsUrl(cover), QString::number(id), 0,
                    stripHtml(pickString(top, {"intro"})), QString(), QString(), 0,
                    playCountOf(top));
                info << item;
            }
        }
        if (info.isEmpty())
            info = fallbackToplists();
        emitList(ApiAction::AllToplist, info, json);
    });
}

void QQMusicApi::onHotSingers(int page, int pageSize)
{
    requestSingerList(-100, page, pageSize, ApiAction::HotSingers);
}

void QQMusicApi::onSingerCategory(int area, int page, int pageSize)
{
    requestSingerList(singerArea(area), page, pageSize, ApiAction::SingerCategory);
}

void QQMusicApi::requestSingerList(int area, int page, int pageSize, const QString &action)
{
    const int size = qBound(1, pageSize, 80);
    const int start = (qMax(1, page) - 1) * size;
    const QJsonObject body{
        {QStringLiteral("comm"), QJsonObject{
             {QStringLiteral("ct"), 24},
             {QStringLiteral("cv"), 0}
         }},
        {QStringLiteral("singerList"), QJsonObject{
             {QStringLiteral("module"), QStringLiteral("Music.SingerListServer")},
             {QStringLiteral("method"), QStringLiteral("get_singer_list")},
             {QStringLiteral("param"), QJsonObject{
                  {QStringLiteral("area"), area},
                  {QStringLiteral("sex"), -100},
                  {QStringLiteral("genre"), -100},
                  {QStringLiteral("index"), -100},
                  {QStringLiteral("sin"), start},
                  {QStringLiteral("cur_page"), qMax(1, page)}
              }}
         }}
    };

    postJson(QUrl(kApi), body, [this, action, size](const QJsonObject &json) {
        QVariantList info;
        const QJsonArray singers = json.value(QStringLiteral("singerList")).toObject()
                                       .value(QStringLiteral("data")).toObject()
                                       .value(QStringLiteral("singerlist")).toArray();
        for (const QJsonValue &entry : singers) {
            const QJsonObject singer = entry.toObject();
            const QString mid = pickString(singer, {"singer_mid", "mid"});
            const QString name = stripHtml(pickString(singer, {"singer_name", "name"}));
            if (mid.isEmpty() || name.isEmpty())
                continue;
            QString pic = httpsUrl(pickString(singer, {"singer_pic", "pic"}));
            if (pic.endsWith(QLatin1String(".webp")))
                pic.replace(pic.size() - 5, 5, QStringLiteral(".jpg"));
            info << ApiCommon::song(name, QString(), pic, mid);
            if (info.size() >= size)
                break;
        }
        emitList(action, info, json);
    });
}

void QQMusicApi::onSingerSongs(const QString &singerid, int page, int pageSize)
{
    const QString mid = singerid.trimmed();
    const int size = qBound(1, pageSize, 50);
    if (mid.isEmpty()) {
        emit resultReady(ApiAction::SingerSongs, ApiCommon::listResult({}), Source);
        return;
    }

    const QJsonObject body{
        {QStringLiteral("comm"), QJsonObject{
             {QStringLiteral("ct"), 24},
             {QStringLiteral("cv"), 0}
         }},
        {QStringLiteral("singer"), QJsonObject{
             {QStringLiteral("module"), QStringLiteral("music.web_singer_info_svr")},
             {QStringLiteral("method"), QStringLiteral("get_singer_detail_info")},
             {QStringLiteral("param"), QJsonObject{
                  {QStringLiteral("sort"), 5},
                  {QStringLiteral("singermid"), mid},
                  {QStringLiteral("sin"), (qMax(1, page) - 1) * size},
                  {QStringLiteral("num"), size}
              }}
         }}
    };

    postJson(QUrl(kApi), body, [this](const QJsonObject &json) {
        QVariantList info;
        for (const QJsonValue &entry : json.value(QStringLiteral("singer")).toObject()
                                           .value(QStringLiteral("data")).toObject()
                                           .value(QStringLiteral("songlist")).toArray()) {
            const QVariantMap item = parseSong(entry.toObject());
            if (!item.isEmpty())
                info << item;
        }
        emitList(ApiAction::SingerSongs, info, json);
    });
}

void QQMusicApi::requestDetail(const QString &mid, const std::function<void(const SongDetail &)> &done)
{
    const auto cached = m_details.constFind(mid);
    if (cached != m_details.constEnd() && cached->id > 0 && !cached->name.isEmpty()
        && !cached->mediaMid.isEmpty()) {
        done(cached.value());
        return;
    }

    QUrl url(QStringLiteral("https://c.y.qq.com/v8/fcg-bin/fcg_play_single_song.fcg"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("songmid"), mid);
    query.addQueryItem(QStringLiteral("platform"), QStringLiteral("yqq"));
    query.addQueryItem(QStringLiteral("format"), QStringLiteral("json"));
    query.addQueryItem(QStringLiteral("inCharset"), QStringLiteral("utf8"));
    query.addQueryItem(QStringLiteral("outCharset"), QStringLiteral("utf-8"));
    query.addQueryItem(QStringLiteral("notice"), QStringLiteral("0"));
    query.addQueryItem(QStringLiteral("needNewCode"), QStringLiteral("0"));
    url.setQuery(query);

    getJson(url, [this, mid, done](const QJsonObject &json) {
        if (json.contains(QStringLiteral("error")) || json.value(QStringLiteral("code")).toInt() != 0) {
            done(m_details.value(mid));
            return;
        }
        const QJsonValue data = json.value(QStringLiteral("data"));
        QJsonObject song;
        if (data.isArray() && !data.toArray().isEmpty())
            song = data.toArray().first().toObject();
        else if (data.isObject())
            song = data.toObject();
        if (song.isEmpty()) {
            done(m_details.value(mid));
            return;
        }

        rememberSong(song);
        SongDetail detail = m_details.value(mid);
        if (detail.mediaMid.isEmpty())
            detail.mediaMid = mid;
        if (detail.name.isEmpty())
            detail.name = mid;
        m_details.insert(mid, detail);
        done(detail);
    });
}

void QQMusicApi::requestPlayUrl(const QString &mid, int type, const SongDetail &detail)
{
    struct Candidate { const char *prefix; const char *ext; };
    QList<Candidate> candidates;
    if (m_quality <= 0) {
        candidates = {{"M500", "mp3"}};
    } else if (m_quality == 1) {
        candidates = {{"M800", "mp3"}, {"M500", "mp3"}};
    } else {
        candidates = {{"F000", "flac"}, {"C400", "m4a"}, {"M800", "mp3"}, {"M500", "mp3"}};
    }

    const QString mediaMid = detail.mediaMid.isEmpty() ? mid : detail.mediaMid;
    QJsonArray filenames;
    QJsonArray songmids;
    QJsonArray songtypes;
    for (const Candidate &candidate : std::as_const(candidates)) {
        filenames.append(QString::fromLatin1(candidate.prefix) + mediaMid
                         + QLatin1Char('.') + QString::fromLatin1(candidate.ext));
        songmids.append(mid);
        songtypes.append(0);
    }

    const QJsonObject body{
        {QStringLiteral("req_1"), QJsonObject{
             {QStringLiteral("module"), QStringLiteral("vkey.GetVkeyServer")},
             {QStringLiteral("method"), QStringLiteral("CgiGetVkey")},
             {QStringLiteral("param"), QJsonObject{
                  {QStringLiteral("filename"), filenames},
                  {QStringLiteral("guid"), QStringLiteral("10000")},
                  {QStringLiteral("songmid"), songmids},
                  {QStringLiteral("songtype"), songtypes},
                  {QStringLiteral("uin"), QStringLiteral("0")},
                  {QStringLiteral("loginflag"), 1},
                  {QStringLiteral("platform"), QStringLiteral("20")}
              }}
         }},
        {QStringLiteral("loginUin"), QStringLiteral("0")},
        {QStringLiteral("comm"), QJsonObject{
             {QStringLiteral("uin"), QStringLiteral("0")},
             {QStringLiteral("format"), QStringLiteral("json")},
             {QStringLiteral("ct"), 24},
             {QStringLiteral("cv"), 0}
         }}
    };

    auto fallback = [this, mid, type, detail](const QString &reason, const QJsonValue &error = {}) {
        QVariantMap data;
        data.insert(QStringLiteral("url"), QString());
        data.insert(QStringLiteral("backup_url"), QString());
        data.insert(QStringLiteral("songName"), detail.name.isEmpty() ? mid : detail.name);
        data.insert(QStringLiteral("author_name"), detail.artist);
        data.insert(QStringLiteral("singer_img"), coverUrl(detail.albumMid, 300));
        data.insert(QStringLiteral("album_img"), coverUrl(detail.albumMid, 500));
        data.insert(QStringLiteral("timeLength"), detail.interval);
        data.insert(QStringLiteral("hash"), mid);
        data.insert(QStringLiteral("type"), type);
        data.insert(QStringLiteral("errReason"), reason);
        if (!error.isUndefined() && !error.isNull())
            data.insert(QStringLiteral("error"), error.toVariant());
        emit resultReady(ApiAction::MusicInfo, data, Source);
    };

    postJson(QUrl(kApi), body, [this, mid, type, detail, candidates, fallback](const QJsonObject &json) {
        const QJsonObject request = json.value(QStringLiteral("req_1")).toObject();
        if (json.contains(QStringLiteral("error")) || request.value(QStringLiteral("code")).toInt() != 0) {
            fallback(detail.paid ? QStringLiteral("vip") : QStringLiteral("unavailable"),
                     json.value(QStringLiteral("error")));
            return;
        }

        const QJsonObject data = request.value(QStringLiteral("data")).toObject();
        const QJsonArray sips = data.value(QStringLiteral("sip")).toArray();
        QString sip = sips.isEmpty() ? QString() : sips.first().toString();
        if (sip.isEmpty())
            sip = QStringLiteral("https://aqqmusic.tc.qq.com/");
        if (!sip.endsWith(QLatin1Char('/')))
            sip += QLatin1Char('/');

        const QJsonArray infos = data.value(QStringLiteral("midurlinfo")).toArray();
        for (int i = 0; i < infos.size(); ++i) {
            const QString purl = infos.at(i).toObject().value(QStringLiteral("purl")).toString();
            if (purl.isEmpty())
                continue;

            QString ext = infos.at(i).toObject().value(QStringLiteral("filename")).toString()
                              .section(QLatin1Char('.'), -1);
            if (ext.isEmpty() && i < candidates.size())
                ext = QString::fromLatin1(candidates.at(i).ext);
            const QString url = purl.startsWith(QLatin1String("http"))
                                    ? httpsUrl(purl) : httpsUrl(sip + purl);

            QVariantMap result;
            result.insert(QStringLiteral("url"), url);
            result.insert(QStringLiteral("backup_url"), url);
            result.insert(QStringLiteral("songName"), detail.name.isEmpty() ? mid : detail.name);
            result.insert(QStringLiteral("author_name"), detail.artist);
            result.insert(QStringLiteral("singer_img"), coverUrl(detail.albumMid, 300));
            result.insert(QStringLiteral("album_img"), coverUrl(detail.albumMid, 500));
            result.insert(QStringLiteral("timeLength"), detail.interval);
            result.insert(QStringLiteral("fileName"),
                          (detail.name.isEmpty() ? mid : detail.name)
                              + QLatin1Char('.') + (ext.isEmpty() ? QStringLiteral("mp3") : ext));
            result.insert(QStringLiteral("hash"), mid);
            result.insert(QStringLiteral("type"), type);
            emit resultReady(ApiAction::MusicInfo, result, Source);
            return;
        }

        fallback(detail.paid ? QStringLiteral("vip") : QStringLiteral("unavailable"));
    });
}

void QQMusicApi::onMusicInfo(const QString &hash, int type)
{
    const QString mid = hash.trimmed();
    if (mid.isEmpty()) {
        QVariantMap data{{QStringLiteral("error"), -1},
                         {QStringLiteral("message"), QStringLiteral("empty song mid")},
                         {QStringLiteral("hash"), mid}, {QStringLiteral("type"), type}};
        emit resultReady(ApiAction::MusicInfo, data, Source);
        return;
    }

    requestDetail(mid, [this, mid, type](const SongDetail &detail) {
        if (detail.name.isEmpty() && detail.mediaMid.isEmpty()) {
            QVariantMap data{{QStringLiteral("error"), -1},
                             {QStringLiteral("message"), QStringLiteral("song detail unavailable")},
                             {QStringLiteral("hash"), mid}, {QStringLiteral("type"), type}};
            emit resultReady(ApiAction::MusicInfo, data, Source);
            return;
        }
        requestPlayUrl(mid, type, detail);
    });
}

void QQMusicApi::onLyricInfo(const QString &hash, int duration)
{
    Q_UNUSED(duration)

    const QString mid = hash.trimmed();
    if (mid.isEmpty()) {
        QVariantMap data{{QStringLiteral("info"), QVariantList{}},
                         {QStringLiteral("translate"), QVariantList{}},
                         {QStringLiteral("hash"), mid}};
        emit resultReady(ApiAction::LyricInfo, data, Source);
        return;
    }

    QUrl url(QStringLiteral("https://c.y.qq.com/lyric/fcgi-bin/fcg_query_lyric_new.fcg"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("songmid"), mid);
    query.addQueryItem(QStringLiteral("g_tk"), QStringLiteral("5381"));
    query.addQueryItem(QStringLiteral("format"), QStringLiteral("json"));
    query.addQueryItem(QStringLiteral("inCharset"), QStringLiteral("utf8"));
    query.addQueryItem(QStringLiteral("outCharset"), QStringLiteral("utf-8"));
    query.addQueryItem(QStringLiteral("nobase64"), QStringLiteral("1"));
    url.setQuery(query);

    getJson(url, [this, mid](const QJsonObject &json) {
        auto plain = [](QString text) {
            if (!text.contains(QLatin1Char('[')) && !text.isEmpty()) {
                const QByteArray decoded = QByteArray::fromBase64(text.toLatin1());
                if (!decoded.isEmpty())
                    text = QString::fromUtf8(decoded);
            }
            return text;
        };

        const QVariantList lyrics = parseLrc(plain(json.value(QStringLiteral("lyric")).toString()));
        const QVariantList translate = alignedTranslations(
            lyrics, plain(json.value(QStringLiteral("trans")).toString()));

        QVariantMap data;
        data.insert(QStringLiteral("info"), lyrics);
        data.insert(QStringLiteral("translate"), translate);
        data.insert(QStringLiteral("hash"), mid);
        if (json.contains(QStringLiteral("error")))
            data.insert(QStringLiteral("error"), json.value(QStringLiteral("error")));
        emit resultReady(ApiAction::LyricInfo, data, Source);
    });
}

void QQMusicApi::onComments(const QString &hash, int page, int pageSize)
{
    const QString mid = hash.trimmed();
    const int size = qBound(1, pageSize, 50);
    if (mid.isEmpty()) {
        QVariantMap result = ApiCommon::listResult({});
        result.insert(QStringLiteral("hasMore"), false);
        emit resultReady(ApiAction::Comments, result, Source);
        return;
    }

    requestDetail(mid, [this, page, size](const SongDetail &detail) {
        if (detail.id <= 0) {
            QVariantMap result = ApiCommon::listResult({});
            result.insert(QStringLiteral("hasMore"), false);
            emit resultReady(ApiAction::Comments, result, Source);
            return;
        }

        QUrl url(QStringLiteral("https://c.y.qq.com/base/fcgi-bin/fcg_global_comment_h5.fcg"));
        QUrlQuery query;
        query.addQueryItem(QStringLiteral("g_tk"), QStringLiteral("5381"));
        query.addQueryItem(QStringLiteral("loginUin"), QStringLiteral("0"));
        query.addQueryItem(QStringLiteral("hostUin"), QStringLiteral("0"));
        query.addQueryItem(QStringLiteral("format"), QStringLiteral("json"));
        query.addQueryItem(QStringLiteral("inCharset"), QStringLiteral("utf8"));
        query.addQueryItem(QStringLiteral("outCharset"), QStringLiteral("utf-8"));
        query.addQueryItem(QStringLiteral("notice"), QStringLiteral("0"));
        query.addQueryItem(QStringLiteral("platform"), QStringLiteral("yqq.json"));
        query.addQueryItem(QStringLiteral("needNewCode"), QStringLiteral("0"));
        query.addQueryItem(QStringLiteral("cid"), QStringLiteral("205360772"));
        query.addQueryItem(QStringLiteral("reqtype"), QStringLiteral("2"));
        query.addQueryItem(QStringLiteral("biztype"), QStringLiteral("1"));
        query.addQueryItem(QStringLiteral("topid"), QString::number(detail.id));
        query.addQueryItem(QStringLiteral("cmd"), QStringLiteral("6"));
        query.addQueryItem(QStringLiteral("needmusiccrit"), QStringLiteral("0"));
        query.addQueryItem(QStringLiteral("pagenum"), QString::number(qMax(0, page - 1)));
        query.addQueryItem(QStringLiteral("pagesize"), QString::number(size));
        url.setQuery(query);

        getJson(url, [this, size](const QJsonObject &json) {
            QVariantList info;
            const QJsonObject comment = json.value(QStringLiteral("comment")).toObject();
            for (const QJsonValue &entry : comment.value(QStringLiteral("commentlist")).toArray()) {
                const QJsonObject item = entry.toObject();
                info << ApiCommon::comment(
                    pickString(item, {"nick", "username"}),
                    httpsUrl(pickString(item, {"avatarurl", "avatar"})),
                    pickString(item, {"rootcommentcontent", "content", "middlecommentcontent"}),
                    pickInt(item, {"time", "commenttime"}),
                    pickInt(item, {"praisenum", "liked"}),
                    pickInt(item, {"replynum", "subcommentnum", "subcommentcount"}));
            }
            QVariantMap result = ApiCommon::listResult(info);
            result.insert(QStringLiteral("hasMore"),
                          info.size() >= size
                              || comment.value(QStringLiteral("morecomment")).toInt() == 1);
            if (json.contains(QStringLiteral("error")))
                result.insert(QStringLiteral("error"), json.value(QStringLiteral("error")));
            emit resultReady(ApiAction::Comments, result, Source);
        });
    });
}

QVariantList QQMusicApi::parseLrc(const QString &text)
{
    static const QRegularExpression timestamp(
        QStringLiteral("\\[(\\d{1,3}):(\\d{1,2})(?:[.:](\\d{1,3}))?\\]"));

    QVariantList result;
    const QStringList lines = text.split(QLatin1Char('\n'));
    for (const QString &raw : lines) {
        const QString line = raw.trimmed();
        if (line.isEmpty())
            continue;

        QList<qlonglong> times;
        int end = 0;
        QRegularExpressionMatchIterator it = timestamp.globalMatch(line);
        while (it.hasNext()) {
            const QRegularExpressionMatch match = it.next();
            int ms = 0;
            const QString fraction = match.captured(3);
            if (fraction.size() == 1)
                ms = fraction.toInt() * 100;
            else if (fraction.size() == 2)
                ms = fraction.toInt() * 10;
            else if (!fraction.isEmpty())
                ms = fraction.left(3).toInt();
            times << (match.captured(1).toLongLong() * 60
                      + match.captured(2).toLongLong()) * 1000 + ms;
            end = match.capturedEnd();
        }
        if (times.isEmpty())
            continue;

        const QString lyric = line.mid(end).trimmed();
        if (lyric.isEmpty())
            continue;
        for (qlonglong time : std::as_const(times)) {
            result << QVariantMap{{QStringLiteral("time"), time},
                                  {QStringLiteral("text"), lyric}};
        }
    }

    std::sort(result.begin(), result.end(), [](const QVariant &a, const QVariant &b) {
        return a.toMap().value(QStringLiteral("time")).toLongLong()
               < b.toMap().value(QStringLiteral("time")).toLongLong();
    });
    return result;
}

QVariantList QQMusicApi::alignedTranslations(const QVariantList &lyrics, const QString &text)
{
    if (text.trimmed().isEmpty() || lyrics.isEmpty())
        return {};

    const QVariantList translations = parseLrc(text);
    if (translations.isEmpty())
        return {};

    QHash<qlonglong, QString> byTime;
    for (const QVariant &entry : translations) {
        const QVariantMap line = entry.toMap();
        byTime.insert(line.value(QStringLiteral("time")).toLongLong(),
                      line.value(QStringLiteral("text")).toString());
    }

    QVariantList result;
    result.reserve(lyrics.size());
    for (const QVariant &entry : lyrics) {
        result << byTime.value(entry.toMap().value(QStringLiteral("time")).toLongLong());
    }
    return result;
}
