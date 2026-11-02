// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#include "Playback.h"

#include "MusicApiService.h"
#include "webdav/WebDavCache.h"
#include "webdav/WebDavStore.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QQmlEngine>
#include <QSettings>

namespace {

QString localFile(const QString &path)
{
    if (path.startsWith(QLatin1String("file://"), Qt::CaseInsensitive)) {
        const QUrl url(path);
        if (url.isLocalFile())
            return url.toLocalFile();
    }
    return path;
}

}

Playback::Playback(QObject *parent)
    : QObject(parent)
{
    loadSettings();
    m_cover = new CoverHelper(this);
    QQmlEngine::setObjectOwnership(m_cover, QQmlEngine::CppOwnership);
    m_history.setLimit(m_historyLimit);
    m_history.load();
    m_cursor.setTracks(m_queue.tracks());
    m_cursor.setCycle(m_cycle);
    m_cursor.setAvoid(m_shuffleAvoid);
    m_engine.setVolume(outVolume());

    {
        QSettings settings;
        const QString cacheDir = settings.value(QStringLiteral("Options/cacheUrl")).toString();
        if (!cacheDir.isEmpty())
            m_cover->setCacheDir(cacheDir);
    }
    connect(m_cover, &CoverHelper::localCoverReady, this, &Playback::onLocalCoverReady);

    connect(&m_queue, &QueueModel::countChanged, this, &Playback::countChanged);
    connect(&m_queue, &QueueModel::contentsChanged, this, &Playback::onQueueChanged);
    connect(&m_engine, &AudioEngine::mediaStatusChanged, this, &Playback::onEngineMediaStatusChanged);
    connect(&m_engine, &AudioEngine::positionChanged, this, &Playback::onEnginePositionChanged);
    connect(&m_engine, &AudioEngine::errorOccurred, this, &Playback::onEngineError);
    connect(&m_engine, &AudioEngine::sourceChanged, this, &Playback::clearAb);
    connect(&m_engine, &AudioEngine::metaDataChanged, this, &Playback::onEngineMetaDataChanged);

    m_engine.setTrackAdvancedCallback([this] { onTrackAdvanced(); });
    QTimer::singleShot(0, this, &Playback::loadQueue);

    m_sleepTimer.setInterval(1000);
    connect(&m_sleepTimer, &QTimer::timeout, this, [this] {
        if (m_sleepMode != 1 || m_sleepRemain <= 0)
            return;
        if (--m_sleepRemain <= 0)
            sleepEnd();
        else
            emit sleepChanged();
    });
    m_pauseTimer.setSingleShot(true);
    connect(&m_pauseTimer, &QTimer::timeout, this, [this] {
        if (!m_inplay)
            m_engine.pause();
    });
    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(2000);
    connect(&m_saveTimer, &QTimer::timeout, &m_history, &PlaybackHistory::save);

    bindApi();
}

Playback::~Playback()
{
    if (s_instance == this)
        s_instance = nullptr;
    saveQueue();
    m_engine.stop();
    m_engine.setTrackAdvancedCallback({});
    m_history.save();
}

Playback *Playback::create(QQmlEngine *, QJSEngine *)
{
    if (!s_instance) {
        s_instance = new Playback(QCoreApplication::instance());
        QQmlEngine::setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
    }
    return s_instance;
}

void Playback::notifyApiReady(MusicApiService *api)
{
    Q_UNUSED(api);
    if (s_instance)
        s_instance->bindApi();
}

void Playback::bindWebDav()
{
    auto *cache = WebDavCache::instance();
    if (!cache)
        cache = WebDavCache::create(nullptr, nullptr);
    if (cache && !m_webDavBound) {
        connect(cache, &WebDavCache::cached, this, &Playback::onWebDavCached);
        m_webDavBound = true;
    }
    if (!WebDavStore::instance())
        WebDavStore::create(nullptr, nullptr);
}

void Playback::bindApi()
{
    auto *api = MusicApiService::instance();
    if (api == m_api)
        return;
    if (m_api)
        disconnect(m_api, nullptr, this, nullptr);
    m_api = api;
    if (!api)
        return;
    connect(api, &MusicApiService::urlplay, this, &Playback::onUrlplay);
    connect(api, &MusicApiService::playUrlReady, this, &Playback::onPlayUrlReady);
    connect(api, &MusicApiService::playFailed, this, &Playback::onPlayFailed);
    connect(api, &MusicApiService::localMetadataReady, this, &Playback::onLocalMetadataReady);
    connect(api, &MusicApiService::localLyricsReady, this, &Playback::onLocalLyricsReady);
    connect(api, &MusicApiService::localLyricsFailed, this, &Playback::onLocalLyricsFailed);
    connect(api, &MusicApiService::warned, this, &Playback::warned);
}

