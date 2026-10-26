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
    return song.value(QStringLiteral("privilege")).toMap()
        .value(QStringLiteral("fee"), song.value(QStringLiteral("fee")))
        .toLongLong();
}

QVariantMap songFromMap(const QVariantMap &song)
{
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
    return ApiCommon::song(
        song.value(QStringLiteral("name")).toString(),
        artists.isEmpty() ? QStringLiteral("未知歌手")
                          : artists.first().toMap().value(QStringLiteral("name")).toString(),
        album.value(QStringLiteral("picUrl")).toString(),
        id,
        duration,
        album.value(QStringLiteral("name")).toString(),
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
    static const QRegularExpression lineRe(QStringLiteral("^\\[(\\d+),(\\d+)\\](.*)$"));
    static const QRegularExpression wordRe(QStringLiteral("\\((\\d+),(\\d+),(\\d+)\\)([^()]*)"));
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
                {QStringLiteral("offset"), int(wordStart >= lineStart ? wordStart - lineStart : wordStart)},
                {QStringLiteral("duration"), word.captured(2).toInt()},
                {QStringLiteral("text"), wordText}
            });
            fullText += wordText;
        }
        if (words.isEmpty() || fullText.trimmed().isEmpty())
            continue;
        result.append(QVariantMap{
            {QStringLiteral("time"), lineStart},
            {QStringLiteral("text"), fullText},
            {QStringLiteral("info"), words}
        });
    }
    std::sort(result.begin(), result.end(), [](const QVariant &a, const QVariant &b) {
        return a.toMap().value(QStringLiteral("time")).toLongLong()
               < b.toMap().value(QStringLiteral("time")).toLongLong();
    });
    return result;
}

QVariantList parseLyrics(const QString &text)
{
    QVariantList result;
    static const QRegularExpression re(QStringLiteral("^(\\d{2}):(\\d{2})[.:](\\d{2,3})$"));
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
            {QStringLiteral("time"), match.captured(1).toInt() * 60000
                                         + match.captured(2).toInt() * 1000 + ms},
            {QStringLiteral("text"), lyric},
            {QStringLiteral("info"), QVariant()}
        });
    }
    std::sort(result.begin(), result.end(), [](const QVariant &a, const QVariant &b) {
        return a.toMap().value(QStringLiteral("time")).toInt()
               < b.toMap().value(QStringLiteral("time")).toInt();
    });
    return result;
}

QVariantList parseComments(const QVariantList &items)
{
    QVariantList info;
    info.reserve(items.size());
    for (const QVariant &value : items) {
        const QVariantMap comment = value.toMap();
        const QVariantMap user = comment.value(QStringLiteral("user")).toMap();
        info.append(ApiCommon::comment(
            user.value(QStringLiteral("nickname")).toString(),
            user.value(QStringLiteral("avatarUrl")).toString(),
            comment.value(QStringLiteral("content")).toString(),
            comment.value(QStringLiteral("time")).toLongLong() / 1000,
            comment.value(QStringLiteral("likedCount")).toInt(),
            int(comment.value(QStringLiteral("beReplied")).toList().size())));
    }
    return info;
}

