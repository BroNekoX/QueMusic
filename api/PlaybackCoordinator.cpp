// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#include "PlaybackCoordinator.h"

#include "ApiActions.h"
#include "ApiCommon.h"
#include "LocalLyricsReader.h"
#include "QualityHashCache.h"


#include <QDebug>
#include <QTimer>
#include <QFutureWatcher>
#include <QtConcurrent>

#include <algorithm>

namespace {

QString sizedCover(const QVariantMap &data, int size)
{
    QString cover = data.value("album_img").toString();
    cover.replace(QLatin1String("{size}"), QString::number(size));
    return cover;
}

int durationMsOf(const QVariantMap &data)
{
    const double length = data.value("timeLength").toDouble();
    return int(length * (length < 1000 ? 1000 : 1));
}

QVariantList placeholderLyrics()
{
    return {QVariantMap{{"time", 0}, {"text", QStringLiteral("纯音乐，请欣赏")}}};
}

} // namespace

PlaybackCoordinator::PlaybackCoordinator(QualityHashCache *qualityCache, QObject *parent)
    : QObject(parent), m_qualityCache(qualityCache)
{
}

QString PlaybackCoordinator::keywordOf(const QString &title, const QString &artist)
{
    return artist.isEmpty() ? title : title + QLatin1Char(' ') + artist;
}

void PlaybackCoordinator::beginPlay(const QString &hash, int source)
{
    ++m_playGeneration;
    m_playHash = hash;
    m_playSource = source;
    stopChain();
    m_chainLow = false;
    m_chainTrialUrl.clear();
}

void PlaybackCoordinator::invalidate()
{
    ++m_playGeneration;
    m_playHash = "\x1f";
    m_playSource = -1;
}

bool PlaybackCoordinator::isStalePlayResponse(const QString &playHash, int source) const
{
    return !playHash.isEmpty() && !m_playHash.isEmpty() && (playHash != m_playHash || source != m_playSource);
}

void PlaybackCoordinator::handleMusicInfo(const QVariantMap &data, int source)
{
    const int type = data.value("type").toInt();
    const QString playHash = data.value("hash").toString();
    const QString playUrl = ApiCommon::firstNonEmpty(data, {"url", "backup_url"});
    const bool fromPlugin = source >= ApiSource::PluginBase;

    if (type == 0 && !fromPlugin && isStalePlayResponse(playHash, source))
        return;

    if (type == 2) {
        emit playUrlReady(m_qualityCache->baseOf(playHash), playUrl,
                          data.value("songName").toString(),
                          data.value("author_name").toString(),
                          sizedCover(data, 512), durationMsOf(data), source);
        return;
    }

    if (fromPlugin) {
        m_chainPending.remove(source);
        if (!playUrl.isEmpty()) {
            stopChain();
        } else if (m_chainActive && m_chainPending.isEmpty()) {
            finishChain();
            return;
        } else {
            return;
        }
    }

    // 试听片段当作“没有地址”，先让音源插件试；都不行再退回试听
    const bool trial = type == 0 && ApiCommon::isTrial(data);
    if (type == 0 && ApiSource::isBuiltin(source) && !m_chainActive && (playUrl.isEmpty() || trial)) {
        if (m_chainLow) {
            finishChain();
            return;
        }
        startChain(data, playHash, source);
        return;
    }
    if (playUrl.isEmpty()) {
        failPlay(data, playHash, source);
        return;
    }

    QVariantMap payload = data;
    int effectiveSource = source;
    if (fromPlugin) {
        payload = m_chainData;
        payload.insert("url", playUrl);
        payload.insert("backup_url", playUrl);
        payload.insert("hash", playHash);
        payload.insert("type", type);
        payload.remove("errReason");
        payload.remove("error");
        effectiveSource = m_chainSource;
    }
    playResolved(payload, effectiveSource, playHash, type);
}