void Playback::loadSettings()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("Options"));
    m_volume = qBound(0.0, settings.value(QStringLiteral("musicVolume"), 0.6).toDouble(), 1.0);
    m_muted = settings.value(QStringLiteral("muted"), false).toBool();
    m_cycle = qBound(0, settings.value(QStringLiteral("cycleIndex"), 0).toInt(), 3);
    m_volumeStepPercent = qBound(1, settings.value(QStringLiteral("volumeStep"), 5).toInt(), 50);
    m_fadeEnabled = settings.value(QStringLiteral("fadeEnabled"), true).toBool();
    m_fadeMs = qBound(0, settings.value(QStringLiteral("fadeMs"), 200).toInt(), 2000);
    m_shuffleAvoid = qMax(0, settings.value(QStringLiteral("shuffleAvoid"), 2).toInt());
    m_historyLimit = qMax(20, settings.value(QStringLiteral("historyLimit"), 200).toInt());
    settings.endGroup();
}

void Playback::refreshSettings()
{
    refreshRuntimeSettings();
}

void Playback::refreshRuntimeSettings()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("Options"));
    const int volumeStep = qBound(1, settings.value(QStringLiteral("volumeStep"), m_volumeStepPercent).toInt(), 50);
    m_fadeEnabled = settings.value(QStringLiteral("fadeEnabled"), m_fadeEnabled).toBool();
    m_fadeMs = qBound(0, settings.value(QStringLiteral("fadeMs"), m_fadeMs).toInt(), 2000);
    m_shuffleAvoid = qMax(0, settings.value(QStringLiteral("shuffleAvoid"), m_shuffleAvoid).toInt());
    const int historyLimit = qMax(20, settings.value(QStringLiteral("historyLimit"), m_historyLimit).toInt());
    settings.endGroup();

    if (volumeStep != m_volumeStepPercent) {
        m_volumeStepPercent = volumeStep;
        emit volumeStepChanged();
    }
    m_cursor.setAvoid(m_shuffleAvoid);
    if (historyLimit != m_historyLimit) {
        m_historyLimit = historyLimit;
        m_history.setLimit(historyLimit);
    }
}

void Playback::setVolume(qreal value)
{
    const qreal clamped = qBound(0.0, value, 1.0);
    if (qFuzzyCompare(m_volume, clamped))
        return;
    m_volume = clamped;
    updateVolume();
    QSettings settings;
    settings.setValue(QStringLiteral("Options/musicVolume"), m_volume);
    emit volumeChanged();
}

void Playback::updateVolume()
{
    m_engine.setVolume(outVolume());
}

void Playback::stepVolume(qreal delta)
{
    setVolume(m_volume + delta);
}

void Playback::setMuted(bool muted)
{
    if (m_muted == muted)
        return;
    m_muted = muted;
    updateVolume();
    QSettings settings;
    settings.setValue(QStringLiteral("Options/muted"), m_muted);
    emit mutedChanged();
    emit volumeChanged();
}

void Playback::toggleMute()
{
    setMuted(!m_muted);
}

void Playback::playIndex(int index)
{
    if (index < 0 || index >= m_queue.count())
        return;
    const QueueTrack track = m_queue.trackAt(index);
    if (track.path.isEmpty())
        return;
    m_pauseTimer.stop();
    setInplay(true);
    m_current = track;
    m_queue.setPlayListIndex(index);
    m_cursor.setIndex(index);
    m_cursor.note(index);
    m_audioIndex.store(index, std::memory_order_release);
    setMusicTitle(track.name);
    setMusicArtist(track.songer);
    setMusicHash(track.path);
    setMusicSource(track.source);
    setLocalLyricsRequestPath(track.source < 0 ? localFile(track.path) : QString());
    m_engine.setNoTitle(track.name);
    m_jsonCover.clear();
    m_next = {};

    if (track.source < 0) {
        openLocal(track, track.path);
    } else if (track.source == kSourceWebDav()) {
        openWebDav(track);
    } else {
        openOnline(track);
    }
}

void Playback::openLocal(const QueueTrack &track, const QString &localPath)
{
    if (localPath.isEmpty())
        return;
    const QString path = localFile(localPath);
    if (m_api) {
        m_api->invalidatePlayRequest();
        m_api->setLyricsData({});
        m_api->setLyricsTranslate({});
    }
    m_onlinePending = false;
    setLocalLyricsRequestPath(path);
    m_engine.playSource(QUrl::fromLocalFile(path), {}, m_fadeEnabled ? m_fadeMs : 0);
    if (m_api)
        m_api->readLocalMetadataAsync(path);
    recordHistory();
    prepareNext();
}

