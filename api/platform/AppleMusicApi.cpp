// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#include "AppleMusicApi.h"

#include "ApiCommon.h"
#include "ApiHttp.h"

#include <QJsonArray>
#include <QSet>
#include <QUrlQuery>

namespace {

const QByteArray kUa = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
                       "(KHTML, like Gecko) Chrome/124.0 Safari/537.36";
const QByteArray kReferer = "https://music.apple.com/";
const QString kCountry = "tw";
const QString kRss = "https://rss.marketingtools.apple.com/api/v2/us/music/most-played/25/";

QUrlQuery pageQuery(int page, int pageSize)
{
    QUrlQuery query;
    query.addQueryItem("limit", QString::number(qBound(1, pageSize, 200)));
    query.addQueryItem("offset", QString::number((qMax(1, page) - 1) * qBound(1, pageSize, 200)));
    query.addQueryItem("country", kCountry);
    return query;
}

}

AppleMusicApi::AppleMusicApi(QObject *parent)
    : MusicSource(Source, parent), m_nam(new QNetworkAccessManager(this))
{
}

void AppleMusicApi::get(const QUrl &url, const JsonCallback &done)
{
    ApiHttp::getJson(m_nam, this, url, kUa, kReferer, {}, done);
}

QString AppleMusicApi::idOf(const QJsonObject &object, const char *key)
{
    const QJsonValue value = object.value(key);
    return value.isString() ? value.toString() : QString::number(value.toVariant().toLongLong());
}

QString AppleMusicApi::artwork(QString url)
{
    url.replace("100x100bb", "600x600bb");
    return url;
}

QVariantMap AppleMusicApi::track(const QJsonObject &object)
{
    const QString id = idOf(object, "trackId"), title = object.value("trackName").toString();
    if (id.isEmpty() || title.isEmpty()) return {};

    const bool preview = !object.value("previewUrl").toString().isEmpty();
    QVariantMap item = ApiCommon::song(title, object.value("artistName").toString(), artwork(object.value("artworkUrl100").toString()),
                                       id, object.value("trackTimeMillis").toInt() / 1000, object.value("collectionName").toString(),
                                       id, id, preview ? ApiCommon::kPaytypeFree : ApiCommon::kPaytypePaid);
    item.insert("id", id);
    return item;
}

QVariantMap AppleMusicApi::album(const QJsonObject &object)
{
    const QString id = idOf(object, "collectionId"), title = object.value("collectionName").toString();
    if (id.isEmpty() || title.isEmpty()) return {};

    QVariantMap item = ApiCommon::song(title, object.value("artistName").toString(), artwork(object.value("artworkUrl100").toString()),
                                       id, 0, title, id, id, 0, object.value("trackCount").toInt());
    item.insert("id", id);
    item.insert("tagid", id);
    return item;
}

QVariantMap AppleMusicApi::rssItem(const QJsonObject &object, bool asAlbum)
{
    const QString id = idOf(object, "id"), title = object.value("name").toString();
    if (id.isEmpty() || title.isEmpty()) return {};

    const QString artist = object.value("artistName").toString();
    const QString cover = artwork(object.value("artworkUrl100").toString());
    QVariantMap item = asAlbum ? ApiCommon::song(title, artist, cover, id, 0, title, id, id)
                               : ApiCommon::song(title, artist, cover, id);
    item.insert("id", id);
    if (asAlbum) item.insert("tagid", id);
    return item;
}

void AppleMusicApi::search(const QString &term, const QString &entity, int page, int pageSize,
                           const QString &action)
{
    if (term.trimmed().isEmpty()) {
        emit resultReady(action, ApiCommon::listResult({}), Source);
        return;
    }
    QUrl url("https://itunes.apple.com/search");
    QUrlQuery query = pageQuery(page, pageSize);
    query.addQueryItem("term", term);
    query.addQueryItem("media", "music");
    query.addQueryItem("entity", entity);
    url.setQuery(query);
    const bool albums = entity == "album";
    get(url, [this, action, albums](const QJsonObject &json) {
        const QJsonArray results = json.value("results").toArray();
        QVariantList info;
        info.reserve(results.size());
        for (const QJsonValue &value : results) {
            const QVariantMap item = albums ? album(value.toObject()) : track(value.toObject());
            if (!item.isEmpty()) info << item;
        }
        emit resultReady(action, ApiCommon::listResult(info, json), Source);
    });
}