QVariantList parsePlaylists(const QVariantList &playlists)
{
    QVariantList info;
    info.reserve(playlists.size());
    for (const QVariant &value : playlists) {
        const QVariantMap playlist = value.toMap();
        const QVariantMap creator = playlist.value(QStringLiteral("creator")).toMap();
        info.append(ApiCommon::song(
            playlist.value(QStringLiteral("name")).toString(),
            creator.value(QStringLiteral("nickname")).toString(),
            playlist.value(QStringLiteral("coverImgUrl")).toString(),
            songId(playlist.value(QStringLiteral("id"))),
            playlist.value(QStringLiteral("trackCount")).toInt(),
            playlist.value(QStringLiteral("description")).toString(),
            QString(), QString(), 0,
            playlist.value(QStringLiteral("playCount")).toLongLong()));
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
        const QString member = call.value(QStringLiteral("member")).toString();
        const QVariantMap arg = call.value(QStringLiteral("arg")).toMap();
        auto invoke = [this](const QString &name, const QVariantMap &args) {
            QVariantMap result = helper()->invoke(name, args);
            if (result.value(QStringLiteral("status")).toInt() >= 500) {
                QThread::msleep(150);
                result = helper()->invoke(name, args);
            }
            return result;
        };

        const QVariantMap raw = invoke(member, arg);
        const bool transportFailed = raw.value(QStringLiteral("status")).toInt() >= 500;
        QVariantMap effective = raw;
        if (transportFailed)
            effective.insert(QStringLiteral("error"), raw.value(QStringLiteral("status")));

        if (action == QLatin1String("getMusicInfo")) {
            static const char *kLevels[] = {"standard", "exhigh", "lossless", "hires"};
            const int requested = qBound(0, call.value(QStringLiteral("quality")).toInt(), 3);
            QString playUrl;
            bool unavailable = false;
            for (int level = requested; level >= 0; --level) {
                QVariantMap urlArg;
                urlArg.insert(QStringLiteral("id"), arg.value(QStringLiteral("id")));
                urlArg.insert(QStringLiteral("level"), QString::fromLatin1(kLevels[level]));
                const QVariantMap urlRaw = invoke(QStringLiteral("song_url_v1"), urlArg);
                if (urlRaw.value(QStringLiteral("status")).toInt() >= 500) {
                    effective.insert(QStringLiteral("error"), urlRaw.value(QStringLiteral("status")));
                    break;
                }
                const QVariantList urlData = urlRaw.value(QStringLiteral("body")).toMap()
                                                 .value(QStringLiteral("data")).toList();
                if (urlData.isEmpty())
                    continue;
                const QVariantMap first = urlData.first().toMap();
                playUrl = first.value(QStringLiteral("url")).toString();
                if (playUrl.isEmpty())
                    unavailable = true;
                const int fee = first.value(QStringLiteral("fee")).toInt();
                if (fee > 1 && playUrl.isEmpty())
                    break;
                if (!playUrl.isEmpty())
                    break;
            }
            effective.insert(QStringLiteral("play_url"), playUrl);
            if (playUrl.isEmpty() && unavailable)
                effective.insert(QStringLiteral("url_unavailable"), true);
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
            m_api->setFilterRules(QStringLiteral("QCloudMusicApi.debug=false"));
        }
        return m_api;
    }

    static QVariant mapResult(const QString &action, const QVariantMap &raw,
                              const QVariantMap &call)
    {
        const bool transportFailed = raw.contains(QStringLiteral("error"));
        const QVariant transportError = raw.value(QStringLiteral("error"));
        const QVariantMap body = raw.value(QStringLiteral("body")).toMap();
        const QVariantMap &data = body.isEmpty() ? raw : body;
        QVariantList info;
        bool commentsHasMore = false;

        if (action == QLatin1String("getComments")) {
            info = parseComments(data.value(QStringLiteral("hotComments")).toList());
            info += parseComments(data.value(QStringLiteral("comments")).toList());
            commentsHasMore = data.value(QStringLiteral("more")).toBool();
        } else if (action == QLatin1String("searchSongs")) {
            const QVariantMap result = data.value(QStringLiteral("result")).toMap();
            const int ntype = call.value(QStringLiteral("ntype")).toInt();
            if (ntype == 1 || ntype == 1006) {
                info = parseSongList(result.value(QStringLiteral("songs")).toList());
            } else if (ntype == 1000) {
                info = parsePlaylists(result.value(QStringLiteral("playlists")).toList());
            } else if (ntype == 10) {
                const QVariantList albums = result.value(QStringLiteral("albums")).toList();
                info.reserve(albums.size());
                for (const QVariant &value : albums) {
                    const QVariantMap album = value.toMap();
                    info.append(ApiCommon::song(
                        album.value(QStringLiteral("name")).toString(),
                        album.value(QStringLiteral("artist")).toMap()
                            .value(QStringLiteral("name")).toString(),
                        album.value(QStringLiteral("picUrl")).toString(),
                        songId(album.value(QStringLiteral("id"))),
                        0, album.value(QStringLiteral("name")).toString()));
                }
            }
        } else if (action == QLatin1String("getPlaylistMenu")) {
            const QVariantMap categories = data.value(QStringLiteral("categories")).toMap();
            const QVariantList subs = data.value(QStringLiteral("sub")).toList();
            info.reserve(subs.size());
            for (const QVariant &value : subs) {
                const QVariantMap sub = value.toMap();
                if (!sub.value(QStringLiteral("hot")).toBool())
                    continue;
                const QString name = sub.value(QStringLiteral("name")).toString();
                if (name.isEmpty())
                    continue;
                info.append(QVariantMap{
                    {QStringLiteral("title"), name},
                    {QStringLiteral("id"), name},
                    {QStringLiteral("tagid"), name},
                    {QStringLiteral("category"),
                     categories.value(sub.value(QStringLiteral("category")).toString()).toString()}
                });
            }
        } else if (action == QLatin1String("getMusicPlaylists")
                   || action == QLatin1String("getHotPlaylists")) {
            info = parsePlaylists(data.value(QStringLiteral("playlists")).toList());
        } else if (action == QLatin1String("getPlaylistSongs")
                   || action == QLatin1String("getMusicToplist")
                   || action == QLatin1String("getSingerSongs")) {
            QVariantList songs = data.value(QStringLiteral("songs")).toList();
            if (songs.isEmpty()) {
                songs = data.value(QStringLiteral("playlist")).toMap()
                            .value(QStringLiteral("tracks")).toList();
            }
            if (action == QLatin1String("getMusicToplist")) {
                const QVariantMap arg = call.value(QStringLiteral("arg")).toMap();
                const int offset = arg.value(QStringLiteral("offset")).toInt();
                const int limit = arg.value(QStringLiteral("limit")).toInt();
                if (limit > 0)
                    songs = offset < songs.size() ? songs.mid(offset, limit) : QVariantList();
            }
            info = parseSongList(songs);
        } else if (action == QLatin1String("getPersonalFm")) {
            QVariantList songs;
            const QVariantList list = data.value(QStringLiteral("data")).toList();
            songs.reserve(list.size());
            for (const QVariant &value : list) {
                const QVariantMap wrapper = value.toMap();
                songs.append(wrapper.contains(QStringLiteral("song"))
                                 ? wrapper.value(QStringLiteral("song")) : wrapper);
            }
            info = parseSongList(songs);
        } else if (action == QLatin1String("getPersonalRadar")) {
            info = parseSongList(data.value(QStringLiteral("data")).toMap()
                                     .value(QStringLiteral("dailySongs")).toList());
        } else if (action == QLatin1String("getRecommendSongs")) {
            const QVariantList list = data.contains(QStringLiteral("data"))
                ? data.value(QStringLiteral("data")).toList()
                : data.value(QStringLiteral("result")).toList();
            QVariantList songs;
            songs.reserve(list.size());
            for (const QVariant &value : list) {
                const QVariantMap wrapper = value.toMap();
                songs.append(wrapper.contains(QStringLiteral("song"))
                                 ? wrapper.value(QStringLiteral("song")) : wrapper);
            }
            info = parseSongList(songs);
        } else if (action == QLatin1String("getHotPlaylistMenu")) {
            const QVariantList tags = data.value(QStringLiteral("tags")).toList();
            info.reserve(tags.size());
            for (const QVariant &value : tags) {
                const QVariantMap tag = value.toMap();
                info.append(QVariantMap{
                    {QStringLiteral("title"), tag.value(QStringLiteral("name")).toString()},
                    {QStringLiteral("id"), songId(tag.value(QStringLiteral("id")))},
                    {QStringLiteral("tagid"), tag.value(QStringLiteral("name")).toString()}
                });
            }
        } else if (action == QLatin1String("getNewSongs")) {
            QVariantList songs;
            const QVariantList list = data.value(QStringLiteral("data")).toList();
            songs.reserve(list.size());
            for (const QVariant &value : list) {
                const QVariantMap wrapper = value.toMap();
                songs.append(wrapper.contains(QStringLiteral("song"))
                                 ? wrapper.value(QStringLiteral("song")) : wrapper);
            }
            info = parseSongList(songs);
        } else if (action == QLatin1String("getAllToplist")) {
            const QVariantList list = data.value(QStringLiteral("list")).toList();
            info.reserve(list.size());
            for (const QVariant &value : list) {
                const QVariantMap toplist = value.toMap();
                info.append(ApiCommon::song(
                    toplist.value(QStringLiteral("name")).toString(),
                    toplist.value(QStringLiteral("updateFrequency")).toString(),
                    toplist.value(QStringLiteral("coverImgUrl")).toString(),
                    songId(toplist.value(QStringLiteral("id"))),
                    0, toplist.value(QStringLiteral("description")).toString()));
            }
        } else if (action == QLatin1String("getHotSingers")
                   || action == QLatin1String("getSingerCategory")) {
            const QVariantList artists = data.value(QStringLiteral("artists")).toList();
            info.reserve(artists.size());
            for (const QVariant &value : artists) {
                const QVariantMap artist = value.toMap();
                info.append(ApiCommon::song(
                    artist.value(QStringLiteral("name")).toString(),
                    QString(),
                    artist.value(QStringLiteral("picUrl")).toString(),
                    songId(artist.value(QStringLiteral("id")))));
            }
        } else if (action == QLatin1String("getMusicInfo")) {
            const QString hash = call.value(QStringLiteral("hash")).toString();
            const int type = call.value(QStringLiteral("type")).toInt();
            QString playUrl = raw.value(QStringLiteral("play_url")).toString();
            if (!transportFailed && playUrl.isEmpty()
                && !raw.value(QStringLiteral("url_unavailable")).toBool()) {
                playUrl = QStringLiteral("http://music.163.com/song/media/outer/url?id=")
                          + hash + QStringLiteral(".mp3");
            }

            const QVariantList songs = data.value(QStringLiteral("songs")).toList();
            const QVariantMap song = songs.isEmpty() ? QVariantMap() : songs.first().toMap();

            QString author;
            QString singerImg;
            const QVariantList ar = song.value(QStringLiteral("ar")).toList();
            if (!ar.isEmpty()) {
                const QVariantMap artist = ar.first().toMap();
                author = artist.value(QStringLiteral("name")).toString();
                singerImg = artist.value(QStringLiteral("img1v1Url")).toString();
                if (singerImg.isEmpty())
                    singerImg = artist.value(QStringLiteral("picUrl")).toString();
            }
            const QString albumImg = song.value(QStringLiteral("al")).toMap()
                                         .value(QStringLiteral("picUrl")).toString();

            QVariantMap detail;
            detail.insert(QStringLiteral("backup_url"), playUrl);
            detail.insert(QStringLiteral("url"), playUrl);
            detail.insert(QStringLiteral("songName"), song.value(QStringLiteral("name")).toString());
            detail.insert(QStringLiteral("author_name"), author);
            detail.insert(QStringLiteral("singer_img"), singerImg);
            detail.insert(QStringLiteral("album_img"), albumImg);
            const qint64 dt = song.value(QStringLiteral("dt")).toLongLong();
            detail.insert(QStringLiteral("timeLength"),
                          dt > 0 ? int(dt / 1000)
                                 : int(song.value(QStringLiteral("duration")).toLongLong() / 1000));
            QString fileName = song.value(QStringLiteral("name")).toString();
            if (fileName.isEmpty())
                fileName = QStringLiteral("unknown");
            detail.insert(QStringLiteral("fileName"), fileName + QStringLiteral(".mp3"));
            detail.insert(QStringLiteral("hash"), hash);
            detail.insert(QStringLiteral("type"), type);
            if (transportFailed)
                detail.insert(QStringLiteral("error"), transportError);
            return detail;
        } else if (action == QLatin1String("getLyricInfo")) {
            QVariantList lyrics = parseYrc(data.value(QStringLiteral("yrc"))
                                               .toMap().value(QStringLiteral("lyric")).toString());
            if (lyrics.isEmpty()) {
                lyrics = parseLyrics(data.value(QStringLiteral("lrc"))
                                         .toMap().value(QStringLiteral("lyric")).toString());
            }

            QString translationText = data.value(QStringLiteral("ytlrc"))
                                          .toMap().value(QStringLiteral("lyric")).toString();
            if (translationText.isEmpty()) {
                translationText = data.value(QStringLiteral("tlyric"))
                                      .toMap().value(QStringLiteral("lyric")).toString();
            }
            const QVariantList translations = parseLyrics(translationText);
            QVariantList translate;
            if (!translations.isEmpty()) {
                QHash<qlonglong, QString> byTime;
                byTime.reserve(translations.size());
                for (const QVariant &value : translations) {
                    const QVariantMap line = value.toMap();
                    byTime.insert(line.value(QStringLiteral("time")).toLongLong(),
                                  line.value(QStringLiteral("text")).toString());
                }
                translate.reserve(lyrics.size());
                for (const QVariant &value : std::as_const(lyrics)) {
                    const qlonglong time = value.toMap()
                                               .value(QStringLiteral("time")).toLongLong();
                    translate.append(byTime.value(time));
                }
            }

            QVariantMap detail;
            detail.insert(QStringLiteral("info"), lyrics);
            detail.insert(QStringLiteral("translate"), translate);
            detail.insert(QStringLiteral("hash"),
                          call.value(QStringLiteral("arg")).toMap()
                              .value(QStringLiteral("id")).toString());
            if (transportFailed)
                detail.insert(QStringLiteral("error"), transportError);
            return detail;
        } else {
            qWarning() << "[NeteaseCloudApi] 未知 action:" << action;
        }

        QVariantMap result = ApiCommon::listResult(info);
        if (action == QLatin1String("getComments"))
            result.insert(QStringLiteral("hasMore"), commentsHasMore);
        if (transportFailed)
            result.insert(QStringLiteral("error"), transportError);
        return result;
    }

    ApiHelper *m_api = nullptr;
};