void Playback::openOnline(const QueueTrack &track)
{
    if (track.path.isEmpty())
        return;
    if (!m_api) {
        setInplay(false);
        emit warned(QStringLiteral("在线音源尚未就绪"), 2);
        return;
    }
    m_onlinePending = true;
    m_engine.clearNextSource();
    m_engine.setSourceHeaders({});
    m_engine.beginLoading();
    m_api->setLyricsData({});
    m_api->setLyricsTranslate({});
    m_engine.fadeOut(m_fadeEnabled ? m_fadeMs : 0);
    m_api->getMusicInfo(track.path, 0, track.source);
}

void Playback::openWebDav(const QueueTrack &track)
{
    if (track.path.isEmpty())
        return;
    bindWebDav();
    if (m_api) {
        m_api->invalidatePlayRequest();
        m_api->setLyricsData({});
        m_api->setLyricsTranslate({});
    }
    m_onlinePending = false;
    const QString cached = WebDavCache::cachedAudioFor(track.path);
    if (!cached.isEmpty()) {
        setLocalLyricsRequestPath(cached);
        m_engine.setUrlStr(QStringLiteral("qrc:/QueMusic/resources/app/musicpic.png"));
        m_engine.playSource(QUrl::fromLocalFile(cached), {}, m_fadeEnabled ? m_fadeMs : 0);
        if (m_api)
            m_api->readLocalMetadataAsync(cached);
        recordHistory();
        prepareNext();
        return;
    }

    auto *store = WebDavStore::instance();
    auto *cache = WebDavCache::instance();
    const QString auth = store ? store->authHeaderOfUrl(track.path) : QString();
    QString lyricsUrl;
    QString coverUrl;
    if (store) {
        const QVariantMap sidecars = store->sidecarsOf(track.path);
        lyricsUrl = sidecars.value(QStringLiteral("lyricsUrl")).toString();
        coverUrl = sidecars.value(QStringLiteral("coverUrl")).toString();
    }
    if (cache)
        cache->cache(track.path, auth, lyricsUrl, coverUrl);

    setLocalLyricsRequestPath(QString());
    m_engine.setUrlStr(coverUrl.isEmpty()
                           ? QStringLiteral("qrc:/QueMusic/resources/app/musicpic.png")
                           : coverUrl);
    const QVariantMap headers = auth.isEmpty()
        ? QVariantMap()
        : QVariantMap{{QStringLiteral("Authorization"), auth}};
    m_engine.playSource(QUrl(track.path), headers, m_fadeEnabled ? m_fadeMs : 0);
    recordHistory();
    prepareNext();
}

void Playback::prepareNext()
{
    m_next = {};
    if (!m_inplay) {
        m_engine.clearNextSource();
        return;
    }
    if (m_engine.source().isEmpty()) {
        m_engine.clearNextSource();
        return;
    }
    const int index = m_cursor.peekAuto();
    if (index < 0) {
        m_engine.clearNextSource();
        return;
    }
    const QueueTrack track = m_cursor.track(index);
    if (track.path.isEmpty()) {
        m_engine.clearNextSource();
        return;
    }

    QUrl url;
    if (track.source < 0) {
        url = QUrl::fromLocalFile(localFile(track.path));
    } else if (track.source == kSourceWebDav()) {
        const QString cached = WebDavCache::cachedAudioFor(track.path);
        if (!cached.isEmpty())
            url = QUrl::fromLocalFile(cached);
    }

    if (!url.isEmpty()) {
        m_next.index = index;
        m_next.track = track;
        m_next.url = url;
        m_engine.prepareNextSource(url);
        return;
    }

    m_engine.clearNextSource();
    if (track.source > 0 && track.source != kSourceWebDav()) {
        m_next.index = index;
        m_next.track = track;
        bindApi();
        if (m_api)
            m_api->prefetchMusicInfo(track.path, track.source);
    }
}

