// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#include "BilibiliApi.h"

#include "ApiCommon.h"
#include "ApiHttp.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QUrl>

#include <algorithm>
#include <climits>

namespace {
constexpr int kMusicTid = 3;
constexpr int kUpLimit = 5;
const char *kUa = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
                  "(KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36";
const QString kApi = "https://api.bilibili.com";
const QString kSearchPath = "/x/web-interface/wbi/search/type";

const int kMixinTab[64] = {46, 47, 18, 2, 53, 8, 23, 32, 15, 50, 10, 31, 58, 3, 45, 35,
                           27, 43, 5, 49, 33, 9, 42, 19, 29, 28, 14, 39, 12, 38, 41, 13,
                           37, 48, 7, 16, 24, 55, 40, 61, 26, 17, 0, 1, 60, 51, 30, 4,
                           22, 25, 54, 21, 56, 59, 6, 63, 57, 62, 11, 36, 20, 34, 44, 52};

struct Region {
    const char *name;
    int tid;
};
const Region kRegions[] = {
    {"音乐综合", 3},   {"原创音乐", 28}, {"翻唱", 31},      {"VOCALOID", 30},
    {"演奏", 59},      {"电音", 194},    {"三次元音乐", 29}, {"乐评盘点", 243},
    {"AI音乐", 265},
};

QString regionName(int tid)
{
    for (const Region &r : kRegions) {
        if (r.tid == tid)
            return QString::fromUtf8(r.name);
    }
    return QString();
}

QString cleanTitle(QString t)
{
    static const QRegularExpression tag("<[^>]*>");
    t.remove(tag);
    return t.trimmed();
}

QString httpsUrl(QString u)
{
    if (u.startsWith(QLatin1String("//")))
        return "https:" + u;
    if (u.startsWith(QLatin1String("http://")))
        u.replace(0, 7, "https://");
    return u;
}

int durationSeconds(const QJsonValue &v)
{
    if (v.isDouble())
        return v.toInt();
    int total = 0;
    for (const QString &part : v.toString().split(QLatin1Char(':')))
        total = total * 60 + part.toInt();
    return total;
}

QJsonObject pickAudio(const QJsonArray &audios, int quality)
{
    const int target = quality >= 2 ? 30251 : (quality == 1 ? 30280 : 30216);
    QJsonObject best = audios.first().toObject();
    int bestScore = INT_MAX;
    for (const QJsonValue &v : audios) {
        const QJsonObject o = v.toObject();
        const int score = qAbs(o.value("id").toInt() - target);
        if (score < bestScore) {
            bestScore = score;
            best = o;
        }
    }
    return best;
}
}

BilibiliApi::BilibiliApi(QObject *parent)
    : MusicSource(ApiSource::Bilibili, parent)
{
    m_nam = new QNetworkAccessManager(this);
}

void BilibiliApi::applyCredentials(const SourceCredentials &credentials)
{
    if (m_credentials == credentials)
        return;

    const bool cookieChanged = m_credentials.cookie != credentials.cookie;
    m_credentials = credentials;
    setQuality(credentials.quality);
    if (cookieChanged)
        setCookie(credentials.cookie);
}

QString BilibiliApi::mixinKey(const QString &imgKey, const QString &subKey)
{
    const QString raw = imgKey + subKey;
    if (raw.size() < 64)
        return QString();
    QString out;
    out.reserve(32);
    for (int i = 0; i < 32; ++i)
        out.append(raw.at(kMixinTab[i]));
    return out;
}