NeteaseCloudApi::NeteaseCloudApi(QObject *parent)
    : QObject(parent)
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
    call.insert(QStringLiteral("member"), member);
    call.insert(QStringLiteral("arg"), arg);

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

void NeteaseCloudApi::searchSongs(const QString &keyword, int type, int page, int pageSize)
{
    int ntype = 1;
    if (type == 1)
        ntype = 1000;
    else if (type == 2)
        ntype = 10;
    else if (type == 3)
        ntype = 1006;

    enqueue(QStringLiteral("searchSongs"), QStringLiteral("cloudsearch"),
            {{QStringLiteral("keywords"), keyword},
             {QStringLiteral("type"), ntype},
             {QStringLiteral("limit"), pageSize},
             {QStringLiteral("offset"), (page - 1) * pageSize}},
            {{QStringLiteral("ntype"), ntype}});
}

void NeteaseCloudApi::getPlaylistMenu(int type)
{
    Q_UNUSED(type);
    enqueue(QStringLiteral("getPlaylistMenu"), QStringLiteral("playlist_catlist"), {});
}

void NeteaseCloudApi::getMusicPlaylists(const QString &tagid, int page, int pageSize)
{
    enqueue(QStringLiteral("getMusicPlaylists"), QStringLiteral("top_playlist"),
            {{QStringLiteral("cat"), tagid},
             {QStringLiteral("order"), QStringLiteral("hot")},
             {QStringLiteral("offset"), (page - 1) * pageSize},
             {QStringLiteral("limit"), pageSize}});
}