void Playback::onTrackAdvanced()
{
    const int index = m_cursor.takeAuto();
    if (index < 0)
        return;
    m_audioIndex.store(index, std::memory_order_release);
    const QueueTrack track = m_cursor.track(index);
    const int following = m_cursor.peekAuto();
    QUrl nextUrl;
    if (following >= 0) {
        const QueueTrack next = m_cursor.track(following);
        if (next.source < 0)
            nextUrl = QUrl::fromLocalFile(localFile(next.path));
    }
    if (nextUrl.isEmpty())
        m_engine.clearNextSource();
    else
        m_engine.prepareNextSource(nextUrl);

    QMetaObject::invokeMethod(this, [this, index, path = track.path] {
        adoptAuto(index, path);
    }, Qt::QueuedConnection);
}

void Playback::adoptAuto(int index, const QString &path)
{
    if (m_audioIndex.load(std::memory_order_acquire) != index)
        return;
    const int row = rowFor(index, path);
    if (row < 0)
        return;
    const QueueTrack track = m_queue.trackAt(row);
    const bool ready = m_next.index == index && m_next.track.path == track.path;
    const QString title = ready && !m_next.title.isEmpty() ? m_next.title : track.name;
    const QString artist = ready && !m_next.artist.isEmpty() ? m_next.artist : track.songer;
    const QString cover = ready ? m_next.cover : QString();

    m_current = track;
    m_cursor.setIndex(row);
    m_audioIndex.store(row, std::memory_order_release);
    m_queue.setPlayListIndex(row);
    setMusicTitle(title);
    setMusicArtist(artist);
    setMusicHash(track.path);
    setMusicSource(track.source);
    m_engine.setNoTitle(title);
    m_engine.setUrlStr(cover.isEmpty()
                           ? QStringLiteral("qrc:/QueMusic/resources/app/musicpic.png")
                           : cover);
    m_engine.setSourceHeaders({});

    if (ready && !m_next.url.isEmpty()) {
        m_engine.adoptSource(m_next.url);
    } else if (track.source < 0) {
        m_engine.adoptSource(QUrl::fromLocalFile(localFile(track.path)));
    }

    if (m_api) {
        m_api->setLyricsData({});
        m_api->setLyricsTranslate({});
    }
    m_jsonCover.clear();
    if (track.source < 0) {
        const QString localPath = localFile(track.path);
        setLocalLyricsRequestPath(localPath);
        if (m_api)
            m_api->readLocalMetadataAsync(localPath);
    } else if (track.source == kSourceWebDav()) {
        const QString cached = WebDavCache::cachedAudioFor(track.path);
        if (!cached.isEmpty()) {
            setLocalLyricsRequestPath(cached);
            if (m_api)
                m_api->readLocalMetadataAsync(cached);
        }
    } else {
        bindApi();
        if (m_api)
            m_api->getLyricInfo(track.path, ready ? m_next.duration : 0, track.source);
    }

    m_next = {};
    recordHistory();
    prepareNext();
}

void Playback::recordHistory()
{
    if (m_current.path.isEmpty() || m_title.isEmpty())
        return;
    HistoryEntry entry;
    entry.title = m_title;
    entry.artist = m_artist;
    entry.path = m_current.path;
    entry.source = m_current.source;
    entry.cover = m_engine.urlStr();
    entry.duration = int(m_engine.duration() / 1000);
    entry.time = QDateTime::currentMSecsSinceEpoch();
    m_history.add(entry);
    m_saveTimer.start();
}

void Playback::next(bool random)
{
    const int index = m_cursor.takeManual(random);
    if (index >= 0)
        playIndex(index);
}

void Playback::previous()
{
    const int index = m_cursor.takePrevious();
    if (index >= 0)
        playIndex(index);
}

bool Playback::enqueue(const QString &name, const QString &path,
                       const QString &songer, int source)
{
    const QString itemPath = source < 0 ? localFile(path) : path;
    if (itemPath.isEmpty() || m_queue.indexOfPath(itemPath) >= 0)
        return false;
    QueueTrack track;
    track.name = name;
    track.path = itemPath;
    track.songer = songer;
    track.source = source;
    m_queue.appendTrack(track);
    return true;
}

void Playback::playNext(const QString &name, const QString &path,
                        const QString &songer, int source)
{
    const QString itemPath = source < 0 ? localFile(path) : path;
    if (itemPath.isEmpty())
        return;
    const int existing = m_queue.indexOfPath(itemPath);
    if (existing >= 0) {
        m_queue.moveTo(existing, m_queue.playListIndex() + 1);
        return;
    }
    QueueTrack track;
    track.name = name;
    track.path = itemPath;
    track.songer = songer;
    track.source = source;
    const int at = m_queue.playListIndex() + 1;
    m_queue.insertTrack(at, track);
}

