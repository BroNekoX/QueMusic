// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
#include "MediaSession.h"

#include "Config.h"
#include "QueueModel.h"
#include "audio/AudioEngine.h"
#include "playback/Playback.h"

#if defined(QUEMUSIC_HAS_SMTC)
#  include "SmtcMediaSessionBackend.h"
#elif defined(Q_OS_MACOS)
#  include "MacMediaSessionBackend.h"
#elif defined(QUEMUSIC_HAS_MPRIS)
#  include "MprisMediaSessionBackend.h"
#endif

#include <QFileInfo>
#include <QMetaObject>
#include <QUrl>

namespace {

class NullMediaSessionBackend final : public MediaSessionBackend
{
public:
    explicit NullMediaSessionBackend(QObject *parent = nullptr) : MediaSessionBackend(parent) {}

    bool available() const override { return false; }
    void initialize(QWindow *) override {}
    void shutdown() override {}
    void setStatus(PlaybackStatus) override {}
    void setTrack(const MediaTrackInfo &) override {}
    void setPosition(qint64, qint64) override {}
};

MediaSessionBackend *createBackend(QObject *parent)
{
#if defined(QUEMUSIC_HAS_SMTC)
    return new SmtcMediaSessionBackend(parent);
#elif defined(Q_OS_MACOS)
    return new MacMediaSessionBackend(parent);
#elif defined(QUEMUSIC_HAS_MPRIS)
    return new MprisMediaSessionBackend(parent);
#else
    return new NullMediaSessionBackend(parent);
#endif
}

QUrl normalizedCover(const QString &cover)
{
    const QString trimmed = cover.trimmed();
    if (trimmed.isEmpty())
        return {};
    const QUrl url(trimmed);
    const QString scheme = url.scheme().toLower();
    if (scheme == QStringLiteral("file") || scheme == QStringLiteral("http")
        || scheme == QStringLiteral("https"))
        return url;
    const QFileInfo file(trimmed);
    return file.isFile() ? QUrl::fromLocalFile(file.absoluteFilePath()) : QUrl();
}

QUrl sourceUrl(const QString &hash)
{
    if (hash.isEmpty())
        return {};
    const QUrl url(hash);
    if (!url.scheme().isEmpty())
        return url;
    const QFileInfo file(hash);
    return file.exists() ? QUrl::fromLocalFile(file.absoluteFilePath()) : QUrl();
}

constexpr int kPositionPushMs = 5000;
constexpr int kPositionJumpMs = 1500;
constexpr int kPositionTickMs = 1000;

} // namespace

MediaSession::MediaSession(QObject *parent)
    : QObject(parent)
{
    m_backend = createBackend(this);
    m_anchor.setFlags(Qt::Tool | Qt::FramelessWindowHint);
    m_anchor.resize(1, 1);
    m_anchor.create();

    connect(m_backend, &MediaSessionBackend::availableChanged, this, [this] {
        emit availableChanged();
        if (available())
            schedule(Info | Status | Position);
    });
    connect(m_backend, &MediaSessionBackend::raiseRequested, this, &MediaSession::raiseRequested);
    connect(m_backend, &MediaSessionBackend::quitRequested, this, &MediaSession::quitRequested);

    bindPlayback();
    m_clock.start();
    m_enabled = Config::instance()->mediaControls();
    applyEnabled();
}

MediaSession::~MediaSession()
{
    shutdown();
}

void MediaSession::setEnabled(bool on)
{
    if (m_enabled == on)
        return;
    m_enabled = on;
    Config::instance()->set_mediaControls(on);
    applyEnabled();
    emit enabledChanged();
}

QString MediaSession::backendName() const
{
#if defined(QUEMUSIC_HAS_SMTC)
    return QStringLiteral("Windows SMTC");
#elif defined(Q_OS_MACOS)
    return QStringLiteral("macOS Now Playing");
#elif defined(QUEMUSIC_HAS_MPRIS)
    return QStringLiteral("MPRIS2");
#else
    return QString();
#endif
}

void MediaSession::shutdown()
{
    if (!m_backend)
        return;
    m_pending = 0;
    m_backend->shutdown();
    emit availableChanged();
}

void MediaSession::applyEnabled()
{
    if (!m_backend)
        return;
    if (m_enabled) {
        m_backend->initialize(&m_anchor);
        schedule(Info | Status | Position);
    } else {
        m_backend->shutdown();
    }
    emit availableChanged();
}

