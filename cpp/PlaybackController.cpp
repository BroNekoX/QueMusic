// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#include "PlaybackController.h"

#include "MusicApiService.h"
#include "webdav/WebDavCache.h"
#include "webdav/WebDavStore.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QRandomGenerator>
#include <QSettings>

#include <algorithm>
#include <utility>

PlaybackHistoryModel::PlaybackHistoryModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int PlaybackHistoryModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_items.size());
}

QVariant PlaybackHistoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return {};
    const QVariantMap &item = m_items.at(index.row());
    switch (role) {
    case TitleRole:    return item.value(QStringLiteral("title"));
    case ArtistRole:   return item.value(QStringLiteral("artist"));
    case PathRole:     return item.value(QStringLiteral("path"));
    case SourceRole:   return item.value(QStringLiteral("source"));
    case CoverRole:    return item.value(QStringLiteral("cover"));
    case DurationRole: return item.value(QStringLiteral("duration"));
    case TimeRole:     return item.value(QStringLiteral("time"));
    default:           return {};
    }
}

QHash<int, QByteArray> PlaybackHistoryModel::roleNames() const
{
    return {
        {TitleRole, "title"},
        {ArtistRole, "artist"},
        {PathRole, "path"},
        {SourceRole, "source"},
        {CoverRole, "cover"},
        {DurationRole, "duration"},
        {TimeRole, "time"}
    };
}

QVariantMap PlaybackHistoryModel::get(int index) const
{
    return index >= 0 && index < m_items.size() ? m_items.at(index) : QVariantMap();
}

void PlaybackHistoryModel::append(const QVariantMap &item)
{
    const int row = int(m_items.size());
    beginInsertRows(QModelIndex(), row, row);
    m_items.append(item);
    endInsertRows();
    emit countChanged();
}

void PlaybackHistoryModel::pushFront(const QVariantMap &item)
{
    beginInsertRows(QModelIndex(), 0, 0);
    m_items.prepend(item);
    endInsertRows();
    emit countChanged();
}

void PlaybackHistoryModel::remove(int index, int count)
{
    if (index < 0 || count <= 0 || index >= m_items.size())
        return;
    count = std::min(count, int(m_items.size()) - index);
    beginRemoveRows(QModelIndex(), index, index + count - 1);
    m_items.remove(index, count);
    endRemoveRows();
    emit countChanged();
}

void PlaybackHistoryModel::clear()
{
    if (m_items.isEmpty())
        return;
    beginResetModel();
    m_items.clear();
    endResetModel();
    emit countChanged();
}

void PlaybackHistoryModel::setItems(const QVector<QVariantMap> &items)
{
    beginResetModel();
    m_items = items;
    endResetModel();
    emit countChanged();
}

PlaybackController::PlaybackController(QObject *parent)
    : QObject(parent)
{
    loadSettings();
    loadHistory();
    bindApi();
    m_engine.setUrlStr(QStringLiteral("qrc:/QueMusic/resources/app/musicpic.png"));

    connect(&m_queue, &QueueModel::countChanged, this, &PlaybackController::countChanged);
    connect(&m_queue, &QueueModel::contentsChanged, this, &PlaybackController::onQueueContentsChanged);

    connect(&m_engine, &AudioEngine::mediaStatusChanged, this,
            &PlaybackController::onEngineMediaStatusChanged);
    connect(&m_engine, &AudioEngine::playbackStateChanged, this,
            &PlaybackController::onEnginePlaybackStateChanged);
    connect(&m_engine, &AudioEngine::positionChanged, this,
            &PlaybackController::onEnginePositionChanged);
    connect(&m_engine, &AudioEngine::errorOccurred, this, &PlaybackController::onEngineError);
    connect(&m_engine, &AudioEngine::sourceChanged, this,
            &PlaybackController::onEngineSourceChanged);
    m_engine.setTrackAdvancedCallback([this] { autoAdvanceFromAudioThread(); });
    m_engine.setVolume(outVolume());

    m_sleepTimer.setInterval(1000);
    connect(&m_sleepTimer, &QTimer::timeout, this, [this] {
        if (m_sleepRemain <= 0)
            return;
        --m_sleepRemain;
        emit sleepChanged();
        if (m_sleepRemain <= 0)
            sleepEnd();
    });

    m_pauseTimer.setSingleShot(true);
    connect(&m_pauseTimer, &QTimer::timeout, this, [this] {
        if (m_engine.playing())
            m_engine.pause();
    });

    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(2000);
    connect(&m_saveTimer, &QTimer::timeout, this, &PlaybackController::writeHistory);

    syncSnapshot();
}

PlaybackController::~PlaybackController()
{
    m_shuttingDown.store(true, std::memory_order_release);
    if (s_instance == this)
        s_instance = nullptr;
    m_engine.setTrackAdvancedCallback({});
    m_sleepTimer.stop();
    m_pauseTimer.stop();
    if (m_saveTimer.isActive()) {
        m_saveTimer.stop();
        writeHistory();
    }
}