void BilibiliApi::onComments(const QString &hash, int page, int pageSize)
{

    const auto requestComments = [this, hash, page, pageSize](qint64 aid) {

        QVariantMap params;
        params.insert("type", "1");
        params.insert("oid", QString::number(aid));
        params.insert("mode", "3");
        params.insert("next",
                      QString::number(page <= 1 ? 0 : m_commentCursors.value(hash)));
        params.insert("ps", QString::number(qBound(1, pageSize, 30)));
        params.insert("web_location", "1315875");

        getSigned("/x/v2/reply/wbi/main", params,
                  [this, hash](const QJsonObject &obj) {
                      const QJsonObject data = obj.value("data").toObject();
                      const QJsonObject cursor = data.value("cursor").toObject();
                      if (m_commentCursors.size() > 2000)
                          m_commentCursors.clear();
                      m_commentCursors.insert(hash,
                                              cursor.value("next")
                                                  .toVariant().toLongLong());
                      QVariantList info;
                      for (const QJsonValue &value : data.value("replies").toArray()) {
                          const QJsonObject c = value.toObject();
                          const QJsonObject member = c.value("member").toObject();
                          info << ApiCommon::comment(
                              member.value("uname").toString(),
                              member.value("avatar").toString(),
                              c.value("content").toObject()
                                  .value("message").toString(),
                              c.value("ctime").toVariant().toLongLong(),
                              c.value("like").toInt(),
                              c.value("rcount").toInt());
                      }
                      QVariantMap result = ApiCommon::listResult(info);

                      result.insert("hasMore",
                                    !cursor.value("is_end").toBool());
                      if (obj.contains("error"))
                          result.insert("error", obj.value("error"));
                      emit resultReady("getComments", result, Source);
                  },
                  !m_loginCookie.isEmpty());
    };

    const qint64 cachedAid = m_aidCache.value(hash);
    if (cachedAid > 0) {
        requestComments(cachedAid);
        return;
    }
    get("https://api.bilibili.com/x/web-interface/view?bvid=" + hash,
        [this, hash, requestComments](const QJsonObject &json) {
            const qint64 aid = json.value("data").toObject()
                                   .value("aid").toVariant().toLongLong();
            if (aid <= 0) {
                emitList("getComments", {});
                return;
            }
            if (m_aidCache.size() > 2000)
                m_aidCache.clear();
            m_aidCache.insert(hash, aid);
            requestComments(aid);
        });
}

void BilibiliApi::get(const QString &url, const Callback &cb, bool withCookie)
{
    QByteArray cookie;
    if (withCookie && !m_buvid3.isEmpty()) {
        cookie = QString("buvid3=%1; buvid4=%2").arg(m_buvid3, m_buvid4).toUtf8();
        if (!m_loginCookie.isEmpty()) {
            cookie += ';';
            cookie += m_loginCookie.toUtf8();
        }
    }
    ApiHttp::getJson(m_nam, this, QUrl(url), kUa, "https://www.bilibili.com", cookie, cb);
}

void BilibiliApi::ensureKeys(const Task &then)
{
    if (!m_mixinKey.isEmpty() && !m_buvid3.isEmpty()) {
        then();
        return;
    }
    if (m_keyRequesting) {
        m_keyWaiters.append(then);
        return;
    }
    m_keyRequesting = true;
    get("https://api.bilibili.com/x/web-interface/nav",
        [this, then](const QJsonObject &j) {
            const QJsonObject img = j.value("data").toObject()
                                        .value("wbi_img").toObject();
            const auto baseName = [](const QString &u) {
                return u.section(QLatin1Char('/'), -1).section(QLatin1Char('.'), 0, 0);
            };
            m_mixinKey = mixinKey(baseName(img.value("img_url").toString()),
                                  baseName(img.value("sub_url").toString()));
            get("https://api.bilibili.com/x/frontend/finger/spi",
                [this, then](const QJsonObject &j2) {
                    const QJsonObject d = j2.value("data").toObject();
                    m_buvid3 = d.value("b_3").toString();
                    m_buvid4 = d.value("b_4").toString();
                    m_keyRequesting = false;

                    then();
                    const QList<Task> waiters = m_keyWaiters;
                    m_keyWaiters.clear();
                    for (const Task &t : waiters)
                        t();
                });
        });
}

