// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#include "NeteaseCloudApi.h"

#include "apihelper.h"
#include "ApiCommon.h"

#include <QDebug>
#include <QHash>
#include <QRegularExpression>
#include <QStringList>
#include <QThread>
#include <QVariantList>
#include <QVariantMap>
#include <algorithm>
#include <utility>

namespace {

constexpr int kSource = 1;

QString songId(const QVariant &value)
{
    return QString::number(value.toLongLong());
}

int paytypeFromFee(qint64 fee)
{
    return (fee == 1 || fee == 4) ? ApiCommon::kPaytypePaid : ApiCommon::kPaytypeFree;
}

qint64 feeOf(const QVariantMap &song)
{
    return song.value("privilege").toMap()
        .value("fee", song.value("fee"))
        .toLongLong();
}

QVariantMap songFromMap(const QVariantMap &song)
{
    const QVariantList ar = song.value("ar").toList();
    const QVariantList artists = ar.isEmpty()
        ? song.value("artists").toList() : ar;
    const QVariantMap al = song.value("al").toMap();
    const QVariantMap album = al.isEmpty()
        ? song.value("album").toMap() : al;
    const QString id = songId(song.value("id"));
    const qint64 dt = song.value("dt").toLongLong();
    const int duration = dt > 0
        ? int(dt / 1000)
        : int(song.value("duration").toLongLong() / 1000);
    return ApiCommon::song(
        song.value("name").toString(),
        artists.isEmpty() ? QStringLiteral("未知歌手")
                          : artists.first().toMap().value("name").toString(),
        album.value("picUrl").toString(),
        id,
        duration,
        album.value("name").toString(),
        id, id,
        paytypeFromFee(feeOf(song)));
}

QVariantList parseSongList(const QVariantList &songs)
{
    QVariantList info;
    info.reserve(songs.size());
    for (const QVariant &value : songs)
        info.append(songFromMap(value.toMap()));
    return info;
}

QVariantList parseYrc(const QString &text)
{
    QVariantList result;
    static const QRegularExpression lineRe("^\\[(\\d+),(\\d+)\\](.*)$");
    static const QRegularExpression wordRe("\\((\\d+),(\\d+),(\\d+)\\)([^()]*)");
    const QStringList lines = text.split(QLatin1Char('\n'));
    for (const QString &raw : lines) {
        const QString line = raw.trimmed();
        if (line.isEmpty())
            continue;
        const QRegularExpressionMatch match = lineRe.match(line);
        if (!match.hasMatch())
            continue;
        const qint64 lineStart = match.captured(1).toLongLong();
        QVariantList words;
        QString fullText;
        QRegularExpressionMatchIterator it = wordRe.globalMatch(match.captured(3));
        while (it.hasNext()) {
            const QRegularExpressionMatch word = it.next();
            const QString wordText = word.captured(4);
            if (wordText.isEmpty())
                continue;
            const qint64 wordStart = word.captured(1).toLongLong();
            words.append(QVariantMap{
                {"offset", int(wordStart >= lineStart ? wordStart - lineStart : wordStart)},
                {"duration", word.captured(2).toInt()},
                {"text", wordText}
            });
            fullText += wordText;
        }
        if (words.isEmpty() || fullText.trimmed().isEmpty())
            continue;
        result.append(QVariantMap{
            {"time", lineStart},
            {"text", fullText},
            {"info", words}
        });
    }
    std::sort(result.begin(), result.end(), [](const QVariant &a, const QVariant &b) {
        return a.toMap().value("time").toLongLong()
               < b.toMap().value("time").toLongLong();
    });
    return result;
}

QVariantList parseLyrics(const QString &text)
{
    QVariantList result;
    static const QRegularExpression re("^(\\d{2}):(\\d{2})[.:](\\d{2,3})$");
    const QStringList parts = text.split(QLatin1Char('['));
    result.reserve(parts.size());
    for (int i = 1; i < parts.size(); ++i) {
        const QString part = parts.at(i);
        const int close = part.indexOf(QLatin1Char(']'));
        if (close < 0)
            continue;
        const QString lyric = part.mid(close + 1).trimmed();
        if (lyric.isEmpty())
            continue;
        const QRegularExpressionMatch match = re.match(part.left(close));
        if (!match.hasMatch())
            continue;
        const QString msText = match.captured(3);
        const int ms = msText.size() == 2 ? msText.toInt() * 10 : msText.toInt();
        result.append(QVariantMap{
            {"time", match.captured(1).toInt() * 60000
                                         + match.captured(2).toInt() * 1000 + ms},
            {"text", lyric},
            {"info", QVariant()}
        });
    }
    std::sort(result.begin(), result.end(), [](const QVariant &a, const QVariant &b) {
        return a.toMap().value("time").toInt()
               < b.toMap().value("time").toInt();
    });
    return result;
}

QVariantList parseComments(const QVariantList &items)
{
    QVariantList info;
    info.reserve(items.size());
    for (const QVariant &value : items) {
        const QVariantMap comment = value.toMap();
        const QVariantMap user = comment.value("user").toMap();
        info.append(ApiCommon::comment(
            user.value("nickname").toString(),
            user.value("avatarUrl").toString(),
            comment.value("content").toString(),
            comment.value("time").toLongLong() / 1000,
            comment.value("likedCount").toInt(),
            int(comment.value("beReplied").toList().size())));
    }
    return info;
}

QVariantList parsePlaylists(const QVariantList &playlists)
{
    QVariantList info;
    info.reserve(playlists.size());
    for (const QVariant &value : playlists) {
        const QVariantMap playlist = value.toMap();
        const QVariantMap creator = playlist.value("creator").toMap();
        info.append(ApiCommon::song(
            playlist.value("name").toString(),
            creator.value("nickname").toString(),
            playlist.value("coverImgUrl").toString(),
            songId(playlist.value("id")),
            playlist.value("trackCount").toInt(),
            playlist.value("description").toString(),
            QString(), QString(), 0,
            playlist.value("playCount").toLongLong()));
    }
    return info;
}

}