PlaybackController *PlaybackController::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
{
    Q_UNUSED(jsEngine);
    auto *controller = new PlaybackController(qmlEngine);
    s_instance = controller;
    return controller;
}

void PlaybackController::notifyApiReady(MusicApiService *api)
{
    if (!s_instance || !api)
        return;
    if (s_instance->m_api == api)
        return;
    if (s_instance->m_api)
        disconnect(s_instance->m_api, nullptr, s_instance, nullptr);
    s_instance->m_api = api;
    connect(api, &MusicApiService::urlplay, s_instance, &PlaybackController::onUrlplay);
    connect(api, &MusicApiService::warned, s_instance, &PlaybackController::onApiWarned);
    connect(api, &MusicApiService::playUrlReady, s_instance, &PlaybackController::onPlayUrlReady);
    connect(api, &MusicApiService::playFailed, s_instance, &PlaybackController::onPlayFailed);
}

void PlaybackController::refreshSettings()
{
    refreshRuntimeSettings();
}

void PlaybackController::loadSettings()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("Options"));
    m_volume = qBound(0.0, settings.value(QStringLiteral("musicVolume"), 0.6).toDouble(), 1.0);
    m_muted = settings.value(QStringLiteral("muted"), false).toBool();
    m_cycleIndex.store(qBound(0, settings.value(QStringLiteral("cycleIndex"), 0).toInt(), 3),
                       std::memory_order_relaxed);
    m_volumeStepPercent = qBound(1, settings.value(QStringLiteral("volumeStep"), 5).toInt(), 50);
    m_fadeEnabled = settings.value(QStringLiteral("fadeEnabled"), true).toBool();
    m_fadeMs = qBound(0, settings.value(QStringLiteral("fadeMs"), 200).toInt(), 2000);
    m_shuffleAvoid = qMax(0, settings.value(QStringLiteral("shuffleAvoid"), 2).toInt());
    m_historyLimit = qMax(20, settings.value(QStringLiteral("historyLimit"), 200).toInt());
    settings.endGroup();
}

void PlaybackController::refreshRuntimeSettings()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("Options"));
    const int volumeStep = qBound(1, settings.value(QStringLiteral("volumeStep"), m_volumeStepPercent).toInt(), 50);
    m_fadeEnabled = settings.value(QStringLiteral("fadeEnabled"), m_fadeEnabled).toBool();
    m_fadeMs = qBound(0, settings.value(QStringLiteral("fadeMs"), m_fadeMs).toInt(), 2000);
    m_historyLimit = qMax(20, settings.value(QStringLiteral("historyLimit"), m_historyLimit).toInt());
    const int shuffleAvoid = qMax(0, settings.value(QStringLiteral("shuffleAvoid"), m_shuffleAvoid).toInt());
    settings.endGroup();
    if (volumeStep != m_volumeStepPercent) {
        m_volumeStepPercent = volumeStep;
        emit volumeStepChanged();
    }
    if (shuffleAvoid != m_shuffleAvoid) {
        QMutexLocker lock(&m_stateMutex);
        m_shuffleAvoid = shuffleAvoid;
    }
}

void PlaybackController::bindApi()
{
    auto *api = MusicApiService::instance();
    if (api == m_api)
        return;
    if (m_api)
        disconnect(m_api, nullptr, this, nullptr);
    m_api = api;
    if (m_api) {
        connect(m_api, &MusicApiService::urlplay, this, &PlaybackController::onUrlplay);
        connect(m_api, &MusicApiService::warned, this, &PlaybackController::onApiWarned);
        connect(m_api, &MusicApiService::playUrlReady, this, &PlaybackController::onPlayUrlReady);
    connect(m_api, &MusicApiService::playFailed, this, &PlaybackController::onPlayFailed);
    }
}

void PlaybackController::clearPrefetchLocked()
{
    m_prefetchIndex = -1;
    m_prefetchTitle.clear();
    m_prefetchArtist.clear();
    m_prefetchCover.clear();
    m_prefetchDuration = 0;
}

void PlaybackController::syncSnapshot()
{
    QMutexLocker lock(&m_stateMutex);
    const bool playing = !m_engine.source().isEmpty()
                         && m_engine.playbackState() != AudioEngine::StoppedState;
    m_snapshot = m_queue.tracks();
    if (!playing)
        m_snapshotIndex = m_queue.playListIndex();
    else if (!musicHash().isEmpty())
        m_snapshotIndex = m_queue.indexOfPath(musicHash());
    else if (m_snapshotIndex >= m_snapshot.size())
        m_snapshotIndex = int(m_snapshot.size()) - 1;
    m_bag.clear();
    m_preparedNextIndex = -1;
    m_preparedNextUrl.clear();
    clearPrefetchLocked();
}

void PlaybackController::onQueueContentsChanged()
{
    syncSnapshot();
    if (!m_engine.source().isEmpty() && m_engine.playbackState() != AudioEngine::StoppedState)
        prepareNext();
    else
        m_engine.clearNextSource();
}