void BilibiliApi::getSigned(const QString &path, QVariantMap params, const Callback &cb,
                            bool withCookie)
{
    ensureKeys([this, path, params, cb, withCookie]() mutable {
        if (m_mixinKey.isEmpty()) {
            cb(QJsonObject());
            return;
        }
        static const QRegularExpression strip("[!'()*]");
        params.insert("wts",
                      QString::number(QDateTime::currentSecsSinceEpoch()));
        QStringList keys = params.keys();
        std::sort(keys.begin(), keys.end());
        QStringList pairs;
        pairs.reserve(keys.size());
        for (const QString &k : keys) {
            QString v = params.value(k).toString();
            v.remove(strip);
            pairs << QString("%1=%2")
                         .arg(QString::fromUtf8(QUrl::toPercentEncoding(k)),
                              QString::fromUtf8(QUrl::toPercentEncoding(v)));
        }
        const QString query = pairs.join(QLatin1Char('&'));
        const QByteArray sign = QCryptographicHash::hash((query + m_mixinKey).toUtf8(),
                                                         QCryptographicHash::Md5).toHex();
        get(kApi + path + QLatin1Char('?') + query + "&w_rid="
                + QString::fromLatin1(sign),
            cb, withCookie);
    });
}

void BilibiliApi::rememberUpName(const QString &mid, const QString &name)
{
    if (mid.isEmpty())
        return;
    if (m_upNames.size() > 2000)
        m_upNames.clear();
    m_upNames.insert(mid, name);
}

QVariantMap BilibiliApi::toSong(const QJsonObject &v, const QString &fallbackArtist)
{
    const QJsonObject owner = v.value("owner").toObject();
    const QJsonValue midVal = owner.isEmpty() ? v.value("mid")
                                             : owner.value("mid");
    const QString mid = midVal.toVariant().toString();
    QString artist = owner.isEmpty() ? v.value("author").toString()
                                     : owner.value("name").toString();
    if (artist.isEmpty())
        artist = fallbackArtist;

    qint64 play = v.value("stat").toObject()
                      .value("view").toVariant().toLongLong();
    if (play <= 0)
        play = v.value("play").toVariant().toLongLong();

    if (!artist.isEmpty())
        rememberUpName(mid, artist);
    return ApiCommon::song(cleanTitle(v.value("title").toString()), artist,
                           httpsUrl(v.value("pic").toString()),
                           v.value("bvid").toString(),
                           durationSeconds(v.value("duration")),
                           QString(), QString(), QString(), 0, play);
}

QStringList BilibiliApi::upMids(const QJsonArray &items, int limit)
{
    QStringList mids;
    for (const QJsonValue &v : items) {
        const QJsonObject o = v.toObject();
        const QJsonObject owner = o.value("owner").toObject();
        const QString mid = (owner.isEmpty() ? o.value("mid")
                                             : owner.value("mid"))
                                .toVariant().toString();
        if (mid.isEmpty() || mids.contains(mid))
            continue;
        rememberUpName(mid, owner.isEmpty() ? o.value("author").toString()
                                            : owner.value("name").toString());
        mids << mid;
        if (limit > 0 && mids.size() >= limit)
            break;
    }
    return mids;
}

void BilibiliApi::emitList(const QString &action, const QVariantList &info)
{
    emit resultReady(action, ApiCommon::listResult(info), Source);
}

void BilibiliApi::emitLyrics(const QString &hash, const QVariantList &lyrics, int error)
{
    QVariantMap data{{"info", lyrics},
                     {"translate", QVariantList()},
                     {"hash", hash}};
    if (error != 0)
        data.insert("error", error);
    emit resultReady("getLyricInfo", data, Source);
}

void BilibiliApi::searchVideos(const QString &keyword, int tid, int page, const QString &action,
                               const QString &order)
{
    if (keyword.trimmed().isEmpty()) {
        emitList(action, {});
        return;
    }
    QVariantMap params;
    params.insert("search_type", "video");
    params.insert("keyword", keyword);
    params.insert("page", page);
    params.insert("tids", tid);
    if (!order.isEmpty())
        params.insert("order", order);
    getSigned(kSearchPath, params, [this, action](const QJsonObject &j) {
        QVariantList info;
        for (const QJsonValue &v : j.value("data").toObject()
                                       .value("result").toArray())
            info << toSong(v.toObject());
        emitList(action, info);
    });
}