void NeteaseCloudApi::getPlaylistSongs(const QString &listid, int page, int pageSize)
{
    enqueue(QStringLiteral("getPlaylistSongs"), QStringLiteral("playlist_track_all"),
            {{QStringLiteral("id"), listid},
             {QStringLiteral("limit"), pageSize},
             {QStringLiteral("offset"), (page - 1) * pageSize}});
}

void NeteaseCloudApi::getRecommendSongs(int page, int pageSize)
{
    Q_UNUSED(page);
    enqueue(QStringLiteral("getRecommendSongs"), QStringLiteral("personalized_newsong"),
            {{QStringLiteral("limit"), pageSize}});
}

void NeteaseCloudApi::getHotPlaylistMenu(int type)
{
    Q_UNUSED(type);
    enqueue(QStringLiteral("getHotPlaylistMenu"), QStringLiteral("playlist_hot"), {});
}

void NeteaseCloudApi::getHotPlaylists(int page, int pageSize)
{
    enqueue(QStringLiteral("getHotPlaylists"), QStringLiteral("top_playlist"),
            {{QStringLiteral("cat"), QStringLiteral("全部")},
             {QStringLiteral("order"), QStringLiteral("hot")},
             {QStringLiteral("offset"), (page - 1) * pageSize},
             {QStringLiteral("limit"), pageSize}});
}