void Playback::playItem(const QString &name, const QString &path,
                        const QString &songer, int source)
{
    const QString itemPath = source < 0 ? localFile(path) : path;
    if (itemPath.isEmpty())
        return;
    int index = m_queue.indexOfPath(itemPath);
    if (index < 0) {
        QueueTrack track;
        track.name = name;
        track.path = itemPath;
        track.songer = songer;
        track.source = source;
        m_queue.appendTrack(track);
        index = m_queue.count() - 1;
    }
    playIndex(index);
}

void Playback::playLocalSong(const QString &path, const QString &name)
{
    if (path.isEmpty())
        return;
    const QString localPath = localFile(path);
    if (localPath.isEmpty())
        return;
    m_pauseTimer.stop();
    setInplay(true);
    const int index = m_queue.indexOfPath(localPath);
    if (index >= 0) {
        playIndex(index);
        return;
    }
    const QString title = name.isEmpty() ? QFileInfo(localPath).completeBaseName() : name;
    QueueTrack track;
    track.name = title;
    track.path = localPath;
    track.source = -1;
    m_current = track;
    m_queue.setPlayListIndex(-1);
    m_cursor.setIndex(-1);
    m_audioIndex.store(-1, std::memory_order_release);
    setMusicTitle(title);
    setMusicArtist(QString());
    setMusicHash(localPath);
    setMusicSource(-1);
    setLocalLyricsRequestPath(localPath);
    m_engine.setNoTitle(title);
    m_jsonCover.clear();
    m_next = {};
    openLocal(track, localPath);
}

void Playback::togglePlay()
{
    if (m_inplay) {
        setInplay(false);
        m_pauseTimer.stop();
        m_engine.clearNextSource();
        m_next = {};
        if (m_engine.source().isEmpty())
            return;
        if (m_fadeEnabled && m_fadeMs > 0) {
            m_engine.fadeOut(m_fadeMs);
            m_pauseTimer.start(m_fadeMs);
        } else {
            m_engine.pause();
        }
        return;
    }

    m_pauseTimer.stop();
    if (m_engine.source().isEmpty()) {
        if (m_queue.count() == 0)
            return;
        int index = m_queue.playListIndex();
        if (index < 0 || index >= m_queue.count())
            index = 0;
        playIndex(index);
        return;
    }

    const bool wasPlaying = m_engine.playing();
    setInplay(true);
    if (!wasPlaying)
        m_engine.play();
    if (m_fadeEnabled && m_fadeMs > 0) {
        if (wasPlaying)
            m_engine.fadeIn(m_fadeMs);
        else
            m_engine.fadeInOnNextAudio(m_fadeMs);
    }
    prepareNext();
}

void Playback::seekBy(qint64 ms)
{
    const qint64 target = qBound<qint64>(0, m_engine.position() + ms, m_engine.duration());
    m_engine.setPosition(target);
}

void Playback::seekBack()
{
    QSettings settings;
    seekBy(-qint64(qMax(1, settings.value(QStringLiteral("Options/seekStep"), 5).toInt())) * 1000);
}

void Playback::seekForward()
{
    QSettings settings;
    seekBy(qint64(qMax(1, settings.value(QStringLiteral("Options/seekStep"), 5).toInt())) * 1000);
}

void Playback::setAbPoint(int which)
{
    const int position = int(m_engine.position());
    if (which == 0) {
        m_abA = position;
        if (m_abB >= 0 && m_abB <= m_abA)
            m_abB = -1;
    } else {
        m_abB = position > m_abA ? position : -1;
    }
    emit abChanged();
}

void Playback::clearAb()
{
    if (m_abA < 0 && m_abB < 0)
        return;
    m_abA = -1;
    m_abB = -1;
    emit abChanged();
}

void Playback::armSleep(int mode, int minutes)
{
    m_sleepMode = qBound(0, mode, 2);
    m_sleepRemain = mode == 1 ? qMax(1, minutes) * 60 : 0;
    if (m_sleepMode == 1 && m_sleepRemain > 0)
        m_sleepTimer.start();
    else
        m_sleepTimer.stop();
    emit sleepChanged();
}

void Playback::stopSleep()
{
    m_sleepMode = 0;
    m_sleepRemain = 0;
    m_sleepTimer.stop();
    emit sleepChanged();
}

QString Playback::sleepLabel() const
{
    if (m_sleepMode == 0)
        return QStringLiteral("关闭");
    if (m_sleepMode == 2)
        return QStringLiteral("本首结束");
    return fmt(qint64(m_sleepRemain) * 1000);
}

