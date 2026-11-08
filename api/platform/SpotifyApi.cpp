// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#include "SpotifyApi.h"

#include "ApiCommon.h"
#include "ApiHttp.h"

#include <QDate>
#include <QDateTime>
#include <QJsonArray>
#include <QStringList>
#include <QUrl>

namespace {

const QByteArray kUa = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
                       "(KHTML, like Gecko) Chrome/124.0 Safari/537.36";
const QByteArray kReferer = "https://open.spotify.com/";
const QString kApi = "https://api.spotify.com";
const QStringList kMenuTitles{"流行", "华语", "电子", "摇滚", "爵士"};
const QStringList kMenuIds{"pop", "华语", "electronic", "rock", "jazz"};
const QStringList kToplistNames{"Spotify Global Top 50", "Spotify US Top 50", "Spotify Viral 50"};
const QStringList kToplists{"37i9dQZEVXbMDoHDwVN2tF", "37i9dQZEVXbLRQDuF5jeBp", "37i9dQZEVXbLiRSasKsNU9"};

QString artistNames(const QJsonArray &artists)
{
    QStringList names;
    names.reserve(artists.size());
    for (const QJsonValue &value : artists) names << value.toObject().value("name").toString();
    return names.join(" / ");
}

QUrlQuery pageQuery(int page, int pageSize)
{
    QUrlQuery query;
    const int size = qBound(1, pageSize, 50);
    query.addQueryItem("limit", QString::number(size));
    query.addQueryItem("offset", QString::number((qMax(1, page) - 1) * size));
    query.addQueryItem("market", "US");
    return query;
}

}

SpotifyApi::SpotifyApi(QObject *parent)
    : MusicSource(Source, parent), m_nam(new QNetworkAccessManager(this))
{
}

void SpotifyApi::applyCredentials(const SourceCredentials &credentials)
{
    if (m_cookie == credentials.cookie && m_accessToken == credentials.accessToken) return;
    m_cookie = credentials.cookie;
    m_accessToken = credentials.accessToken;
    m_token.clear();
    m_expiry = 0;
}

void SpotifyApi::ensureToken(const TokenCallback &done)
{
    if (!m_token.isEmpty() && QDateTime::currentMSecsSinceEpoch() < m_expiry) {
        done(true);
        return;
    }
    if (!m_accessToken.isEmpty()) {
        m_token = m_accessToken;
        m_expiry = QDateTime::currentMSecsSinceEpoch() + 3000000;
        done(true);
        return;
    }
    if (m_cookie.isEmpty()) {
        done(false);
        return;
    }
    m_waiters.append(done);
    if (m_tokenPending) return;

    m_tokenPending = true;
    const QUrl url("https://open.spotify.com/get_access_token?reason=transport&productType=web_player");
    ApiHttp::getJson(m_nam, this, url, kUa, kReferer, m_cookie.toUtf8(), [this](const QJsonObject &json) {
        m_tokenPending = false;
        m_token = json.value("accessToken").toString();
        const qint64 expires = json.value("accessTokenExpirationTimestampMs").toVariant().toLongLong();
        m_expiry = m_token.isEmpty() ? 0 : expires > 0 ? expires - 60000 : QDateTime::currentMSecsSinceEpoch() + 3000000;
        auto waiters = std::move(m_waiters);
        m_waiters.clear();
        for (const TokenCallback &callback : waiters) callback(!m_token.isEmpty());
    }, {{"Origin", "https://open.spotify.com"}, {"Accept", "application/json"}});
}

void SpotifyApi::get(const QString &path, const QUrlQuery &query, const JsonCallback &done)
{
    ensureToken([this, path, query, done](bool ok) {
        if (!ok) {
            done(QJsonObject{{"error", -1}, {"message", "Spotify 登录态不可用，请重新登录"}});
            return;
        }
        QUrl url(kApi + path);
        if (!query.isEmpty()) url.setQuery(query);
        ApiHttp::getJson(m_nam, this, url, kUa, kReferer, {}, [this, done](const QJsonObject &json) {
            if (json.value("error").toObject().value("status").toInt() == 401) {
                m_expiry = 0;
                m_accessToken.clear();
            }
            done(json);
        }, {{"Authorization", "Bearer " + m_token.toUtf8()}, {"Origin", "https://open.spotify.com"}, {"Accept", "application/json"}});
    });
}