void AppleMusicApi::onSearchSongs(const QString &keyword, int type, int page, int pageSize)
{
    if (type == 3) {
        onSearchLyrics(keyword, page, pageSize);
        return;
    }
    search(keyword, type == 0 ? "song" : "album", page, pageSize, ApiAction::SearchSongs);
}

void AppleMusicApi::onSearchLyrics(const QString &, int, int)
{
    emit resultReady(ApiAction::SearchLyrics, ApiCommon::listResult({}), Source);
}

void AppleMusicApi::onPlaylistMenu(int)
{
    const QVariantList info{
        QVariantMap{{"title", "华语"}, {"id", "华语"}, {"category", "分类"}},
        QVariantMap{{"title", "欧美"}, {"id", "pop"}, {"category", "分类"}},
        QVariantMap{{"title", "日韩"}, {"id", "kpop"}, {"category", "分类"}},
        QVariantMap{{"title", "经典"}, {"id", "classic"}, {"category", "分类"}},
        QVariantMap{{"title", "电子"}, {"id", "electronic"}, {"category", "分类"}}
    };
    emit resultReady(ApiAction::PlaylistMenu, ApiCommon::listResult(info), Source);
}

void AppleMusicApi::onMusicPlaylists(const QString &tagid, int page, int pageSize)
{
    search(tagid.isEmpty() ? "华语" : tagid, "album", page, pageSize, ApiAction::MusicPlaylists);
}

void AppleMusicApi::onCategoryPlaylists(const QString &categoryid, int page, int pageSize)
{
    search(categoryid.isEmpty() ? "华语" : categoryid, "album", page, pageSize, ApiAction::CategoryPlaylists);
}

void AppleMusicApi::lookupTracks(const QString &id, int page, int pageSize, const QString &action)
{
    if (id.isEmpty()) {
        emit resultReady(action, ApiCommon::listResult({}), Source);
        return;
    }
    QUrl url("https://itunes.apple.com/lookup");
    QUrlQuery query = pageQuery(1, 200);
    query.addQueryItem("id", id);
    query.addQueryItem("entity", "song");
    url.setQuery(query);
    get(url, [this, action, page, pageSize](const QJsonObject &json) {
        const int size = qBound(1, pageSize, 50), start = (qMax(1, page) - 1) * size;
        QVariantList info;
        info.reserve(size);
        int valid = 0;
        for (const QJsonValue &value : json.value("results").toArray()) {
            const QVariantMap item = track(value.toObject());
            if (item.isEmpty() || valid++ < start) continue;
            if (info.size() >= size) break;
            info << item;
        }
        emit resultReady(action, ApiCommon::listResult(info, json), Source);
    });
}

void AppleMusicApi::onPlaylistSongs(const QString &listid, int page, int pageSize)
{
    lookupTracks(listid, page, pageSize, ApiAction::PlaylistSongs);
}

void AppleMusicApi::rss(const QString &action, bool albums, int page, int pageSize)
{
    if (page > 1) {
        emit resultReady(action, ApiCommon::listResult({}), Source);
        return;
    }
    const int size = qBound(1, pageSize, 50);
    get(QUrl(kRss + (albums ? "albums" : "songs") + ".json"), [this, action, albums, size](const QJsonObject &json) {
        const QJsonArray results = json.value("feed").toObject().value("results").toArray();
        QVariantList info;
        info.reserve(qMin(size, int(results.size())));
        for (const QJsonValue &value : results) {
            const QVariantMap item = rssItem(value.toObject(), albums);
            if (!item.isEmpty()) info << item;
            if (info.size() >= size) break;
        }
        emit resultReady(action, ApiCommon::listResult(info, json), Source);
    });
}

void AppleMusicApi::onRecommendSongs(int page, int pageSize)
{
    rss(ApiAction::RecommendSongs, false, page, pageSize);
}

void AppleMusicApi::onHotPlaylistMenu(int)
{
    emit resultReady(ApiAction::HotPlaylistMenu,
                     ApiCommon::listResult({QVariantMap{{"title", "热门专辑"}, {"id", "hot"}, {"category", "热门"}}}),
                     Source);
}

void AppleMusicApi::onHotPlaylists(int page, int pageSize)
{
    rss(ApiAction::HotPlaylists, true, page, pageSize);
}