class CloudWorker : public QObject
{
    Q_OBJECT
public:
    ~CloudWorker() { delete m_api; }

public slots:
    void request(const QString &action, const QVariantMap &call)
    {
        const QString member = call.value("member").toString();
        const QVariantMap arg = call.value("arg").toMap();
        auto invoke = [this](const QString &name, const QVariantMap &args) {
            QVariantMap result = helper()->invoke(name, args);
            if (result.value("status").toInt() >= 500) {
                QThread::msleep(150);
                result = helper()->invoke(name, args);
            }
            return result;
        };

        const QVariantMap raw = invoke(member, arg);
        const bool transportFailed = raw.value("status").toInt() >= 500;
        QVariantMap effective = raw;
        if (transportFailed)
            effective.insert("error", raw.value("status"));

        if (action == QLatin1String("getMusicInfo")) {
            static const char *kLevels[] = {"standard", "exhigh", "lossless", "hires"};
            const int requested = qBound(0, call.value("quality").toInt(), 3);
            QString playUrl;
            bool unavailable = false;
            for (int level = requested; level >= 0; --level) {
                QVariantMap urlArg;
                urlArg.insert("id", arg.value("id"));
                urlArg.insert("level", QString::fromLatin1(kLevels[level]));
                const QVariantMap urlRaw = invoke("song_url_v1", urlArg);
                if (urlRaw.value("status").toInt() >= 500) {
                    effective.insert("error", urlRaw.value("status"));
                    break;
                }
                const QVariantList urlData = urlRaw.value("body").toMap()
                                                 .value("data").toList();
                if (urlData.isEmpty())
                    continue;
                const QVariantMap first = urlData.first().toMap();
                playUrl = first.value("url").toString();
                if (playUrl.isEmpty())
                    unavailable = true;
                const int fee = first.value("fee").toInt();
                if (fee > 1 && playUrl.isEmpty())
                    break;
                if (!playUrl.isEmpty())
                    break;
            }
            effective.insert("play_url", playUrl);
            if (playUrl.isEmpty() && unavailable)
                effective.insert("url_unavailable", true);
        }

        emit result(action, mapResult(action, effective, call), kSource);
    }