void PlaybackController::notePlayed(int index)
{
    if (index < 0)
        return;
    QMutexLocker lock(&m_stateMutex);
    m_recent.append(index);
    while (m_recent.size() > qMax(0, m_shuffleAvoid))
        m_recent.removeFirst();
}

int PlaybackController::randomIndexLocked(bool consume)
{
    const int size = int(m_snapshot.size());
    if (size <= 1)
        return size == 1 ? 0 : -1;

    if (m_bag.isEmpty()) {
        for (int i = 0; i < size; ++i) {
            if (i != m_snapshotIndex && !m_recent.contains(i))
                m_bag.append(i);
        }
        if (m_bag.isEmpty()) {
            for (int i = 0; i < size; ++i)
                m_bag.append(i);
        }
        for (int i = int(m_bag.size()) - 1; i > 0; --i) {
            const int j = QRandomGenerator::global()->bounded(i + 1);
            std::swap(m_bag[i], m_bag[j]);
        }
    }
    return consume ? m_bag.takeLast() : m_bag.last();
}

int PlaybackController::nextIndexForCycleLocked(bool consume)
{
    if (m_snapshot.isEmpty())
        return -1;
    switch (m_cycleIndex.load(std::memory_order_relaxed)) {
    case 1:
        return qMax(0, m_snapshotIndex);
    case 2:
        return randomIndexLocked(consume);
    case 3:
        return -1;
    default:
        return m_snapshotIndex < 0 ? 0 : (m_snapshotIndex + 1) % int(m_snapshot.size());
    }
}

int PlaybackController::nextIndexForCycle()
{
    QMutexLocker lock(&m_stateMutex);
    return nextIndexForCycleLocked(true);
}

void PlaybackController::prepareNextLocked()
{
    m_preparedNextIndex = -1;
    m_preparedNextUrl.clear();

    const int next = nextIndexForCycleLocked(false);
    if (next < 0 || next >= m_snapshot.size())
        return;
    const QueueTrack &track = m_snapshot.at(next);
    QUrl url;
    if (track.source < 0)
        url = QUrl::fromLocalFile(track.path);
    if (url.isEmpty())
        return;
    m_preparedNextIndex = next;
    m_preparedNextUrl = url;
}

void PlaybackController::prepareNext()
{
    QUrl nextUrl;
    int prefetchIndex = -1;
    int webdavIndex = -1;
    QString prefetchHash;
    QString webdavPath;
    int prefetchSource = 0;
    {
        QMutexLocker lock(&m_stateMutex);
        if (m_preparedNextIndex >= 0 && !m_preparedNextUrl.isEmpty()) {
            nextUrl = m_preparedNextUrl;
        } else {
            prepareNextLocked();
            nextUrl = m_preparedNextUrl;
            if (nextUrl.isEmpty() && !m_snapshot.isEmpty()) {
                const int next = nextIndexForCycleLocked(false);
                if (next >= 0 && next < m_snapshot.size()) {
                    const QueueTrack &track = m_snapshot.at(next);
                    if (track.source == kSourceWebDav()) {
                        webdavIndex = next;
                        webdavPath = track.path;
                    } else if (!track.path.isEmpty() && track.source >= 0
                               && m_prefetchIndex != next) {
                        clearPrefetchLocked();
                        m_prefetchIndex = next;
                        prefetchIndex = next;
                        prefetchHash = track.path;
                        prefetchSource = track.source;
                    }
                }
            }
        }
    }
    if (nextUrl.isEmpty() && webdavIndex >= 0) {
        const QString cached = WebDavCache::cachedAudioFor(webdavPath);
        if (!cached.isEmpty()) {
            QMutexLocker lock(&m_stateMutex);
            m_preparedNextIndex = webdavIndex;
            m_preparedNextUrl = QUrl::fromLocalFile(cached);
            nextUrl = m_preparedNextUrl;
        }
    }
    if (!nextUrl.isEmpty()) {
        m_engine.prepareNextSource(nextUrl);
        return;
    }
    m_engine.clearNextSource();
    if (prefetchIndex >= 0) {
        bindApi();
        if (m_api)
            m_api->prefetchMusicInfo(prefetchHash, prefetchSource);
    }
}

void PlaybackController::autoAdvanceFromAudioThread()
{
    if (m_shuttingDown.load(std::memory_order_acquire))
        return;
    int index = -1;
    QUrl url;
    QUrl following;
    QString title;
    QString artist;
    QString cover;
    QString trackPath;
    int duration = 0;
    {
        QMutexLocker lock(&m_stateMutex);
        index = m_preparedNextIndex;
        url = m_preparedNextUrl;
        if (index < 0 || index >= m_snapshot.size() || url.isEmpty())
            return;
        trackPath = m_snapshot.at(index).path;

        if (m_cycleIndex.load(std::memory_order_relaxed) == 2)
            m_bag.removeOne(index);
        if (m_prefetchIndex == index) {
            title = m_prefetchTitle;
            artist = m_prefetchArtist;
            cover = m_prefetchCover;
            duration = m_prefetchDuration;
        }

        m_snapshotIndex = index;
        m_recent.append(index);
        while (m_recent.size() > qMax(0, m_shuffleAvoid))
            m_recent.removeFirst();
        clearPrefetchLocked();
        prepareNextLocked();
        following = m_preparedNextUrl;
    }

    if (!following.isEmpty())
        m_engine.prepareNextSource(following);

    QMetaObject::invokeMethod(this, [this, index, url, title, artist, cover, duration, trackPath] {
        applyAutoAdvance(index, url, title, artist, cover, duration, trackPath);
    }, Qt::QueuedConnection);
}