void BilibiliApi::searchPlaylists(const QString &keyword, int page, int limit,
                                  const QString &action)
{
    if (keyword.trimmed().isEmpty()) {
        emitList(action, {});
        return;
    }
    QVariantMap params;
    params.insert("search_type", "video");
    params.insert("keyword", keyword);
    params.insert("page", page);
    params.insert("tids", kMusicTid);
    getSigned(kSearchPath, params, [this, action, limit](const QJsonObject &j) {
        seasonsOf(action, upMids(j.value("data").toObject()
                                     .value("result").toArray(),
                                 kUpLimit),
                  limit);
    });
}

void BilibiliApi::seasonsOf(const QString &action, const QStringList &mids, int limit)
{
    if (mids.isEmpty()) {
        emitList(action, {});
        return;
    }
    auto st = QSharedPointer<PlaylistSearch>::create();
    st->action = action;
    st->limit = limit;
    st->pending = mids.size();
    for (const QString &mid : mids)
        fetchSeasons(mid, st);
}

void BilibiliApi::fetchSeasons(const QString &mid, const QSharedPointer<PlaylistSearch> &st)
{
    get(kApi + QStringLiteral("/x/polymer/web-space/home/seasons_series"
                              "?mid=%1&page_num=1&page_size=10")
                  .arg(mid),
        [this, mid, st](const QJsonObject &j) {
            const QJsonArray seasons = j.value("data").toObject()
                                           .value("items_lists").toObject()
                                           .value("seasons_list").toArray();
            for (const QJsonValue &v : seasons) {
                const QJsonObject meta = v.toObject().value("meta").toObject();
                const QString sid = meta.value("season_id").toVariant().toString();
                const int total = meta.value("total").toInt();

                if (sid.isEmpty() || total < 2 || st->seen.contains(sid))
                    continue;
                st->seen.insert(sid);
                st->items << ApiCommon::song(
                    meta.value("name").toString(), m_upNames.value(mid),
                    httpsUrl(meta.value("cover").toString()),
                    QString("season:%1:%2").arg(sid, mid), total,
                    meta.value("description").toString());
            }
            if (--st->pending > 0)
                return;
            if (st->limit > 0 && st->items.size() > st->limit)
                st->items = st->items.mid(0, st->limit);
            emitList(st->action, st->items);
        });
}

void BilibiliApi::seasonSongs(const QString &sid, const QString &mid, int page, int pageSize)
{
    get(kApi + QStringLiteral("/x/polymer/web-space/seasons_archives_list"
                              "?mid=%1&season_id=%2&sort_reverse=false&page_num=%3&page_size=%4")
                  .arg(mid, sid)
                  .arg(qMax(page, 1))
                  .arg(qMax(pageSize, 1)),
        [this, mid](const QJsonObject &j) {
            QVariantList info;
            const QString artist = m_upNames.value(mid);
            for (const QJsonValue &v : j.value("data").toObject()
                                           .value("archives").toArray())
                info << toSong(v.toObject(), artist);
            emitList("getPlaylistSongs", info);
        });
}

void BilibiliApi::ranking(int rid, int page, int pageSize, const QString &action)
{
    get(kApi + QString("/x/web-interface/ranking/v2?rid=%1&type=all").arg(rid),
        [this, page, pageSize, action](const QJsonObject &j) {
            const QJsonArray arr = j.value("data").toObject()
                                       .value("list").toArray();
            const int offset = (qMax(page, 1) - 1) * qMax(pageSize, 1);
            QVariantList info;
            for (int i = offset; i < arr.size() && i < offset + pageSize; ++i)
                info << toSong(arr.at(i).toObject());
            emitList(action, info);
        });
}