    void setCookie(const QString &cookie) { helper()->set_cookie(cookie); }

signals:
    void result(const QString &action, const QVariant &data, int source);

private:
    ApiHelper *helper()
    {
        if (!m_api) {
            m_api = new ApiHelper;
            m_api->setFilterRules("QCloudMusicApi.debug=false");
        }
        return m_api;
    }

    static QVariant mapResult(const QString &action, const QVariantMap &raw,
                              const QVariantMap &call)
    {
        const bool transportFailed = raw.contains("error");
        const QVariant transportError = raw.value("error");
        const QVariantMap body = raw.value("body").toMap();
        const QVariantMap &data = body.isEmpty() ? raw : body;
        QVariantList info;
        bool commentsHasMore = false;
        auto unwrapSongs = [](const QVariantList &list) {
            QVariantList songs;
            songs.reserve(list.size());
            for (const QVariant &value : list) {
                const QVariantMap wrapper = value.toMap();
                songs << wrapper.value("song", wrapper);
            }
            return songs;
        };

        if (action == QLatin1String("getComments")) {
            info = parseComments(data.value("hotComments").toList());
            info += parseComments(data.value("comments").toList());
            commentsHasMore = data.value("more").toBool();
        } else if (action == QLatin1String("searchSongs")
                   || action == QLatin1String("searchLyrics")) {
            const QVariantMap result = data.value("result").toMap();
            const int ntype = call.value("ntype").toInt();
            if (ntype == 1 || ntype == 1006) {
                info = parseSongList(result.value("songs").toList());
            } else if (ntype == 1000) {
                info = parsePlaylists(result.value("playlists").toList());
            } else if (ntype == 10) {
                const QVariantList albums = result.value("albums").toList();
                info.reserve(albums.size());
                for (const QVariant &value : albums) {
                    const QVariantMap album = value.toMap();
                    info.append(ApiCommon::song(album.value("name").toString(), album.value("artist").toMap().value("name").toString(),
                                                album.value("picUrl").toString(), songId(album.value("id")), 0, album.value("name").toString()));
                }
            }
        } else if (action == QLatin1String("getPlaylistMenu")) {
            const QVariantMap categories = data.value("categories").toMap();
            const QVariantList subs = data.value("sub").toList();
            info.reserve(subs.size());
            for (const QVariant &value : subs) {
                const QVariantMap sub = value.toMap();
                if (!sub.value("hot").toBool()) continue;
                const QString name = sub.value("name").toString();
                if (name.isEmpty()) continue;
                info.append(QVariantMap{{"title", name}, {"id", name}, {"tagid", name},
                                        {"category", categories.value(sub.value("category").toString()).toString()}});
            }
        } else if (action == QLatin1String("getMusicPlaylists")
                   || action == QLatin1String("getHotPlaylists")) {
            info = parsePlaylists(data.value("playlists").toList());
        } else if (action == QLatin1String("getPlaylistSongs")
                   || action == QLatin1String("getMusicToplist")
                   || action == QLatin1String("getSingerSongs")) {
            QVariantList songs = data.value("songs").toList();
            if (songs.isEmpty()) songs = data.value("playlist").toMap().value("tracks").toList();
            if (action == QLatin1String("getMusicToplist")) {
                const QVariantMap arg = call.value("arg").toMap();
                const int offset = arg.value("offset").toInt();
                const int limit = arg.value("limit").toInt();
                if (limit > 0)
                    songs = offset < songs.size() ? songs.mid(offset, limit) : QVariantList();
            }
            info = parseSongList(songs);
        } else if (action == QLatin1String("getPersonalFm")) {
            info = parseSongList(unwrapSongs(data.value("data").toList()));
        } else if (action == QLatin1String("getPersonalRadar")) {
            info = parseSongList(data.value("data").toMap().value("dailySongs").toList());
        } else if (action == QLatin1String("getRecommendSongs")) {
            info = parseSongList(unwrapSongs(data.contains("data") ? data.value("data").toList() : data.value("result").toList()));
        } else if (action == QLatin1String("getHotPlaylistMenu")) {
            const QVariantList tags = data.value("tags").toList();
            info.reserve(tags.size());
            for (const QVariant &value : tags) {
                const QVariantMap tag = value.toMap();
                info.append(QVariantMap{{"title", tag.value("name").toString()}, {"id", songId(tag.value("id"))},
                                        {"tagid", tag.value("name").toString()}});
            }
        } else if (action == QLatin1String("getNewSongs")) {
            info = parseSongList(unwrapSongs(data.value("data").toList()));
        } else if (action == QLatin1String("getAllToplist")) {
            const QVariantList list = data.value("list").toList();
            info.reserve(list.size());
            for (const QVariant &value : list) {
                const QVariantMap toplist = value.toMap();
                info.append(ApiCommon::song(
                    toplist.value("name").toString(),
                    toplist.value("updateFrequency").toString(),
                    toplist.value("coverImgUrl").toString(),
                    songId(toplist.value("id")),
                    0, toplist.value("description").toString()));
            }
        } else if (action == QLatin1String("getHotSingers")
                   || action == QLatin1String("getSingerCategory")) {
            const QVariantList artists = data.value("artists").toList();
            info.reserve(artists.size());
            for (const QVariant &value : artists) {
                const QVariantMap artist = value.toMap();
                info.append(ApiCommon::song(
                    artist.value("name").toString(),
                    QString(),
                    artist.value("picUrl").toString(),
                    songId(artist.value("id"))));
            }
        } else if (action == QLatin1String("getMusicInfo")) {
            const QString hash = call.value("hash").toString();
            const int type = call.value("type").toInt();
            QString playUrl = raw.value("play_url").toString();
            if (!transportFailed && playUrl.isEmpty()
                && !raw.value("url_unavailable").toBool()) {
                playUrl = "http://music.163.com/song/media/outer/url?id="
                          + hash + ".mp3";
            }

            const QVariantList songs = data.value("songs").toList();
            const QVariantMap song = songs.isEmpty() ? QVariantMap() : songs.first().toMap();

            QString author;
            QString singerImg;
            const QVariantList ar = song.value("ar").toList();
            if (!ar.isEmpty()) {
                const QVariantMap artist = ar.first().toMap();
                author = artist.value("name").toString();
                singerImg = artist.value("img1v1Url").toString();
                if (singerImg.isEmpty())
                    singerImg = artist.value("picUrl").toString();
            }
            const QString albumImg = song.value("al").toMap().value("picUrl").toString();
            const qint64 dt = song.value("dt").toLongLong();
            const QString fileName = song.value("name").toString();
            QVariantMap detail{{"backup_url", playUrl}, {"url", playUrl}, {"songName", song.value("name").toString()},
                               {"author_name", author}, {"singer_img", singerImg}, {"album_img", albumImg},
                               {"timeLength", dt > 0 ? int(dt / 1000) : int(song.value("duration").toLongLong() / 1000)},
                               {"fileName", (fileName.isEmpty() ? "unknown" : fileName) + ".mp3"}, {"hash", hash}, {"type", type}};
            if (transportFailed) detail.insert("error", transportError);
            return detail;
        } else if (action == QLatin1String("getLyricInfo")) {
            QVariantList lyrics = parseYrc(data.value("yrc").toMap().value("lyric").toString());
            if (lyrics.isEmpty()) lyrics = parseLyrics(data.value("lrc").toMap().value("lyric").toString());

            QString translationText = data.value("ytlrc").toMap().value("lyric").toString();
            if (translationText.isEmpty()) translationText = data.value("tlyric").toMap().value("lyric").toString();
            const QVariantList translations = parseLyrics(translationText);
            QVariantList translate;
            if (!translations.isEmpty()) {
                QHash<qlonglong, QString> byTime;
                byTime.reserve(translations.size());
                for (const QVariant &value : translations) {
                    const QVariantMap line = value.toMap();
                    byTime.insert(line.value("time").toLongLong(), line.value("text").toString());
                }
                translate.reserve(lyrics.size());
                for (const QVariant &value : std::as_const(lyrics))
                    translate.append(byTime.value(value.toMap().value("time").toLongLong()));
            }

            QVariantMap detail{{"info", lyrics}, {"translate", translate}, {"hash", call.value("arg").toMap().value("id").toString()}};
            if (transportFailed) detail.insert("error", transportError);
            return detail;
        } else {
            qWarning() << "[NeteaseCloudApi] 未知 action:" << action;
        }

        QVariantMap result = ApiCommon::listResult(info);
        if (action == QLatin1String("getComments")) result.insert("hasMore", commentsHasMore);
        if (transportFailed) result.insert("error", transportError);
        return result;
    }