void Playback::sleepEnd()
{
    refreshRuntimeSettings();
    stopSleep();
    setInplay(false);
    m_engine.clearNextSource();
    m_next = {};
    if (m_fadeEnabled && m_fadeMs > 0) {
        m_engine.fadeOut(m_fadeMs);
        m_pauseTimer.start(m_fadeMs);
    } else {
        m_engine.pause();
    }
    emit warned(QStringLiteral("睡眠定时：已停止播放"), 1);
}

void Playback::onQueueChanged()
{
    if (m_appending)
        return;
    m_cursor.setTracks(m_queue.tracks());
    const int row = m_current.path.isEmpty() ? -1 : m_queue.indexOfPath(m_current.path);
    if (row >= 0) {
        m_audioIndex.store(row, std::memory_order_release);
        m_cursor.setIndex(row);
        if (m_queue.playListIndex() != row)
            m_queue.setPlayListIndex(row);
    } else {
        m_audioIndex.store(-1, std::memory_order_release);
        m_cursor.setIndex(-1);
        if (m_queue.playListIndex() != -1)
            m_queue.setPlayListIndex(-1);
    }
    prepareNext();
}

void Playback::onEngineMediaStatusChanged()
{
    switch (m_engine.mediaStatus()) {
    case AudioEngine::LoadedMedia:
        m_autoSkipCount = 0;
        return;
    case AudioEngine::EndOfMedia:
        if (!m_inplay)
            return;
        break;
    default:
        return;
    }

    if (m_sleepMode == 2) {
        sleepEnd();
        return;
    }
    if (m_cycle == 3) {
        setInplay(false);
        m_engine.stop();
        return;
    }
    const int index = m_cursor.takeAuto(false);
    if (index >= 0) {
        playIndex(index);
    } else {
        setInplay(false);
        m_engine.stop();
    }
}

void Playback::onEnginePositionChanged()
{
    if (abArmed() && m_engine.position() >= m_abB)
        m_engine.setPosition(m_abA);
}

void Playback::onEngineError(int code, const QString &message)
{
    Q_UNUSED(code);
    if (!m_inplay) {
        m_engine.stop();
        return;
    }
    if (m_queue.count() < 2) {
        setInplay(false);
        m_engine.stop();
        return;
    }
    if (m_autoSkipCount >= 2) {
        m_autoSkipCount = 0;
        emit warned(QStringLiteral("连续多首无法播放，已停止自动跳过"), 0);
        setInplay(false);
        m_engine.stop();
        return;
    }
    ++m_autoSkipCount;
    emit warned(QStringLiteral("当前歌曲无法播放，已自动跳过：") + message, 0);
    const int index = m_cursor.takeManual(m_cycle == 2);
    if (index >= 0)
        playIndex(index);
}

void Playback::onEngineMetaDataChanged()
{
    if (m_engine.source().toString().startsWith(QLatin1String("http")))
        return;
    if (!m_engine.tagTitle().isEmpty())
        setMusicTitle(m_engine.tagTitle());
    if (!m_engine.artist().isEmpty())
        setMusicArtist(m_engine.artist());
}

void Playback::onLocalMetadataReady(const QString &filePath, const QVariantMap &meta)
{
    if (!m_api || filePath != m_localLyricsRequestPath)
        return;
    const QString title = meta.value(QStringLiteral("title")).toString();
    const QString artist = meta.value(QStringLiteral("artist")).toString();
    const QString album = meta.value(QStringLiteral("album")).toString();
    const QVariantList lyrics = meta.value(QStringLiteral("lyrics")).toList();
    const QVariantList translate = meta.value(QStringLiteral("translate")).toList();
    const int duration = meta.value(QStringLiteral("duration")).toInt();

    if (!title.isEmpty())
        setMusicTitle(title);
    if (!artist.isEmpty())
        setMusicArtist(artist);
    m_engine.setAlbum(album);

    m_api->setLyricsData(lyrics);
    m_api->setLyricsTranslate(translate);
    if (lyrics.isEmpty())
        m_api->setLocalLyrics();
    m_api->readLocalLyricsAsync(filePath, title.isEmpty() ? m_engine.noTitle() : title,
                                artist, duration, lyrics.isEmpty());

    m_jsonCover = meta.value(QStringLiteral("cover")).toString();
    if (!m_jsonCover.isEmpty())
        m_engine.setUrlStr(m_jsonCover);
    m_cover->findEmbeddedCoverAsync(filePath);
}

void Playback::onLocalLyricsReady(const QString &filePath, const QVariantList &lyrics,
                                  const QVariantList &translate)
{
    if (!m_api || filePath != m_localLyricsRequestPath)
        return;
    m_api->setLyricsData(lyrics);
    m_api->setLyricsTranslate(translate);
}