QString SpotifyApi::imageOf(const QJsonObject &object)
{
    const QJsonArray images = object.value("images").toArray();
    return images.isEmpty() ? QString() : images.first().toObject().value("url").toString();
}

QVariantMap SpotifyApi::track(const QJsonObject &object)
{
    const QString id = object.value("id").toString(), title = object.value("name").toString();
    if (id.isEmpty() || title.isEmpty()) return {};

    const QJsonObject albumObject = object.value("album").toObject();
    const bool preview = !object.value("preview_url").toString().isEmpty();
    QVariantMap item = ApiCommon::song(title, artistNames(object.value("artists").toArray()), imageOf(albumObject), id,
                                       object.value("duration_ms").toInt() / 1000, albumObject.value("name").toString(),
                                       id, id, preview ? ApiCommon::kPaytypeFree : ApiCommon::kPaytypePaid);
    item.insert("id", id);
    return item;
}

QVariantMap SpotifyApi::playlist(const QJsonObject &object)
{
    const QString id = object.value("id").toString(), title = object.value("name").toString();
    if (id.isEmpty() || title.isEmpty()) return {};

    QVariantMap item = ApiCommon::song(title, object.value("owner").toObject().value("display_name").toString(), imageOf(object),
                                       id, 0, object.value("description").toString(), id, id, 0,
                                       object.value("tracks").toObject().value("total").toInt());
    item.insert("id", id);
    item.insert("tagid", id);
    return item;
}

QVariantMap SpotifyApi::album(const QJsonObject &object)
{
    const QString id = object.value("id").toString(), title = object.value("name").toString();
    if (id.isEmpty() || title.isEmpty()) return {};

    const QString hash = "album:" + id;
    QVariantMap item = ApiCommon::song(title, artistNames(object.value("artists").toArray()), imageOf(object), hash, 0,
                                       object.value("release_date").toString(), hash, hash);
    item.insert("id", hash);
    item.insert("tagid", id);
    return item;
}

QVariantMap SpotifyApi::artist(const QJsonObject &object)
{
    const QString id = object.value("id").toString(), name = object.value("name").toString();
    if (id.isEmpty() || name.isEmpty()) return {};

    QVariantMap item = ApiCommon::song(name, QString(), imageOf(object), id);
    item.insert("id", id);
    return item;
}

void SpotifyApi::search(const QString &keyword, int type, int page, int pageSize, const QString &action)
{
    if (keyword.trimmed().isEmpty()) {
        emit resultReady(action, ApiCommon::listResult({}), Source);
        return;
    }
    QUrlQuery query = pageQuery(page, pageSize);
    query.addQueryItem("q", keyword);
    query.addQueryItem("type", type == 1 ? "playlist" : type == 2 ? "album" : "track");
    get("/v1/search", query, [this, type, action](const QJsonObject &json) {
        const QString key = type == 1 ? "playlists" : type == 2 ? "albums" : "tracks";
        const QJsonArray items = json.value(key).toObject().value("items").toArray();
        QVariantList info;
        info.reserve(items.size());
        for (const QJsonValue &value : items) {
            const QVariantMap item = type == 1 ? playlist(value.toObject())
                                   : type == 2 ? album(value.toObject()) : track(value.toObject());
            if (!item.isEmpty()) info << item;
        }
        emit resultReady(action, ApiCommon::listResult(info, json), Source);
    });
}

void SpotifyApi::onSearchSongs(const QString &keyword, int type, int page, int pageSize)
{
    if (type == 3) {
        onSearchLyrics(keyword, page, pageSize);
        return;
    }
    search(keyword, type, page, pageSize, ApiAction::SearchSongs);
}

void SpotifyApi::onSearchLyrics(const QString &, int, int)
{
    emit resultReady(ApiAction::SearchLyrics, ApiCommon::listResult({}), Source);
}