void PlaybackController::applyAutoAdvance(int index, const QUrl &url, const QString &title,
                                          const QString &artist, const QString &cover,
                                          int duration, const QString &expectedPath)
{
    if (expectedPath.isEmpty())
        return;
    if (index < 0 || index >= m_queue.count()
        || m_queue.get(index).value(QStringLiteral("path")).toString() != expectedPath) {
        index = m_queue.indexOfPath(expectedPath);
    }
    if (index < 0)
        return;
    {
        QMutexLocker lock(&m_stateMutex);
        if (m_snapshot.size() != m_queue.count())
            m_snapshot = m_queue.tracks();
        m_snapshotIndex = index;
    }
    m_queue.setPlayListIndex(index);
    const QVariantMap item = m_queue.get(index);
    const QString trackTitle = title.isEmpty()
        ? item.value(QStringLiteral("name")).toString() : title;
    const QString trackArtist = artist.isEmpty()
        ? item.value(QStringLiteral("songer")).toString() : artist;
    setMusicTitle(trackTitle);
    setMusicArtist(trackArtist);
    setMusicHash(item.value(QStringLiteral("path")).toString());
    setMusicSource(item.value(QStringLiteral("source"), -1).toInt());
    m_engine.setNoTitle(trackTitle);
    m_engine.setUrlStr(cover.isEmpty()
                           ? QStringLiteral("qrc:/QueMusic/resources/app/musicpic.png")
                           : cover);
    m_engine.setSourceHeaders({});
    m_engine.adoptSource(url);

    if (m_api) {
        m_api->setLyricsData({});
        m_api->setLyricsTranslate({});
    }
    if (musicSource() < 0) {
        setLocalLyricsRequestPath(musicHash());
        if (m_api)
            m_api->readLocalMetadataAsync(musicHash());
    } else if (musicSource() == kSourceWebDav()) {
        const QString cached = WebDavCache::cachedAudioFor(musicHash());
        if (!cached.isEmpty()) {
            setLocalLyricsRequestPath(cached);
            if (m_api)
                m_api->readLocalMetadataAsync(cached);
        }
    } else {
        bindApi();
        if (m_api)
            m_api->getLyricInfo(musicHash(), duration, musicSource());
    }
    noteNowPlaying();
    prepareNext();
}

void PlaybackController::startSource(const QueueTrack &track)
{
    refreshRuntimeSettings();
    if (track.source == kSourceWebDav()) {
        playWebDav(track.path, track.name, track.songer);
        return;
    }
    if (track.source < 0) {
        startLocal(track.path, track.name);
        return;
    }

    bindApi();
    if (!m_api) {
        emit warned(QStringLiteral("在线音源尚未就绪"), 2);
        return;
    }
    setMusicHash(track.path);
    setMusicSource(track.source);
    m_engine.clearNextSource();
    m_engine.fadeOut(m_fadeOutMs);
    m_api->getMusicInfo(track.path, 0, track.source);
}

void PlaybackController::setEngineSource(const QUrl &url)
{
    m_engine.setSource(url);
    m_engine.play();
    if (m_fadeEnabled && m_fadeMs > 0)
        m_engine.fadeInOnNextAudio(m_fadeInMs);
    prepareNext();
}

void PlaybackController::startLocal(const QString &path, const QString &name)
{
    if (m_api)
        m_api->invalidatePlayRequest();
    setLocalLyricsRequestPath(path);
    setMusicTitle(name);
    setMusicArtist(QString());
    setMusicHash(path);
    setMusicSource(-1);
    m_engine.setNoTitle(name);
    m_engine.setUrlStr(QStringLiteral("qrc:/QueMusic/resources/app/musicpic.png"));
    m_engine.setSourceHeaders({});
    setEngineSource(QUrl::fromLocalFile(path));
    if (m_api)
        m_api->readLocalMetadataAsync(path);
}

void PlaybackController::playLocalSong(const QString &path, const QString &name)
{
    if (path.isEmpty())
        return;
    startLocal(path, name);
}