void Playback::onLocalLyricsFailed(const QString &filePath)
{
    if (!m_api || filePath != m_localLyricsRequestPath)
        return;
    if (m_api->lyricsData().size() > 1)
        return;
    m_api->setLocalLyrics();
}

void Playback::onLocalCoverReady(const QString &filePath, const QString &coverUrl)
{
    if (filePath != m_localLyricsRequestPath)
        return;
    const QString cover = coverUrl.isEmpty() ? m_cover->findLocalCover(filePath) : coverUrl;
    if (!cover.isEmpty())
        m_engine.setUrlStr(cover);
    else if (m_jsonCover.isEmpty())
        m_engine.setUrlStr(QStringLiteral("qrc:/QueMusic/resources/app/musicpic.png"));
}

void Playback::onWebDavCached(const QString &url, const QString &localPath)
{
    if (m_engine.source().toString() != url)
        return;
    setLocalLyricsRequestPath(localPath);
    if (m_api)
        m_api->readLocalMetadataAsync(localPath);
}

void Playback::onUrlplay(const QString &playurl, const QString &title, const QString &artist,
                         const QString &cover, const QString &solve, const QString &hash, int source)
{
    Q_UNUSED(solve);
    if (playurl.isEmpty() || hash.isEmpty())
        return;
    const bool pending = m_onlinePending && hash == m_current.path;
    m_onlinePending = false;
    int index = m_queue.indexOfPath(hash);
    if (index < 0) {
        QueueTrack track;
        track.name = title;
        track.path = hash;
        track.songer = artist;
        track.source = source;
        m_current = track;
        m_appending = true;
        m_queue.appendTrack(track);
        m_appending = false;
        m_cursor.setTracks(m_queue.tracks());
        index = m_queue.count() - 1;
    }

    const QueueTrack track = m_queue.trackAt(index);
    m_current = track;
    m_queue.setPlayListIndex(index);
    m_cursor.setIndex(index);
    m_cursor.note(index);
    m_audioIndex.store(index, std::memory_order_release);
    setMusicTitle(title);
    setMusicArtist(artist);
    setMusicHash(hash);
    setMusicSource(source);
    setLocalLyricsRequestPath(QString());
    m_engine.setNoTitle(title);
    m_jsonCover.clear();
    m_engine.setUrlStr(cover.isEmpty()
                          ? QStringLiteral("qrc:/QueMusic/resources/app/musicpic.png")
                          : cover);
    m_engine.setSourceHeaders({});
    if (m_api) {
        m_api->setLyricsData({});
        m_api->setLyricsTranslate({});
    }
    if (!pending || m_inplay) {
        m_pauseTimer.stop();
        setInplay(true);
        m_engine.playSource(QUrl(playurl), {}, m_fadeEnabled ? m_fadeMs : 0);
    } else {
        m_engine.setSource(QUrl(playurl));
        if (!m_pauseTimer.isActive())
            m_engine.pause();
    }
    m_next = {};
    recordHistory();
    prepareNext();
}

void Playback::onPlayUrlReady(const QString &hash, const QString &url, const QString &title,
                              const QString &artist, const QString &cover, int duration, int source)
{
    if (url.isEmpty() || m_next.index < 0 || m_next.track.path != hash
        || m_next.track.source != source) {
        return;
    }
    m_next.url = QUrl(url);
    m_next.title = title;
    m_next.artist = artist;
    m_next.cover = cover;
    m_next.duration = duration;
    m_engine.prepareNextSource(QUrl(url));
}

void Playback::onPlayFailed(const QString &hash, int source)
{
    if (hash != m_hash || source != m_source)
        return;
    m_onlinePending = false;
    m_engine.setSource(QUrl());
    m_next = {};
    if (!m_inplay)
        return;
    setInplay(false);
    if (m_queue.count() < 2 || m_autoSkipCount >= 5)
        return;
    ++m_autoSkipCount;
    const int index = m_cursor.takeManual(m_cycle == 2);
    if (index >= 0)
        playIndex(index);
}

int Playback::rowFor(int index, const QString &path) const
{
    if (index >= 0 && index < m_queue.count() && m_queue.trackAt(index).path == path)
        return index;
    return m_queue.indexOfPath(path);
}

void Playback::setInplay(bool inplay)
{
    if (m_inplay == inplay)
        return;
    m_inplay = inplay;
    emit inplayChanged();
}

void Playback::setMusicTitle(const QString &title)
{
    if (m_title == title)
        return;
    m_title = title;
    emit musicTitleChanged();
}