void NeteaseCloudApi::getNewSongs(int type, int page, int pageSize)
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
    enqueue(QStringLiteral("getNewSongs"), QStringLiteral("top_song"),
            {{QStringLiteral("type"), ntype},
             {QStringLiteral("limit"), pageSize},
             {QStringLiteral("offset"), (page - 1) * pageSize}});
}

void NeteaseCloudApi::getAllToplist()
{
    enqueue(QStringLiteral("getAllToplist"), QStringLiteral("toplist"), {});
}

void NeteaseCloudApi::getMusicToplist(int page, int pageSize, int rankid)
{
    enqueue(QStringLiteral("getMusicToplist"), QStringLiteral("top_list"),
            {{QStringLiteral("id"), rankid},
             {QStringLiteral("offset"), (page - 1) * pageSize},
             {QStringLiteral("limit"), pageSize}});
}

void NeteaseCloudApi::getHotSingers(int page, int pageSize)
{
    enqueue(QStringLiteral("getHotSingers"), QStringLiteral("top_artists"),
            {{QStringLiteral("offset"), (page - 1) * pageSize},
             {QStringLiteral("limit"), pageSize}});
}

void NeteaseCloudApi::getSingerCategory(int area, int page, int pageSize)
{
    enqueue(QStringLiteral("getSingerCategory"), QStringLiteral("artist_list"),
            {{QStringLiteral("offset"), (page - 1) * pageSize},
             {QStringLiteral("limit"), pageSize},
             {QStringLiteral("type"), QStringLiteral("1")},
             {QStringLiteral("area"), area}});
}