void PlaybackController::playWebDav(const QString &url, const QString &name, const QString &artist)
{
    if (url.isEmpty())
        return;
    if (m_api)
        m_api->invalidatePlayRequest();

    const QString cached = WebDavCache::cachedAudioFor(url);
    if (!cached.isEmpty()) {
        startLocal(cached, name);
        setMusicArtist(artist);
        setMusicSource(kSourceWebDav());
        setMusicHash(url);
        return;
    }

    auto *store = WebDavStore::instance();
    auto *cache = WebDavCache::instance();
    const QString auth = store ? store->authHeaderOfUrl(url) : QString();
    QString lyricsUrl;
    QString coverUrl;
    if (store) {
        const QVariantMap sidecars = store->sidecarsOf(url);
        lyricsUrl = sidecars.value(QStringLiteral("lyricsUrl")).toString();
        coverUrl = sidecars.value(QStringLiteral("coverUrl")).toString();
    }
    if (cache)
        cache->cache(url, auth, lyricsUrl, coverUrl);

    setLocalLyricsRequestPath(QString());
    setMusicTitle(name);
    setMusicArtist(artist);
    setMusicHash(url);
    setMusicSource(kSourceWebDav());
    m_engine.setNoTitle(name);
    m_engine.setUrlStr(coverUrl);
    m_engine.setSourceHeaders(auth.isEmpty() ? QVariantMap() : QVariantMap{{QStringLiteral("Authorization"), auth}});
    setEngineSource(QUrl(url));
}

void PlaybackController::goTo(int index)
{
    if (index < 0 || index >= m_queue.count())
        return;
    m_queue.setPlayListIndex(index);
    {
        QMutexLocker lock(&m_stateMutex);
        if (m_snapshot.size() != m_queue.count())
            m_snapshot = m_queue.tracks();
        m_snapshotIndex = index;
        m_preparedNextIndex = -1;
        m_preparedNextUrl.clear();
        clearPrefetchLocked();
    }
    const QVariantMap item = m_queue.get(index);
    notePlayed(index);
    setMusicTitle(item.value(QStringLiteral("name")).toString());
    setMusicArtist(item.value(QStringLiteral("songer")).toString());
    setMusicHash(item.value(QStringLiteral("path")).toString());
    setMusicSource(item.value(QStringLiteral("source"), -1).toInt());
    startSource(QueueTrack::fromMap(item));
}

void PlaybackController::next(bool random)
{
    const int size = m_queue.count();
    if (size == 0)
        return;
    int index = -1;
    {
        QMutexLocker lock(&m_stateMutex);
        if (m_snapshot.isEmpty())
            m_snapshot = m_queue.tracks();
        if (m_snapshotIndex < 0)
            m_snapshotIndex = m_queue.playListIndex();
        if (random)
            index = randomIndexLocked(true);
        else
            index = m_snapshotIndex < 0 ? 0 : (m_snapshotIndex + 1) % size;
    }
    if (index >= 0)
        goTo(index);
}

void PlaybackController::previous()
{
    const int size = m_queue.count();
    if (size == 0)
        return;
    int current;
    {
        QMutexLocker lock(&m_stateMutex);
        current = m_snapshotIndex;
    }
    if (current < 0)
        current = m_queue.playListIndex();
    goTo(current > 0 ? current - 1 : size - 1);
}

void PlaybackController::startTrack(int index)
{
    goTo(index);
}

bool PlaybackController::enqueue(const QVariantMap &item)
{
    const QString path = item.value(QStringLiteral("path")).toString();
    if (path.isEmpty() || m_queue.indexOfPath(path) != -1)
        return false;
    m_queue.append(item);
    return true;
}

void PlaybackController::playNext(const QVariantMap &item)
{
    const QString path = item.value(QStringLiteral("path")).toString();
    if (path.isEmpty())
        return;
    const int existing = m_queue.indexOfPath(path);
    if (existing != -1) {
        m_queue.moveTo(existing, m_queue.playListIndex() + 1);
        return;
    }
    const int at = m_queue.playListIndex() + 1;
    m_queue.insert(at <= 0 ? m_queue.count() : at, item);
}

void PlaybackController::playItem(const QVariantMap &item)
{
    const QString path = item.value(QStringLiteral("path")).toString();
    if (path.isEmpty())
        return;
    int index = m_queue.indexOfPath(path);
    if (index < 0) {
        if (!enqueue(item))
            return;
        index = m_queue.count() - 1;
    }
    goTo(index);
}

// 暂停和开始播放
void PlaybackController::togglePlay()
{
    refreshRuntimeSettings();
    if (m_engine.source().isEmpty())
        return;
    if (m_engine.playing()) {
        if (m_fadeEnabled && m_fadeMs > 0) {
            m_engine.fadeOut(m_fadeMs);
            m_pauseTimer.start(m_fadeMs);
        } else {
            m_engine.pause();
        }
        return;
    }
    m_engine.play();
    if (m_fadeEnabled && m_fadeMs > 0)
        m_engine.fadeInOnNextAudio(m_fadeInMs);
}

void PlaybackController::setVolume(qreal value)
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

void PlaybackController::updateVolume()
{
    m_engine.setVolume(outVolume());
}

void PlaybackController::stepVolume(qreal delta)
{
    setVolume(m_volume + delta);
}

void PlaybackController::setMuted(bool muted)
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

void PlaybackController::toggleMute()
{
    setMuted(!m_muted);
}