void SpotifyApi::onPlaylistMenu(int)
{
    QVariantList info;
    info.reserve(kMenuTitles.size());
    for (int i = 0; i < kMenuTitles.size(); ++i)
        info << QVariantMap{{"title", kMenuTitles.at(i)}, {"id", kMenuIds.at(i)}, {"category", "分类"}};
    emit resultReady(ApiAction::PlaylistMenu, ApiCommon::listResult(info), Source);
}

void SpotifyApi::onMusicPlaylists(const QString &tagid, int page, int pageSize)
{
    search(tagid.isEmpty() ? "Top 50" : tagid, 1, page, pageSize, ApiAction::MusicPlaylists);
}

void SpotifyApi::onCategoryPlaylists(const QString &categoryid, int page, int pageSize)
{
    search(categoryid.isEmpty() ? "Top 50" : categoryid, 1, page, pageSize, ApiAction::CategoryPlaylists);
}

void SpotifyApi::playlistTracks(const QString &id, int page, int pageSize, const QString &action)
{
    if (id.isEmpty()) {
        emit resultReady(action, ApiCommon::listResult({}), Source);
        return;
    }
    QUrlQuery query = pageQuery(page, pageSize);
    const bool album = id.startsWith("album:");
    const QString path = album ? "/v1/albums/" + QString::fromUtf8(QUrl::toPercentEncoding(id.mid(6))) + "/tracks"
                               : "/v1/playlists/" + QString::fromUtf8(QUrl::toPercentEncoding(id)) + "/tracks";
    get(path, query,
        [this, action](const QJsonObject &json) {
            const QJsonArray items = json.value("items").toArray();
            QVariantList info;
            info.reserve(items.size());
            for (const QJsonValue &value : items) {
                const QVariantMap item = track(value.toObject().value("track").toObject());
                if (!item.isEmpty()) info << item;
            }
            emit resultReady(action, ApiCommon::listResult(info, json), Source);
        });
}

void SpotifyApi::onPlaylistSongs(const QString &listid, int page, int pageSize)
{
    playlistTracks(listid, page, pageSize, ApiAction::PlaylistSongs);
}

void SpotifyApi::recommendations(int page, int pageSize, const QString &action)
{
    if (page > 1) {
        emit resultReady(action, ApiCommon::listResult({}), Source);
        return;
    }
    QUrlQuery query = pageQuery(1, pageSize);
    query.addQueryItem("seed_genres", "pop");
    get("/v1/recommendations", query, [this, action](const QJsonObject &json) {
        const QJsonArray tracks = json.value("tracks").toArray();
        QVariantList info;
        info.reserve(tracks.size());
        for (const QJsonValue &value : tracks) {
            const QVariantMap item = track(value.toObject());
            if (!item.isEmpty()) info << item;
        }
        emit resultReady(action, ApiCommon::listResult(info, json), Source);
    });
}

void SpotifyApi::onRecommendSongs(int page, int pageSize)
{
    recommendations(page, pageSize, ApiAction::RecommendSongs);
}

void SpotifyApi::onHotPlaylistMenu(int)
{
    emit resultReady(ApiAction::HotPlaylistMenu,
                     ApiCommon::listResult({QVariantMap{{"title", "热门歌单"}, {"id", "Top 50"}, {"category", "热门"}}}),
                     Source);
}

void SpotifyApi::onHotPlaylists(int page, int pageSize)
{
    search("Top 50", 1, page, pageSize, ApiAction::HotPlaylists);
}

void SpotifyApi::onNewSongs(int, int page, int pageSize)
{
    search(QString("year:%1").arg(QDate::currentDate().year()), 0, page, pageSize, ApiAction::NewSongs);
}

void SpotifyApi::onAllToplist()
{
    QVariantList info;
    info.reserve(kToplistNames.size());
    for (int i = 0; i < kToplistNames.size(); ++i)
        info << ApiCommon::song(kToplistNames.at(i), "Spotify", QString(), QString::number(i));
    emit resultReady(ApiAction::AllToplist, ApiCommon::listResult(info), Source);
}