void BilibiliApi::newVideos(int page, int pageSize, const QString &action)
{
    get(kApi + QString("/x/web-interface/newlist?rid=%1&type=0&pn=%2&ps=%3")
                  .arg(kMusicTid)
                  .arg(qMax(page, 1))
                  .arg(qMax(pageSize, 1)),
        [this, action](const QJsonObject &j) {
            QVariantList info;
            for (const QJsonValue &v : j.value("data").toObject()
                                           .value("archives").toArray())
                info << toSong(v.toObject());
            emitList(action, info);
        });
}

void BilibiliApi::rankingUsers(const QString &action, int limit)
{
    get(kApi + QString("/x/web-interface/ranking/v2?rid=%1&type=all").arg(kMusicTid),
        [this, action, limit](const QJsonObject &j) {
            QVariantList info;
            QSet<QString> seen;
            for (const QJsonValue &v : j.value("data").toObject()
                                           .value("list").toArray()) {
                const QJsonObject owner = v.toObject().value("owner").toObject();
                const QString mid = owner.value("mid").toVariant().toString();
                if (mid.isEmpty() || seen.contains(mid))
                    continue;
                seen.insert(mid);
                const QString name = owner.value("name").toString();
                rememberUpName(mid, name);
                info << ApiCommon::song(name, QString(),
                                        httpsUrl(owner.value("face").toString()),
                                        mid);
                if (limit > 0 && info.size() >= limit)
                    break;
            }
            emitList(action, info);
        });
}

void BilibiliApi::onSearchSongs(const QString &keyword, int type, int page, int pageSize)
{
    if (type == 0)
        searchVideos(keyword, kMusicTid, page, "searchSongs",
                     "totalrank");
    else if (type == 1)
        searchPlaylists(keyword, page, pageSize, "searchSongs");
    else
        emitList("searchSongs", {});
}

void BilibiliApi::onSearchLyrics(const QString &keyword, int page, int pageSize)
{
    Q_UNUSED(pageSize)
    searchVideos(keyword, kMusicTid, page, ApiAction::SearchLyrics,
                 "totalrank");
}

void BilibiliApi::onPlaylistMenu(int type)
{
    Q_UNUSED(type);
    QVariantList info;
    for (const Region &r : kRegions) {
        const QString name = QString::fromUtf8(r.name);
        info << QVariantMap{{"title", name},
                            {"id", QString::number(r.tid)},
                            {"category", name}};
    }
    emitList("getPlaylistMenu", info);
}

void BilibiliApi::onMusicPlaylists(const QString &tagid, int page, int pageSize)
{
    Q_UNUSED(pageSize);
    bool ok = false;
    const int tid = tagid.toInt(&ok);
    searchVideos(QStringLiteral("音乐"), ok && tid > 0 ? tid : kMusicTid, page,
                 "getMusicPlaylists", "click");
}

void BilibiliApi::onCategoryPlaylists(const QString &id, int page, int pageSize)
{
    // B 站没有独立分类接口，按分区搜索即可。
    onMusicPlaylists(id, page, pageSize);
}

void BilibiliApi::onPlaylistSongs(const QString &listid, int page, int pageSize)
{
    const QStringList parts = listid.split(QLatin1Char(':'));
    if (parts.size() == 3 && parts.first() == QLatin1String("season")) {
        seasonSongs(parts.at(1), parts.at(2), page, pageSize);
        return;
    }
    if (listid.isEmpty()) {
        emitList("getPlaylistSongs", {});
        return;
    }
    get(kApi + "/x/web-interface/view?bvid=" + listid,
        [this](const QJsonObject &j) {
            const QJsonObject d = j.value("data").toObject();
            QVariantList info;
            if (!d.isEmpty())
                info << toSong(d);
            emitList("getPlaylistSongs", info);
        });
}

void BilibiliApi::onRecommendSongs(int page, int pageSize)
{
    ranking(kMusicTid, page, pageSize, "getRecommendSongs");
}