void PlaybackController::seekBy(qint64 ms)
{
    const qint64 target = qMax<qint64>(0, qMin<qint64>(m_engine.duration(),
                                                       m_engine.position() + ms));
    m_engine.setPosition(target);
}

void PlaybackController::seekBack()
{
    QSettings settings;
    const int step = settings.value(QStringLiteral("Options/seekStep"), 5).toInt();
    seekBy(-qint64(qMax(1, step)) * 1000);
}

void PlaybackController::seekForward()
{
    QSettings settings;
    const int step = settings.value(QStringLiteral("Options/seekStep"), 5).toInt();
    seekBy(qint64(qMax(1, step)) * 1000);
}

void PlaybackController::setAbPoint(int which)
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

void PlaybackController::clearAb()
{
    if (m_abA < 0 && m_abB < 0)
        return;
    m_abA = -1;
    m_abB = -1;
    emit abChanged();
}

void PlaybackController::armSleep(int mode, int minutes)
{
    m_sleepMode = qBound(0, mode, 2);
    m_sleepRemain = mode == 1 ? qMax(1, minutes) * 60 : 0;
    if (m_sleepMode == 1 && m_sleepRemain > 0)
        m_sleepTimer.start();
    else
        m_sleepTimer.stop();
    emit sleepChanged();
}

void PlaybackController::stopSleep()
{
    m_sleepMode = 0;
    m_sleepRemain = 0;
    m_sleepTimer.stop();
    emit sleepChanged();
}

QString PlaybackController::sleepLabel() const
{
    if (m_sleepMode == 0)
        return QStringLiteral("关闭");
    if (m_sleepMode == 2)
        return QStringLiteral("本首结束");
    return fmt(qint64(m_sleepRemain) * 1000);
}

void PlaybackController::sleepEnd()
{
    refreshRuntimeSettings();
    stopSleep();
    if (m_fadeEnabled && m_fadeMs > 0) {
        m_engine.fadeOut(m_fadeMs);
        m_pauseTimer.start(m_fadeMs);
    } else {
        m_engine.pause();
    }
    emit warned(QStringLiteral("睡眠定时：已停止播放"), 1);
}

void PlaybackController::setAutoSkipCount(int count)
{
    if (m_autoSkipCount == count)
        return;
    m_autoSkipCount = qMax(0, count);
    emit autoSkipCountChanged();
}

void PlaybackController::onEngineMediaStatusChanged()
{
    if (m_engine.mediaStatus() == AudioEngine::LoadedMedia) {
        setAutoSkipCount(0);
        return;
    }
    if (m_engine.mediaStatus() != AudioEngine::EndOfMedia)
        return;
    if (m_sleepMode == 2) {
        sleepEnd();
        return;
    }
    if (m_cycleIndex.load(std::memory_order_relaxed) == 3) {
        m_engine.stop();
        return;
    }
    const int index = nextIndexForCycle();
    if (index >= 0)
        goTo(index);
}

void PlaybackController::onEnginePlaybackStateChanged()
{
    if (m_engine.playing())
        noteNowPlaying();
}

void PlaybackController::onEnginePositionChanged()
{
    if (!abArmed())
        return;
    if (m_engine.position() >= m_abB)
        m_engine.setPosition(m_abA);
}

void PlaybackController::onEngineError(int code, const QString &message)
{
    Q_UNUSED(code);
    if (m_queue.count() < 2)
        return;
    if (m_autoSkipCount >= 2) {
        setAutoSkipCount(0);
        emit warned(QStringLiteral("连续多首无法播放，已停止自动跳过"), 0);
        return;
    }
    setAutoSkipCount(m_autoSkipCount + 1);
    emit warned(QStringLiteral("当前歌曲无法播放，已自动跳过：") + message, 0);
    next(m_cycleIndex.load(std::memory_order_relaxed) == 2);
}

void PlaybackController::onEngineSourceChanged()
{
    clearAb();
}

void PlaybackController::onUrlplay(const QString &playurl, const QString &title,
                                   const QString &artist, const QString &cover,
                                   const QString &solve, const QString &hash, int source)
{
    Q_UNUSED(solve);
    int index = m_queue.indexOfPath(hash);
    if (index < 0) {
        m_queue.append(QVariantMap{
            {QStringLiteral("name"), title},
            {QStringLiteral("path"), hash},
            {QStringLiteral("songer"), artist},
            {QStringLiteral("source"), source}
        });
        index = m_queue.count() - 1;
        notePlayed(index);
    }
    m_queue.setPlayListIndex(index);
    {
        QMutexLocker lock(&m_stateMutex);
        if (m_snapshot.size() != m_queue.count())
            m_snapshot = m_queue.tracks();
        m_snapshotIndex = index;
        m_preparedNextIndex = -1;
        m_preparedNextUrl.clear();
        clearPrefetchLocked();
    }
    setMusicTitle(title);
    setMusicArtist(artist);
    setMusicHash(hash);
    setMusicSource(source);
    m_engine.setNoTitle(title);
    m_engine.setUrlStr(cover);
    m_engine.setSourceHeaders({});
    setEngineSource(QUrl(playurl));
}