void PlaybackCoordinator::playResolved(const QVariantMap &payload, int source,
                                       const QString &playHash, int type)
{
    const QString playUrl = ApiCommon::firstNonEmpty(payload, {"url", "backup_url"});
    const QString outName = payload.value("songName").toString();
    const QString outAuthor = payload.value("author_name").toString();
    const QString identityHash = m_qualityCache->baseOf(playHash);
    const double timeLength = payload.value("timeLength").toDouble();
    const int durationSeconds = int(timeLength);
    const int timeMs = int(timeLength * (timeLength < 1000 ? 1000 : 1));

    if (source == ApiSource::Bilibili)
        m_biliTrack = BiliTrack{playHash, outName, outAuthor, durationSeconds};

    if (type == 0) {
        QString solve = payload.value("album_img").toString();
        solve.replace(QLatin1String("{size}"), "128");
        emit urlplay(playUrl, outName, outAuthor, sizedCover(payload, 512), solve, identityHash,
                     source);

        if (m_lyricGenerations.size() > 32) {
            for (auto it = m_lyricGenerations.begin(); it != m_lyricGenerations.end();) {
                if (it.value() != m_playGeneration)
                    it = m_lyricGenerations.erase(it);
                else
                    ++it;
            }
        }
        m_lyricGenerations.insert(playHash, m_playGeneration);
        emit lyricRequested(playHash, timeMs, source);
    } else if (type == 1) {
        QVariantMap meta;
        meta.insert("title", outName);
        meta.insert("artist", outAuthor);
        meta.insert("cover", sizedCover(payload, 512));
        meta.insert("duration", durationSeconds);
        meta.insert("hash", playHash);
        meta.insert("url", playUrl);
        meta.insert("fileName", payload.value("fileName").toString());

        if (m_pendingDownloads.size() > 32)
            m_pendingDownloads.clear();
        m_pendingDownloads.insert(playHash, meta);
        emit lyricRequested(playHash, timeMs, source);
    }
}

void PlaybackCoordinator::startChain(const QVariantMap &data, const QString &hash, int source)
{
    m_chainData = data;
    m_chainHash = hash;
    m_chainSource = source;
    m_chainActive = true;
    m_chainLow = false;
    m_chainTrialUrl = ApiCommon::isTrial(data)
                          ? ApiCommon::firstNonEmpty(data, {"url", "backup_url"})
                          : QString();

    m_chainPending.clear();
    for (int id : std::as_const(m_sourcePlugins)) {
        if (id > 0)
            m_chainPending.insert(id);
    }
    if (m_chainPending.isEmpty()) {
        finishChain();
        return;
    }

    if (!m_chainTimer) {
        m_chainTimer = new QTimer(this);
        m_chainTimer->setSingleShot(true);
        connect(m_chainTimer, &QTimer::timeout, this, [this] {
            if (m_chainActive)
                finishChain();
        });
    }
    m_chainTimer->start(5000);

    const QVariantMap args{{ApiArg::Hash, hash}, {ApiArg::Id, hash}, {ApiArg::Type, 0},
                           {QStringLiteral("platform"), source},
                           {QStringLiteral("quality"), m_soundQuality}};
    const QSet<int> pending = m_chainPending;
    for (int id : pending)
        emit searchRequested(ApiAction::MusicInfo, args, id);
}

void PlaybackCoordinator::stopChain()
{
    m_chainActive = false;
    m_chainPending.clear();
    if (m_chainTimer)
        m_chainTimer->stop();
}

void PlaybackCoordinator::finishChain()
{
    stopChain();
    if (!m_chainLow) {
        m_chainLow = true;
        emit lowQualityRequested(m_chainHash, m_chainSource);
        return;
    }
    if (!m_chainTrialUrl.isEmpty()) {
        QVariantMap payload = m_chainData;
        payload.insert("url", m_chainTrialUrl);
        payload.insert("backup_url", m_chainTrialUrl);
        payload.remove("errReason");
        playResolved(payload, m_chainSource, m_chainHash, 0);
        emit warned(QStringLiteral("版权限制，仅能试听 30 秒"), 1);
        return;
    }
    failPlay(m_chainData, m_chainHash, m_chainSource);
}