void NeteaseCloudApi::getSingerSongs(const QString &singerid, int page, int pageSize)
{
    Q_UNUSED(page);
    Q_UNUSED(pageSize);
    enqueue(QStringLiteral("getSingerSongs"), QStringLiteral("artist_top_song"),
            {{QStringLiteral("id"), singerid}});
}

void NeteaseCloudApi::getMusicInfo(const QString &hash, int type)
{
    enqueue(QStringLiteral("getMusicInfo"), QStringLiteral("song_detail"),
            {{QStringLiteral("ids"), hash},
             {QStringLiteral("id"), hash}},
            {{QStringLiteral("hash"), hash},
             {QStringLiteral("type"), type},
             {QStringLiteral("quality"), m_quality.load(std::memory_order_relaxed)}});
}

void NeteaseCloudApi::getLyricInfo(const QString &hash, int duration)
{
    Q_UNUSED(duration);
    enqueue(QStringLiteral("getLyricInfo"), QStringLiteral("lyric_new"),
            {{QStringLiteral("id"), hash}});
}

void NeteaseCloudApi::getComments(const QString &hash, int page, int pageSize)
{
    enqueue(QStringLiteral("getComments"), QStringLiteral("comment_music"),
            {{QStringLiteral("id"), hash},
             {QStringLiteral("limit"), pageSize},
             {QStringLiteral("offset"), qMax(0, (page - 1) * pageSize)}});
}

void NeteaseCloudApi::getPersonalFm(int page, int pageSize)
{
    Q_UNUSED(page);
    Q_UNUSED(pageSize);
    enqueue(QStringLiteral("getPersonalFm"), QStringLiteral("personal_fm"), {});
}

void NeteaseCloudApi::getPersonalRadar(int page, int pageSize)
{
    Q_UNUSED(pageSize);
    if (page > 1) {
        emit resultReady(QStringLiteral("getPersonalRadar"),
                         ApiCommon::listResult(QVariantList()), kSource);
        return;
    }
    enqueue(QStringLiteral("getPersonalRadar"), QStringLiteral("recommend_songs"), {});
}

#include "NeteaseCloudApi.moc"