    ApiHelper *m_api = nullptr;
};

NeteaseCloudApi::NeteaseCloudApi(QObject *parent)
    : MusicSource(ApiSource::Netease, parent)
{
    for (int i = 0; i < 3; ++i) {
        auto *thread = new QThread(this);
        auto *worker = new CloudWorker;
        worker->moveToThread(thread);
        connect(this, &NeteaseCloudApi::setCookieRequest, worker, &CloudWorker::setCookie);
        connect(worker, &CloudWorker::result, this, &NeteaseCloudApi::onWorkerResult);
        thread->start();
        m_threads.append(thread);
        m_workers.append(worker);
    }
}

NeteaseCloudApi::~NeteaseCloudApi()
{
    for (QThread *thread : std::as_const(m_threads))
        thread->quit();
    for (QThread *thread : std::as_const(m_threads))
        thread->wait();
    qDeleteAll(m_workers);
}

void NeteaseCloudApi::onWorkerResult(const QString &action, const QVariant &data, int source)
{
    emit resultReady(action, data, source);
}

void NeteaseCloudApi::enqueue(const QString &action, const QString &member,
                              const QVariantMap &arg, const QVariantMap &extra)
{
    if (m_workers.isEmpty())
        return;
    QVariantMap call = extra;
    call.insert("member", member);
    call.insert("arg", arg);

    CloudWorker *worker = m_workers.at(m_next);
    m_next = (m_next + 1) % m_workers.size();
    QMetaObject::invokeMethod(worker, "request", Qt::QueuedConnection,
                              Q_ARG(QString, action), Q_ARG(QVariantMap, call));
}