void PlaybackCoordinator::failPlay(const QVariantMap &data, const QString &hash, int source)
{
    stopChain();
    m_chainLow = false;
    const bool networkFailed = data.contains("error");
    const QString reason = data.value("errReason").toString();
    const bool kugouNotLoggedIn = source == ApiSource::Kugou && !m_kugouLoggedIn;
    const QString message = networkFailed
        ? QStringLiteral("网络请求失败，请检查网络后重试")
        : reason == QLatin1String("vip")
            ? (kugouNotLoggedIn
                   ? QStringLiteral("该歌曲需要 VIP，请登录酷狗或启用音源插件")
                   : QStringLiteral("该歌曲需要 VIP，可登录对应平台或启用音源插件"))
            : (kugouNotLoggedIn
                   ? QStringLiteral("暂无可用播放地址，可登录酷狗或启用音源插件")
                   : QStringLiteral("该歌曲受版权限制，可启用音源插件后重试"));
    qDebug() << "[api] 无法播放:" << message << "hash:" << hash.left(8);
    emit playFailed(m_qualityCache->baseOf(hash), source);
    emit warned(message, 2);
}

void PlaybackCoordinator::handleLyricInfo(const QVariantMap &data, int source)
{
    const QString lyricHash = data.value("hash").toString();

    if (m_lyricGenerations.contains(lyricHash)) {
        const int generation = m_lyricGenerations.take(lyricHash);
        if (generation != m_playGeneration) {
            qDebug() << "[api] 丢弃过期歌词响应 hash:" << lyricHash.left(8);
            return;
        }
    }

    const bool failed = data.contains("error");
    const QVariantList onlineLyrics = data.value("info").toList();
    const QVariantList translate = data.value("translate").toList();

    if (failed) {
        emit warned(QStringLiteral("歌词加载失败，请检查网络后重试"), 0);
        if (!lyricHash.isEmpty()) {
            const auto it = m_pendingLocalLyrics.find(lyricHash);
            if (it != m_pendingLocalLyrics.end()) {
                emit localLyricsFailed(it.value().filePath);
                m_pendingLocalLyrics.erase(it);
            }
        }
    } else {
        // B 站视频没有字幕时，回退到默认在线音源按标题搜词。
        const bool biliNoSubtitle = source == ApiSource::Bilibili && !lyricHash.isEmpty()
                                    && lyricHash == m_biliTrack.hash;
        if (biliNoSubtitle && onlineLyrics.isEmpty())
            findOnlineLyrics(m_biliTrack.title, m_biliTrack.artist, m_biliTrack.duration);

        emit lyricsReady(onlineLyrics.isEmpty() ? placeholderLyrics() : onlineLyrics, translate);

        if (!lyricHash.isEmpty()) {
            const auto it = m_pendingLocalLyrics.find(lyricHash);
            if (it != m_pendingLocalLyrics.end()) {
                const LyricSearch request = it.value();
                m_pendingLocalLyrics.erase(it);
                if (onlineLyrics.isEmpty())
                    emit localLyricsFailed(request.filePath);
                else
                    emit localLyricsReady(request.filePath, onlineLyrics, translate);
            }
        }
    }

    // 下载任务：歌词可能是空的，但音频地址已经拿到，仍然要入队。
    if (!lyricHash.isEmpty()) {
        const auto it = m_pendingDownloads.find(lyricHash);
        if (it != m_pendingDownloads.end()) {
            QVariantMap meta = it.value();
            m_pendingDownloads.erase(it);
            meta.insert("lyrics", data.value("info"));
            meta.insert("translate", translate);
            const QString fileName = meta.value("fileName").toString();
            emit downloadRequested(meta.value("url").toString(), fileName, meta);
            emit warned(QStringLiteral("已添加下载: ") + fileName, 1);
        }
    }
}

void PlaybackCoordinator::findLocalLyrics(const QString &filePath, const QString &title,
                                          const QString &artist, int duration, int source)
{
    const QString cleanTitle = title.trimmed();
    if (filePath.trimmed().isEmpty() || cleanTitle.isEmpty()) {
        emit localLyricsFailed(filePath);
        return;
    }

    const int resolved = resolveSource(source);
    LyricSearch request;
    request.filePath = filePath;
    request.title = cleanTitle;
    request.artist = artist.trimmed();
    request.duration = duration;
    m_localLyricSearches.insert(resolved, request);

    emit searchRequested(ApiAction::SearchLyrics,
                         QVariantMap{{ApiArg::Keyword, keywordOf(request.title, request.artist)},
                                     {ApiArg::Type, 0},
                                     {ApiArg::Page, 1},
                                     {ApiArg::PageSize, 10}},
                         resolved);
}