void SpotifyApi::onMusicToplist(int page, int pageSize, int rankid)
{
    if (rankid < 0 || rankid >= kToplists.size()) {
        emit resultReady(ApiAction::MusicToplist, ApiCommon::listResult({}), Source);
        return;
    }
    playlistTracks(kToplists.at(rankid), page, pageSize, ApiAction::MusicToplist);
}

void SpotifyApi::onHotSingers(int page, int pageSize)
{
    QUrlQuery query = pageQuery(page, pageSize);
    query.addQueryItem("q", "pop");
    query.addQueryItem("type", "artist");
    get("/v1/search", query, [this](const QJsonObject &json) {
        const QJsonArray artists = json.value("artists").toObject().value("items").toArray();
        QVariantList info;
        info.reserve(artists.size());
        for (const QJsonValue &value : artists) {
            const QVariantMap item = artist(value.toObject());
            if (!item.isEmpty()) info << item;
        }
        emit resultReady(ApiAction::HotSingers, ApiCommon::listResult(info, json), Source);
    });
}

void SpotifyApi::onSingerCategory(int, int page, int pageSize)
{
    onHotSingers(page, pageSize);
}

void SpotifyApi::onSingerSongs(const QString &singerid, int page, int pageSize)
{
    if (singerid.isEmpty()) {
        emit resultReady(ApiAction::SingerSongs, ApiCommon::listResult({}), Source);
        return;
    }
    QUrlQuery query;
    query.addQueryItem("market", "US");
    get("/v1/artists/" + QString::fromUtf8(QUrl::toPercentEncoding(singerid)) + "/top-tracks", query,
        [this, page, pageSize](const QJsonObject &json) {
            const int size = qBound(1, pageSize, 50), start = (qMax(1, page) - 1) * size;
            const QJsonArray tracks = json.value("tracks").toArray();
            QVariantList info;
            info.reserve(size);
            for (int i = start; i < tracks.size() && i < start + size; ++i) {
                const QVariantMap item = track(tracks.at(i).toObject());
                if (!item.isEmpty()) info << item;
            }
            emit resultReady(ApiAction::SingerSongs, ApiCommon::listResult(info, json), Source);
        });
}

void SpotifyApi::onMusicInfo(const QString &hash, int type)
{
    const QString id = hash.trimmed();
    if (id.isEmpty()) {
        emit resultReady(ApiAction::MusicInfo, QVariantMap{{"error", -1}, {"hash", id}, {"type", type}}, Source);
        return;
    }
    get("/v1/tracks/" + QString::fromUtf8(QUrl::toPercentEncoding(id)), {}, [this, id, type](const QJsonObject &json) {
        const QString preview = json.value("preview_url").toString();
        if (preview.isEmpty()) {
            emit resultReady(ApiAction::MusicInfo,
                             QVariantMap{{"error", -1}, {"message", "Spotify 无试听地址"}, {"hash", id}, {"type", type}},
                             Source);
            return;
        }
        const QJsonObject albumObject = json.value("album").toObject();
        const QString title = json.value("name").toString(), cover = imageOf(albumObject);
        QVariantMap data{{"url", preview}, {"backup_url", preview}, {"songName", title},
                         {"author_name", artistNames(json.value("artists").toArray())},
                         {"singer_img", cover}, {"album_img", cover},
                         {"timeLength", json.value("duration_ms").toInt() / 1000},
                         {"fileName", title + ".m4a"}, {"hash", id}, {"type", type}};
        emit resultReady(ApiAction::MusicInfo, data, Source);
    });
}

void SpotifyApi::onLyricInfo(const QString &hash, int)
{
    emit resultReady(ApiAction::LyricInfo,
                     QVariantMap{{"info", QVariantList{}}, {"translate", QVariantList{}}, {"hash", hash}},
                     Source);
}

void SpotifyApi::onComments(const QString &, int, int)
{
    emit resultReady(ApiAction::Comments, ApiCommon::listResult({}, false), Source);
}

void SpotifyApi::onPersonalFm(int page, int pageSize)
{
    recommendations(page, pageSize, ApiAction::PersonalFm);
}

void SpotifyApi::onPersonalRadar(int page, int pageSize)
{
    recommendations(page, pageSize, ApiAction::PersonalRadar);
}