void MediaSession::bindPlayback()
{
    Playback *playback = Playback::create(nullptr, nullptr);
    if (!playback)
        return;
    m_playback = playback;

    AudioEngine *player = playback->player();
    QueueModel *queue = playback->queue();

    connect(m_backend, &MediaSessionBackend::togglePlayPauseRequested,
            playback, &Playback::togglePlay);
    connect(m_backend, &MediaSessionBackend::playRequested, this, [playback] {
        if (!playback->inplay())
            playback->togglePlay();
    });
    connect(m_backend, &MediaSessionBackend::pauseRequested, this, [playback] {
        if (playback->inplay())
            playback->togglePlay();
    });
    connect(m_backend, &MediaSessionBackend::stopRequested, this, [playback] {
        if (playback->inplay())
            playback->togglePlay();
    });
    connect(m_backend, &MediaSessionBackend::nextRequested, this,
            [playback] { playback->next(false); });
    connect(m_backend, &MediaSessionBackend::previousRequested, playback, &Playback::previous);
    connect(m_backend, &MediaSessionBackend::seekRequested, this,
            [player](qint64 position) { player->setPosition(position); });
    connect(m_backend, &MediaSessionBackend::volumeRequested, this,
            [playback](qreal volume) { playback->setVolume(volume); });
    connect(m_backend, &MediaSessionBackend::uriRequested, this, [playback](const QUrl &url) {
        if (!url.isLocalFile())
            return;
        const QFileInfo file(url.toLocalFile());
        if (file.isFile())
            playback->playLocalSong(file.absoluteFilePath(), file.completeBaseName());
    });

    const auto mark = [this](Update type) {
        return [this, type] { schedule(type); };
    };

    connect(player, &AudioEngine::durationChanged, this, mark(Info));
    connect(player, &AudioEngine::metaDataChanged, this, mark(Info));
    connect(player, &AudioEngine::urlStrChanged, this, mark(Info));
    connect(player, &AudioEngine::playbackStateChanged, this, mark(Status));
    connect(player, &AudioEngine::playbackRateChanged, this, mark(Levels));
    connect(player, &AudioEngine::positionChanged, this, [this, player] {
        const qint64 jump = qAbs(player->position() - m_lastPosition);
        if (jump < kPositionJumpMs && m_clock.elapsed() - m_lastTimeline < kPositionTickMs)
            return;
        schedule(Position);
    });
    connect(playback, &Playback::inplayChanged, this, mark(Status));
    connect(playback, &Playback::musicTitleChanged, this, mark(Info));
    connect(playback, &Playback::musicArtistChanged, this, mark(Info));
    connect(playback, &Playback::musicHashChanged, this, mark(Info));
    connect(playback, &Playback::volumeChanged, this, mark(Levels));
    connect(queue, &QueueModel::countChanged, this, mark(Info));
    connect(queue, &QueueModel::playListIndexChanged, this, mark(Info));
}

void MediaSession::schedule(int flags)
{
    m_pending |= flags;
    if (m_flushPosted || !m_enabled)
        return;
    m_flushPosted = true;
    QMetaObject::invokeMethod(this, [this] {
        m_flushPosted = false;
        flush();
    }, Qt::QueuedConnection);
}

void MediaSession::flush()
{
    if (!m_enabled || !m_playback || !m_backend || !m_backend->available()) {
        m_pending = 0;
        return;
    }

    const int flags = m_pending;
    m_pending = 0;

    if (flags & Info) {
        m_backend->setTrack(trackInfo());
        m_lastTimeline = 0;
        m_lastPosition = -1;
    }
    if (flags & (Info | Status))
        m_backend->setStatus(statusOf());
    if (flags & (Info | Levels)) {
        m_backend->setVolume(m_playback->volume());
        m_backend->setRate(m_playback->player()->playbackRate());
    }
    if (flags & (Info | Status | Position))
        updatePosition((flags & (Info | Status)) != 0);
}

void MediaSession::updatePosition(bool force)
{
    AudioEngine *player = m_playback->player();
    const qint64 position = player->position();
    const qint64 duration = player->duration();
    if (!force) {
        const qint64 jump = qAbs(position - m_lastPosition);
        if (m_clock.elapsed() - m_lastTimeline < kPositionPushMs && jump < kPositionJumpMs
            && duration - position > kPositionPushMs)
            return;
    }
    m_lastTimeline = m_clock.elapsed();
    m_lastPosition = position;
    m_backend->setPosition(position, duration);
}

MediaSessionBackend::PlaybackStatus MediaSession::statusOf() const
{
    if (!m_playback)
        return MediaSessionBackend::Stopped;
    if (m_playback->inplay())
        return MediaSessionBackend::Playing;
    if (!m_playback->musicHash().isEmpty() || m_playback->player()->duration() > 0)
        return MediaSessionBackend::Paused;
    return MediaSessionBackend::Stopped;
}

MediaTrackInfo MediaSession::trackInfo() const
{
    MediaTrackInfo info;
    if (!m_playback)
        return info;

    AudioEngine *player = m_playback->player();
    QueueModel *queue = m_playback->queue();

    info.title = m_playback->musicTitle();
    info.artist = m_playback->musicArtist();
    info.album = player->albumTitle();
    info.cover = normalizedCover(player->urlStr());
    info.mediaId = m_playback->musicHash();
    info.source = sourceUrl(info.mediaId);
    info.durationMs = player->duration();
    info.cycle = m_playback->cycleIndex();
    info.hasMedia = !info.mediaId.isEmpty() || info.durationMs > 0;

    const int rows = queue->rowCount();
    const int index = queue->playListIndex();
    const bool multi = rows > 1 && index >= 0;
    info.canNext = multi;
    info.canPrevious = multi;
    info.canSeek = info.hasMedia;
    return info;
}