void AppleMusicApi::onNewSongs(int, int page, int pageSize)
{
    rss(ApiAction::NewSongs, false, page, pageSize);
}

void AppleMusicApi::onAllToplist()
{
    const QVariantList info{ApiCommon::song("Apple Music 热门歌曲", "Apple Music", QString(), "0")};
    emit resultReady(ApiAction::AllToplist, ApiCommon::listResult(info), Source);
}

void AppleMusicApi::onMusicToplist(int page, int pageSize, int)
{
    rss(ApiAction::MusicToplist, false, page, pageSize);
}

void AppleMusicApi::onHotSingers(int page, int pageSize)
{
    if (page > 1) {
        emit resultReady(ApiAction::HotSingers, ApiCommon::listResult({}), Source);
        return;
    }
    const int size = qBound(1, pageSize, 50);
    get(QUrl(kRss + "songs.json"), [this, size](const QJsonObject &json) {
        const QJsonArray results = json.value("feed").toObject().value("results").toArray();
        QVariantList info;
        info.reserve(size);
        QSet<QString> seen;
        for (const QJsonValue &value : results) {
            const QJsonObject object = value.toObject();
            const QString id = idOf(object, "artistId"), name = object.value("artistName").toString();
            if (id.isEmpty() || name.isEmpty() || seen.contains(id)) continue;
            seen.insert(id);
            QVariantMap item = ApiCommon::song(name, QString(), artwork(object.value("artworkUrl100").toString()), id);
            item.insert("id", id);
            info << item;
            if (info.size() >= size) break;
        }
        emit resultReady(ApiAction::HotSingers, ApiCommon::listResult(info, json), Source);
    });
}

void AppleMusicApi::onSingerCategory(int, int page, int pageSize)
{
    onHotSingers(page, pageSize);
}

void AppleMusicApi::onSingerSongs(const QString &singerid, int page, int pageSize)
{
    lookupTracks(singerid, page, pageSize, ApiAction::SingerSongs);
}

void AppleMusicApi::onMusicInfo(const QString &hash, int type)
{
    const QString id = hash.trimmed();
    if (id.isEmpty()) {
        emit resultReady(ApiAction::MusicInfo, QVariantMap{{"error", -1}, {"hash", id}, {"type", type}}, Source);
        return;
    }
    QUrl url("https://itunes.apple.com/lookup");
    QUrlQuery query;
    query.addQueryItem("id", id);
    query.addQueryItem("entity", "song");
    query.addQueryItem("country", kCountry);
    url.setQuery(query);
    get(url, [this, id, type](const QJsonObject &json) {
        QJsonObject object;
        for (const QJsonValue &value : json.value("results").toArray()) {
            if (value.toObject().value("wrapperType").toString() == "track") {
                object = value.toObject();
                break;
            }
        }
        const QString preview = object.value("previewUrl").toString();
        const QString title = object.value("trackName").toString();
        if (preview.isEmpty() || title.isEmpty()) {
            emit resultReady(ApiAction::MusicInfo,
                             QVariantMap{{"error", -1}, {"message", "Apple Music 无试听地址"}, {"hash", id}, {"type", type}},
                             Source);
            return;
        }
        const QString cover = artwork(object.value("artworkUrl100").toString());
        QVariantMap data{{"url", preview}, {"backup_url", preview}, {"songName", title},
                         {"author_name", object.value("artistName").toString()},
                         {"singer_img", cover}, {"album_img", cover},
                         {"timeLength", object.value("trackTimeMillis").toInt() / 1000},
                         {"fileName", title + ".m4a"}, {"hash", id}, {"type", type}};
        emit resultReady(ApiAction::MusicInfo, data, Source);
    });
}

void AppleMusicApi::onLyricInfo(const QString &hash, int)
{
    emit resultReady(ApiAction::LyricInfo,
                     QVariantMap{{"info", QVariantList{}}, {"translate", QVariantList{}}, {"hash", hash}},
                     Source);
}

void AppleMusicApi::onComments(const QString &, int, int)
{
    emit resultReady(ApiAction::Comments, ApiCommon::listResult({}, false), Source);
}

void AppleMusicApi::onPersonalFm(int page, int pageSize)
{
    rss(ApiAction::PersonalFm, false, page, pageSize);
}

void AppleMusicApi::onPersonalRadar(int page, int pageSize)
{
    rss(ApiAction::PersonalRadar, false, page, pageSize);
}