void NeteaseCloudApi::setCookie(const QString &cookie)
{
    emit setCookieRequest(cookie);
}

void NeteaseCloudApi::setQuality(int quality)
{
    m_quality.store(qBound(0, quality, 3), std::memory_order_relaxed);
}

void NeteaseCloudApi::applyCredentials(const SourceCredentials &credentials)
{
    if (m_credentials == credentials)
        return;

    const bool cookieChanged = m_credentials.cookie != credentials.cookie;
    m_credentials = credentials;
    setQuality(credentials.quality);
    if (cookieChanged)
        setCookie(credentials.cookie);
}

void NeteaseCloudApi::onSearchSongs(const QString &keyword, int type, int page, int pageSize)
{
    int ntype = 1;
    if (type == 1)
        ntype = 1000;
    else if (type == 2)
        ntype = 10;
    else if (type == 3)
        ntype = 1006;

    enqueue("searchSongs", "cloudsearch",
            {{"keywords", keyword},
             {"type", ntype},
             {"limit", pageSize},
             {"offset", (page - 1) * pageSize}},
            {{"ntype", ntype}});
}

void NeteaseCloudApi::onSearchLyrics(const QString &keyword, int page, int pageSize)
{
    enqueue("searchLyrics", "cloudsearch",
            {{"keywords", keyword},
             {"type", 1},
             {"limit", pageSize},
             {"offset", qMax(0, page - 1) * pageSize}},
            {{"ntype", 1}});
}

void NeteaseCloudApi::onPlaylistMenu(int type)
{
    Q_UNUSED(type);
    enqueue("getPlaylistMenu", "playlist_catlist", {});
}

void NeteaseCloudApi::onCategoryPlaylists(const QString &id, int page, int pageSize)
{
    // 网易云的分类歌单就是 top_playlist?cat=，直接复用。
    onMusicPlaylists(id, page, pageSize);
}

void NeteaseCloudApi::onMusicPlaylists(const QString &tagid, int page, int pageSize)
{
    enqueue("getMusicPlaylists", "top_playlist",
            {{"cat", tagid},
             {"order", "hot"},
             {"offset", (page - 1) * pageSize},
             {"limit", pageSize}});
}