void BilibiliApi::onHotPlaylistMenu(int type)
{
    Q_UNUSED(type);
    QVariantList info;
    for (const Region &r : kRegions) {
        const QString tid = QString::number(r.tid);
        info << QVariantMap{{"title", QString::fromUtf8(r.name)},
                            {"id", tid},
                            {"tagid", tid},
                            {"cover", QString()}};
    }
    emitList("getHotPlaylistMenu", info);
}

void BilibiliApi::onHotPlaylists(int page, int pageSize)
{
    const QString action = "getHotPlaylists";
    if (page > 1) {
        emitList(action, {});
        return;
    }
    get(kApi + QString("/x/web-interface/ranking/v2?rid=%1&type=all").arg(kMusicTid),
        [this, action, pageSize](const QJsonObject &j) {
            seasonsOf(action, upMids(j.value("data").toObject()
                                         .value("list").toArray(),
                                     kUpLimit),
                      pageSize);
        });
}

void BilibiliApi::onNewSongs(int type, int page, int pageSize)
{
    Q_UNUSED(type);
    newVideos(page, pageSize, "getNewSongs");
}

void BilibiliApi::onAllToplist()
{
    QVariantList info;
    for (const Region &r : kRegions) {
        const QString name = QString::fromUtf8(r.name);
        info << ApiCommon::song(name + QStringLiteral("榜"), QStringLiteral("哔哩哔哩"),
                                QString(), QString::number(r.tid), 0, name);
    }
    emitList("getAllToplist", info);
}

void BilibiliApi::onMusicToplist(int page, int pageSize, int rankid)
{
    const QString name = regionName(rankid);
    if (name.isEmpty() || rankid == kMusicTid)
        ranking(kMusicTid, page, pageSize, "getMusicToplist");
    else
        searchVideos(name, rankid, page, "getMusicToplist");
}

void BilibiliApi::onHotSingers(int page, int pageSize)
{
    Q_UNUSED(page);
    rankingUsers("getHotSingers", pageSize);
}

void BilibiliApi::onSingerCategory(int area, int page, int pageSize)
{
    Q_UNUSED(area);
    Q_UNUSED(page);
    rankingUsers("getSingerCategory", pageSize);
}

void BilibiliApi::onSingerSongs(const QString &singerid, int page, int pageSize)
{
    Q_UNUSED(pageSize);
    const QString name = m_upNames.value(singerid);
    if (name.isEmpty()) {
        emitList("getSingerSongs", {});
        return;
    }
    QVariantMap params;
    params.insert("search_type", "video");
    params.insert("keyword", name);
    params.insert("page", page);
    params.insert("tids", kMusicTid);
    getSigned(kSearchPath, params, [this, singerid](const QJsonObject &j) {
        QVariantList info;
        for (const QJsonValue &v : j.value("data").toObject()
                                       .value("result").toArray()) {
            const QJsonObject o = v.toObject();
            if (o.value("mid").toVariant().toString() != singerid)
                continue;
            info << toSong(o);
        }
        emitList("getSingerSongs", info);
    });
}