void PlaybackController::onApiWarned(const QString &text, int type)
{
    emit warned(text, type);
}

void PlaybackController::onPlayUrlReady(const QString &hash, const QString &url,
                                        const QString &title, const QString &artist,
                                        const QString &cover, int duration, int source)
{
    Q_UNUSED(source);
    if (url.isEmpty())
        return;

    {
        QMutexLocker lock(&m_stateMutex);
        const int index = m_prefetchIndex;
        if (index < 0 || index >= m_snapshot.size())
            return;
        if (m_snapshot.at(index).path != hash)
            return;
        m_preparedNextIndex = index;
        m_preparedNextUrl = QUrl(url);
        m_prefetchTitle = title;
        m_prefetchArtist = artist;
        m_prefetchCover = cover;
        m_prefetchDuration = duration;
    }
    m_engine.prepareNextSource(QUrl(url));
}

void PlaybackController::onPlayFailed(const QString &hash, int source)
{
    if (hash != musicHash() || source != musicSource())
        return;
    m_engine.setSource(QUrl());
    if (m_queue.count() > 1 && m_autoSkipCount < 5) {
        setAutoSkipCount(m_autoSkipCount + 1);
        next(false);
    }
}

QString PlaybackController::cycleIcon() const
{
    static const QString icons[] = {
        QStringLiteral("\uf118"), QStringLiteral("\uf115"),
        QStringLiteral("\uf0e2"), QStringLiteral("\uf03b")
    };
    return icons[qBound(0, cycleIndex(), 3)];
}

QString PlaybackController::cycleTip() const
{
    static const QString tips[] = {
        QStringLiteral("列表循环"), QStringLiteral("单曲循环"),
        QStringLiteral("随机播放"), QStringLiteral("暂停操作")
    };
    return tips[qBound(0, cycleIndex(), 3)];
}

void PlaybackController::setCycleIndex(int index)
{
    const int clamped = qBound(0, index, 3);
    if (cycleIndex() == clamped)
        return;
    m_cycleIndex.store(clamped, std::memory_order_relaxed);
    QSettings settings;
    settings.setValue(QStringLiteral("Options/cycleIndex"), clamped);
    emit cycleIndexChanged();
    if (!m_engine.source().isEmpty()) {
        {
            QMutexLocker lock(&m_stateMutex);
            m_preparedNextIndex = -1;
            m_preparedNextUrl.clear();
            clearPrefetchLocked();
        }
        prepareNext();
    }
}

void PlaybackController::setMusicTitle(const QString &title)
{
    if (m_musicTitle == title)
        return;
    m_musicTitle = title;
    emit musicTitleChanged();
}

void PlaybackController::setMusicArtist(const QString &artist)
{
    if (m_musicArtist == artist)
        return;
    m_musicArtist = artist;
    emit musicArtistChanged();
}

void PlaybackController::setMusicHash(const QString &hash)
{
    if (m_musicHash == hash)
        return;
    m_musicHash = hash;
    emit musicHashChanged();
}

void PlaybackController::setMusicSource(int source)
{
    if (m_musicSource == source)
        return;
    m_musicSource = source;
    emit musicSourceChanged();
}

void PlaybackController::setLocalLyricsRequestPath(const QString &path)
{
    if (m_localLyricsRequestPath == path)
        return;
    m_localLyricsRequestPath = path;
    emit localLyricsRequestPathChanged();
}

QString PlaybackController::sourceText(int source) const
{
    return source < 0 || source == kSourceWebDav() ? QStringLiteral("本地")
                                                   : QStringLiteral("在线");
}

QString PlaybackController::fmt(qint64 ms) const
{
    if (ms < 0)
        ms = 0;
    const qint64 seconds = ms / 1000;
    return QStringLiteral("%1:%2")
        .arg(seconds / 60, 2, 10, QLatin1Char('0'))
        .arg(seconds % 60, 2, 10, QLatin1Char('0'));
}

void PlaybackController::noteNowPlaying()
{
    if (m_queue.count() == 0 || m_queue.playListIndex() < 0 || m_musicTitle.isEmpty())
        return;
    const QVariantMap item = m_queue.get(m_queue.playListIndex());
    pushHistory(QVariantMap{
        {QStringLiteral("title"), m_musicTitle},
        {QStringLiteral("artist"), m_musicArtist},
        {QStringLiteral("path"), item.value(QStringLiteral("path"))},
        {QStringLiteral("source"), item.value(QStringLiteral("source"))},
        {QStringLiteral("cover"), m_engine.urlStr()},
        {QStringLiteral("duration"), int(m_engine.duration() / 1000)},
        {QStringLiteral("time"), qint64(QDateTime::currentMSecsSinceEpoch())},
        {QStringLiteral("paytype"), 0}
    });
}