void NeteaseCloudApi::onPlaylistSongs(const QString &listid, int page, int pageSize)
{
    enqueue("getPlaylistSongs", "playlist_track_all",
            {{"id", listid},
             {"limit", pageSize},
             {"offset", (page - 1) * pageSize}});
}

void NeteaseCloudApi::onRecommendSongs(int page, int pageSize)
{
    Q_UNUSED(page);
    enqueue("getRecommendSongs", "personalized_newsong",
            {{"limit", pageSize}});
}

void NeteaseCloudApi::onHotPlaylistMenu(int type)
{
    Q_UNUSED(type);
    enqueue("getHotPlaylistMenu", "playlist_hot", {});
}

void NeteaseCloudApi::onHotPlaylists(int page, int pageSize)
{
    enqueue("getHotPlaylists", "top_playlist",
            {{"cat", QStringLiteral("全部")},
             {"order", "hot"},
             {"offset", (page - 1) * pageSize},
             {"limit", pageSize}});
}

void NeteaseCloudApi::onNewSongs(int type, int page, int pageSize)
{
    int ntype = 0;
    switch (type) {
    case 1: ntype = 7;  break;
    case 2: ntype = 96; break;
    case 3: ntype = 8;  break;
    case 4: ntype = 16; break;
    case 5: ntype = 8;  break;
    default: ntype = 0; break;
    }
    enqueue("getNewSongs", "top_song",
            {{"type", ntype},
             {"limit", pageSize},
             {"offset", (page - 1) * pageSize}});
}

void NeteaseCloudApi::onAllToplist()
{
    enqueue("getAllToplist", "toplist", {});
}

void NeteaseCloudApi::onMusicToplist(int page, int pageSize, int rankid)
{
    enqueue("getMusicToplist", "top_list",
            {{"id", rankid},
             {"offset", (page - 1) * pageSize},
             {"limit", pageSize}});
}

void NeteaseCloudApi::onHotSingers(int page, int pageSize)
{
    enqueue("getHotSingers", "top_artists",
            {{"offset", (page - 1) * pageSize},
             {"limit", pageSize}});
}

void NeteaseCloudApi::onSingerCategory(int area, int page, int pageSize)
{
    enqueue("getSingerCategory", "artist_list",
            {{"offset", (page - 1) * pageSize},
             {"limit", pageSize},
             {"type", "1"},
             {"area", area}});
}

void NeteaseCloudApi::onSingerSongs(const QString &singerid, int page, int pageSize)
{
    Q_UNUSED(page);
    Q_UNUSED(pageSize);
    enqueue("getSingerSongs", "artist_top_song",
            {{"id", singerid}});
}

void NeteaseCloudApi::onMusicInfo(const QString &hash, int type)
{
    enqueue("getMusicInfo", "song_detail",
            {{"ids", hash},
             {"id", hash}},
            {{"hash", hash},
             {"type", type},
             {"quality", m_quality.load(std::memory_order_relaxed)}});
}

void NeteaseCloudApi::onLyricInfo(const QString &hash, int duration)
{
    Q_UNUSED(duration);
    enqueue("getLyricInfo", "lyric_new",
            {{"id", hash}});
}

void NeteaseCloudApi::onComments(const QString &hash, int page, int pageSize)
{
    enqueue("getComments", "comment_music",
            {{"id", hash},
             {"limit", pageSize},
             {"offset", qMax(0, (page - 1) * pageSize)}});
}

void NeteaseCloudApi::onPersonalFm(int page, int pageSize)
{
    Q_UNUSED(page);
    Q_UNUSED(pageSize);
    enqueue("getPersonalFm", "personal_fm", {});
}

void NeteaseCloudApi::onPersonalRadar(int page, int pageSize)
{
    Q_UNUSED(pageSize);
    if (page > 1) {
        emit resultReady("getPersonalRadar",
                         ApiCommon::listResult(QVariantList()), kSource);
        return;
    }
    enqueue("getPersonalRadar", "recommend_songs", {});
}

#include "NeteaseCloudApi.moc"