void Playback::setMusicArtist(const QString &artist)
{
    if (m_artist == artist)
        return;
    m_artist = artist;
    emit musicArtistChanged();
}

void Playback::setMusicHash(const QString &hash)
{
    if (m_hash == hash)
        return;
    m_hash = hash;
    emit musicHashChanged();
}

void Playback::setMusicSource(int source)
{
    if (m_source == source)
        return;
    m_source = source;
    emit musicSourceChanged();
}

void Playback::setLocalLyricsRequestPath(const QString &path)
{
    if (m_localLyricsRequestPath == path)
        return;
    m_localLyricsRequestPath = path;
    emit localLyricsRequestPathChanged();
}

void Playback::setCycleIndex(int index)
{
    const int clamped = qBound(0, index, 3);
    if (m_cycle == clamped)
        return;
    m_cycle = clamped;
    m_cursor.setCycle(clamped);
    QSettings settings;
    settings.setValue(QStringLiteral("Options/cycleIndex"), clamped);
    emit cycleIndexChanged();
    prepareNext();
}

QString Playback::cycleIcon() const
{
    static const QString icons[] = {
        QStringLiteral("\uf118"), QStringLiteral("\uf115"),
        QStringLiteral("\uf0e2"), QStringLiteral("\uf03b")
    };
    return icons[qBound(0, m_cycle, 3)];
}

QString Playback::cycleTip() const
{
    static const QString tips[] = {
        QStringLiteral("列表循环"), QStringLiteral("单曲循环"),
        QStringLiteral("随机播放"), QStringLiteral("暂停操作")
    };
    return tips[qBound(0, m_cycle, 3)];
}

QString Playback::sourceText(int source) const
{
    return source < 0 || source == kSourceWebDav() ? QStringLiteral("本地")
                                                   : QStringLiteral("在线");
}

QString Playback::fmt(qint64 ms) const
{
    if (ms < 0)
        ms = 0;
    const qint64 seconds = ms / 1000;
    return QStringLiteral("%1:%2")
        .arg(seconds / 60, 2, 10, QLatin1Char('0'))
        .arg(seconds % 60, 2, 10, QLatin1Char('0'));
}

void Playback::saveQueue()
{
    if (!m_queueLoaded)
        return;
    QJsonArray array;
    const QVector<QueueTrack> tracks = m_queue.tracks();
    for (const QueueTrack &track : tracks) {
        array.append(QJsonObject{
            {QStringLiteral("name"), track.name},
            {QStringLiteral("path"), track.path},
            {QStringLiteral("songer"), track.songer},
            {QStringLiteral("source"), track.source}
        });
    }
    QSettings settings;
    settings.setValue(QStringLiteral("Options/lastQueue"),
                      QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact)));
    settings.setValue(QStringLiteral("Options/lastQueueIndex"), m_queue.playListIndex());
    settings.sync();
}

void Playback::loadQueue()
{
    m_queueLoaded = true;
    if (m_queue.count() > 0)
        return;
    QSettings settings;
    if (!settings.value(QStringLiteral("Options/autoRestoreQueue"), true).toBool())
        return;

    const QJsonDocument document = QJsonDocument::fromJson(
        settings.value(QStringLiteral("Options/lastQueue")).toString().toUtf8());
    if (!document.isArray())
        return;

    QVector<QueueTrack> tracks;
    const QJsonArray array = document.array();
    tracks.reserve(array.size());
    for (const QJsonValue &value : array) {
        const QJsonObject object = value.toObject();
        QueueTrack track;
        track.name = object.value(QStringLiteral("name")).toString();
        track.path = object.value(QStringLiteral("path")).toString();
        track.songer = object.value(QStringLiteral("songer")).toString();
        track.source = object.value(QStringLiteral("source")).toInt(-1);
        if (track.source < 0)
            track.path = localFile(track.path);
        if (!track.path.isEmpty())
            tracks.append(track);
    }
    if (tracks.isEmpty())
        return;

    m_queue.appendTracks(tracks, false);
    const int index = settings.value(QStringLiteral("Options/lastQueueIndex"), -1).toInt();
    if (index >= 0 && index < m_queue.count()) {
        m_queue.setPlayListIndex(index);
        m_cursor.setIndex(index);
    }
}

void Playback::clearHistory()
{
    m_saveTimer.stop();
    m_history.clear();
}

void Playback::flush()
{
    if (m_saveTimer.isActive()) {
        m_saveTimer.stop();
        m_history.save();
    }
}