void PlaybackController::pushHistory(const QVariantMap &item)
{
    refreshRuntimeSettings();
    const QString path = item.value(QStringLiteral("path")).toString();
    if (path.isEmpty())
        return;
    const QVector<QVariantMap> items = m_history.items();
    if (!items.isEmpty() && items.first().value(QStringLiteral("path")).toString() == path)
        return;
    for (int i = 1; i < items.size(); ++i) {
        if (items.at(i).value(QStringLiteral("path")).toString() == path) {
            m_history.remove(i, 1);
            break;
        }
    }
    m_history.pushFront(item);
    while (m_history.rowCount() > m_historyLimit)
        m_history.remove(m_history.rowCount() - 1, 1);
    m_saveTimer.start();
}

void PlaybackController::loadHistory()
{
    QSettings settings;
    const QJsonArray array = QJsonDocument::fromJson(
        settings.value(QStringLiteral("Options/playHistory")).toString().toUtf8()).array();
    QVector<QVariantMap> items;
    items.reserve(array.size());
    for (const QJsonValue &value : array) {
        const QJsonObject object = value.toObject();
        items.append(QVariantMap{
            {QStringLiteral("title"), object.value(QStringLiteral("title")).toString()},
            {QStringLiteral("artist"), object.value(QStringLiteral("artist")).toString()},
            {QStringLiteral("path"), object.value(QStringLiteral("path")).toString()},
            {QStringLiteral("source"), object.value(QStringLiteral("source")).toInt()},
            {QStringLiteral("cover"), object.value(QStringLiteral("cover")).toString()},
            {QStringLiteral("duration"), object.value(QStringLiteral("duration")).toInt()},
            {QStringLiteral("time"), qint64(object.value(QStringLiteral("time")).toDouble())}
        });
    }
    m_history.setItems(items);
}

void PlaybackController::writeHistory()
{
    QJsonArray array;
    const QVector<QVariantMap> items = m_history.items();
    for (const QVariantMap &item : items) {
        array.append(QJsonObject{
            {QStringLiteral("title"), item.value(QStringLiteral("title")).toString()},
            {QStringLiteral("artist"), item.value(QStringLiteral("artist")).toString()},
            {QStringLiteral("path"), item.value(QStringLiteral("path")).toString()},
            {QStringLiteral("source"), item.value(QStringLiteral("source")).toInt()},
            {QStringLiteral("cover"), item.value(QStringLiteral("cover")).toString()},
            {QStringLiteral("duration"), item.value(QStringLiteral("duration")).toInt()},
            {QStringLiteral("time"), item.value(QStringLiteral("time")).toDouble()}
        });
    }
    QSettings settings;
    settings.setValue(QStringLiteral("Options/playHistory"),
                      QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact)));
    settings.sync();
}

void PlaybackController::clearHistory()
{
    m_history.clear();
    writeHistory();
}

void PlaybackController::flush()
{
    if (m_saveTimer.isActive()) {
        m_saveTimer.stop();
        writeHistory();
    }
}

void PlaybackController::saveQueue()
{
    QJsonArray array;
    for (int i = 0; i < m_queue.count(); ++i) {
        const QVariantMap item = m_queue.get(i);
        array.append(QJsonObject{
            {QStringLiteral("name"), item.value(QStringLiteral("name")).toString()},
            {QStringLiteral("path"), item.value(QStringLiteral("path")).toString()},
            {QStringLiteral("songer"), item.value(QStringLiteral("songer")).toString()},
            {QStringLiteral("source"), item.value(QStringLiteral("source")).toInt()}
        });
    }
    QSettings settings;
    settings.setValue(QStringLiteral("Options/lastQueue"),
                      QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact)));
    settings.setValue(QStringLiteral("Options/lastQueueIndex"), m_queue.playListIndex());
    settings.sync();
}

void PlaybackController::loadQueue()
{
    if (m_queue.count() > 0)
        return;
    QSettings settings;
    if (!settings.value(QStringLiteral("Options/autoRestoreQueue"), true).toBool())
        return;
    const QJsonDocument document = QJsonDocument::fromJson(
        settings.value(QStringLiteral("Options/lastQueue")).toString().toUtf8());
    if (!document.isArray())
        return;
    const QJsonArray array = document.array();
    if (array.isEmpty())
        return;

    QVariantList items;
    items.reserve(array.size());
    for (const QJsonValue &value : array) {
        const QJsonObject object = value.toObject();
        const QString path = object.value(QStringLiteral("path")).toString();
        if (path.isEmpty())
            continue;
        const QJsonValue sourceValue = object.value(QStringLiteral("source"));
        items.append(QVariantMap{
            {QStringLiteral("name"), object.value(QStringLiteral("name")).toString()},
            {QStringLiteral("path"), path},
            {QStringLiteral("songer"), object.value(QStringLiteral("songer")).toString()},
            {QStringLiteral("source"), sourceValue.isUndefined() ? -1 : sourceValue.toInt()}
        });
    }
    m_queue.appendBatch(items, false);
    const int index = settings.value(QStringLiteral("Options/lastQueueIndex"), -1).toInt();
    if (index >= 0 && index < m_queue.count())
        m_queue.setPlayListIndex(index);
}

void PlaybackController::restoreSession()
{
    loadQueue();
}