void PlaybackCoordinator::findOnlineLyrics(const QString &title, const QString &artist,
                                           int duration)
{
    const QString cleanTitle = title.trimmed();
    if (cleanTitle.isEmpty())
        return;

    LyricSearch request;
    request.title = cleanTitle;
    request.artist = artist.trimmed();
    request.duration = duration;

    const int source = ApiSource::Kugou; // 历史回退源：酷狗
    m_onlineLyricSearches.insert(source, request);
    emit searchRequested(ApiAction::SearchLyrics,
                         QVariantMap{{ApiArg::Keyword, keywordOf(request.title, request.artist)},
                                     {ApiArg::Type, 0},
                                     {ApiArg::Page, 1},
                                     {ApiArg::PageSize, 10}},
                         source);
}

void PlaybackCoordinator::handleLyricSearchResult(const QVariant &info, int source, bool failed)
{
    const auto localIt = m_localLyricSearches.find(source);
    const auto onlineIt = m_onlineLyricSearches.find(source);
    const bool localLookup = localIt != m_localLyricSearches.end();
    if (!localLookup && onlineIt == m_onlineLyricSearches.end())
        return;

    const LyricSearch request = localLookup ? localIt.value() : onlineIt.value();
    if (localLookup)
        m_localLyricSearches.erase(localIt);
    else
        m_onlineLyricSearches.erase(onlineIt);

    if (failed) {
        if (!request.filePath.isEmpty())
            emit localLyricsFailed(request.filePath);
        return;
    }

    const QVariantList songs = ApiCommon::normalizeList(info);
    QVariantMap best;
    int bestScore = -1;
    for (const QVariant &value : songs) {
        const QVariantMap candidate = value.toMap();
        const QString title = candidate.value(ApiCommon::kTitle).toString();
        const QString artist = candidate.value(ApiCommon::kArtist).toString();
        int score = 0;
        if (title.compare(request.title, Qt::CaseInsensitive) == 0) {
            score += 4;
        } else if (title.contains(request.title, Qt::CaseInsensitive)
                   || request.title.contains(title, Qt::CaseInsensitive)) {
            score += 2;
        }
        if (!request.artist.isEmpty()
            && (artist.contains(request.artist, Qt::CaseInsensitive)
                || request.artist.contains(artist, Qt::CaseInsensitive))) {
            score += 2;
        }
        if (!candidate.value(ApiCommon::kHash).toString().isEmpty())
            score += 1;
        if (score > bestScore) {
            bestScore = score;
            best = candidate;
        }
    }

    const QString hash = best.value(ApiCommon::kHash).toString();
    if (hash.isEmpty()) {
        if (!request.filePath.isEmpty())
            emit localLyricsFailed(request.filePath);
        return;
    }
    if (!request.filePath.isEmpty()) {
        if (m_pendingLocalLyrics.size() > 32)
            m_pendingLocalLyrics.clear();
        m_pendingLocalLyrics.insert(hash, request);
    }
    emit lyricRequested(hash, request.duration, source);
}

void PlaybackCoordinator::readLocalLyricsAsync(const QString &filePath, const QString &title,
                                               const QString &artist, int duration,
                                               bool allowOnlineSearch)
{
    if (filePath.trimmed().isEmpty()) {
        emit localLyricsFailed(filePath);
        return;
    }

    const int generation = ++m_localLyricsGeneration;

    auto *watcher = new QFutureWatcher<QVariantMap>(this);
    connect(watcher, &QFutureWatcher<QVariantMap>::finished, this,
            [this, watcher, generation, filePath, title, artist, duration, allowOnlineSearch]() {
                const QVariantMap result = watcher->result();
                watcher->deleteLater();

                if (generation != m_localLyricsGeneration)
                    return;

                const QVariantList lyrics = result.value("lyrics").toList();
                if (result.value("found").toBool() && !lyrics.isEmpty()) {
                    emit localLyricsReady(filePath, lyrics,
                                          result.value("translate").toList());
                    if (result.value("source").toString()
                        == QLatin1String("embedded-sylt")) {
                        findLocalLyrics(filePath, title, artist, duration);
                    }
                } else if (allowOnlineSearch) {
                    findLocalLyrics(filePath, title, artist, duration);
                }
            });
    watcher->setFuture(QtConcurrent::run(&LocalLyricsReader::read, filePath));
}