void BilibiliApi::onMusicInfo(const QString &hash, int type)
{
    const auto fail = [this, hash, type](int error = 0) {
        QVariantMap data{{"hash", hash},
                         {"type", type},
                         {"errReason", "unavailable"}};
        if (error != 0)
            data.insert("error", error);
        emit resultReady("getMusicInfo", data, Source);
    };
    if (hash.isEmpty()) {
        fail();
        return;
    }

    get(kApi + "/x/web-interface/view?bvid=" + hash,
        [this, hash, type, fail](const QJsonObject &j) {
            if (j.contains("error")) {
                fail(j.value("error").toInt());
                return;
            }
            const QJsonObject d = j.value("data").toObject();
            const QString cid = d.value("cid").toVariant().toString();
            if (cid.isEmpty()) {
                fail();
                return;
            }
            get(kApi + QString("/x/player/playurl?bvid=%1&cid=%2&fnval=16&fourk=1")
                          .arg(hash, cid),
                [this, hash, type, d, fail](const QJsonObject &pj) {
                    if (pj.contains("error")) {
                        fail(pj.value("error").toInt());
                        return;
                    }
                    const QJsonObject pd = pj.value("data").toObject();
                    const QJsonArray audios = pd.value("dash").toObject()
                                                  .value("audio").toArray();
                    QString url;
                    bool fromDurl = false;
                    if (!audios.isEmpty()) {
                        const QJsonObject best = pickAudio(audios, m_quality);
                        url = httpsUrl(best.value("baseUrl").toString());
                        if (url.isEmpty())
                            url = httpsUrl(best.value("base_url").toString());
                    }
                    if (url.isEmpty()) {
                        const QJsonArray durl = pd.value("durl").toArray();
                        if (!durl.isEmpty()) {
                            url = httpsUrl(durl.first().toObject()
                                               .value("url").toString());
                            fromDurl = !url.isEmpty();
                        }
                    }
                    if (url.isEmpty()) {
                        fail();
                        return;
                    }

                    const QJsonObject owner = d.value("owner").toObject();
                    const QString title = cleanTitle(d.value("title").toString());
                    emit resultReady(
                        "getMusicInfo",
                        QVariantMap{
                            {"backup_url", url},
                            {"url", url},
                            {"songName", title},
                            {"author_name",
                             owner.value("name").toString()},
                            {"singer_img",
                             httpsUrl(owner.value("face").toString())},
                            {"album_img",
                             httpsUrl(d.value("pic").toString())},
                            {"timeLength",
                             d.value("duration").toInt()},
                            {"fileName",
                             title + (fromDurl ? ".mp4" : ".m4a")},
                            {"hash", hash},
                            {"type", type}},
                        Source);
                });
        });
}

void BilibiliApi::onLyricInfo(const QString &hash, int duration)
{
    Q_UNUSED(duration);
    if (hash.isEmpty()) {
        emitLyrics(hash, {});
        return;
    }
    get(kApi + "/x/web-interface/view?bvid=" + hash,
        [this, hash](const QJsonObject &j) {
            const int viewError = j.value("error").toInt();
            const QJsonObject d = j.value("data").toObject();
            const QString cid = d.value("cid").toVariant().toString();
            if (cid.isEmpty()) {
                emitLyrics(hash, {}, viewError);
                return;
            }
            get(kApi + QString("/x/player/v2?bvid=%1&cid=%2").arg(hash, cid),
                [this, hash](const QJsonObject &pj) {
                    const int playerError = pj.value("error").toInt();
                    const QJsonArray subs = pj.value("data").toObject()
                                                .value("subtitle").toObject()
                                                .value("subtitles").toArray();
                    QString subUrl;
                    for (const QJsonValue &v : subs) {
                        const QString u = v.toObject()
                                              .value("subtitle_url").toString();
                        if (!u.isEmpty()) {
                            subUrl = httpsUrl(u);
                            break;
                        }
                    }
                    if (subUrl.isEmpty()) {
                        emitLyrics(hash, {}, playerError);
                        return;
                    }
                    get(subUrl, [this, hash](const QJsonObject &sj) {
                        QVariantList lyrics;
                        for (const QJsonValue &v : sj.value("body").toArray()) {
                            const QJsonObject l = v.toObject();
                            const QString text = l.value("content")
                                                     .toString().trimmed();
                            if (text.isEmpty())
                                continue;
                            lyrics << QVariantMap{
                                {"time",
                                 int(l.value("from").toDouble() * 1000)},
                                {"text", text}};
                        }
                        emitLyrics(hash, lyrics, sj.value("error").toInt());
                    });
                });
        });
}

void BilibiliApi::onPersonalFm(int page, int pageSize)
{
    ranking(kMusicTid, page, pageSize, "getPersonalFm");
}

void BilibiliApi::onPersonalRadar(int page, int pageSize)
{
    newVideos(page, pageSize, "getPersonalRadar");
}
