// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
#include "MprisMediaSessionBackend.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QDebug>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtDBus/QDBusAbstractAdaptor>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusObjectPath>

namespace {

const QString kService = QStringLiteral("org.mpris.MediaPlayer2.io.github.BroNekoX.QueMusic");
const QString kPath = QStringLiteral("/org/mpris/MediaPlayer2");
const QString kPlayerInterface = QStringLiteral("org.mpris.MediaPlayer2.Player");
const QString kDesktopEntry = QStringLiteral("io.github.BroNekoX.QueMusic");
const QString kTrackPrefix = QStringLiteral("/org/quemusic/QueMusic/track/");

} // namespace

// ---------------------------------------------------------------- root adaptor

class MprisRootAdaptor final : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2")
    Q_PROPERTY(bool CanQuit READ canQuit CONSTANT)
    Q_PROPERTY(bool CanRaise READ canRaise CONSTANT)
    Q_PROPERTY(bool HasTrackList READ hasTrackList CONSTANT)
    Q_PROPERTY(QString Identity READ identity CONSTANT)
    Q_PROPERTY(QString DesktopEntry READ desktopEntry CONSTANT)
    Q_PROPERTY(QStringList SupportedUriSchemes READ supportedUriSchemes CONSTANT)
    Q_PROPERTY(QStringList SupportedMimeTypes READ supportedMimeTypes CONSTANT)

public:
    explicit MprisRootAdaptor(MprisMediaSessionBackend *backend)
        : QDBusAbstractAdaptor(backend)
        , m_backend(backend)
    {
    }

    bool canQuit() const { return true; }
    bool canRaise() const { return true; }
    bool hasTrackList() const { return false; }
    QString identity() const { return QStringLiteral("QueMusic"); }
    QString desktopEntry() const { return kDesktopEntry; }

    QStringList supportedUriSchemes() const
    {
        return {QStringLiteral("file"), QStringLiteral("http"), QStringLiteral("https")};
    }

    QStringList supportedMimeTypes() const
    {
        return {QStringLiteral("audio/mpeg"), QStringLiteral("audio/flac"), QStringLiteral("audio/ogg"),
                QStringLiteral("audio/x-wav"), QStringLiteral("audio/mp4"), QStringLiteral("audio/aac")};
    }

public slots:
    void Raise() { m_backend->requestRaise(); }
    void Quit() { m_backend->requestQuit(); }

private:
    MprisMediaSessionBackend *m_backend;
};

// -------------------------------------------------------------- player adaptor

class MprisPlayerAdaptor final : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2.Player")
    Q_PROPERTY(QString PlaybackStatus READ playbackStatus)
    Q_PROPERTY(QString LoopStatus READ loopStatus)
    Q_PROPERTY(double Rate READ rate)
    Q_PROPERTY(bool Shuffle READ shuffle)
    Q_PROPERTY(QVariantMap Metadata READ metadata)
    Q_PROPERTY(double Volume READ volume WRITE setVolume)
    Q_PROPERTY(qlonglong Position READ position)
    Q_PROPERTY(double MinimumRate READ minimumRate)
    Q_PROPERTY(double MaximumRate READ maximumRate)
    Q_PROPERTY(bool CanGoNext READ canGoNext)
    Q_PROPERTY(bool CanGoPrevious READ canGoPrevious)
    Q_PROPERTY(bool CanPlay READ canPlay)
    Q_PROPERTY(bool CanPause READ canPause)
    Q_PROPERTY(bool CanSeek READ canSeek)
    Q_PROPERTY(bool CanControl READ canControl)

public:
    explicit MprisPlayerAdaptor(MprisMediaSessionBackend *backend)
        : QDBusAbstractAdaptor(backend)
        , m_backend(backend)
    {
        setAutoRelaySignals(true);
    }

    QString playbackStatus() const { return m_backend->playbackStatusText(); }
    QString loopStatus() const { return m_backend->loopStatusText(); }
    double rate() const { return m_backend->rate(); }
    bool shuffle() const { return m_backend->shuffle(); }
    QVariantMap metadata() const { return m_backend->metadata(); }
    double volume() const { return m_backend->volume(); }
    void setVolume(double value) { m_backend->requestVolume(value); }
    qlonglong position() const { return m_backend->positionUs(); }
    double minimumRate() const { return 0.25; }
    double maximumRate() const { return 4.0; }
    bool canGoNext() const { return m_backend->canGoNext(); }
    bool canGoPrevious() const { return m_backend->canGoPrevious(); }
    bool canPlay() const { return m_backend->canPlay(); }
    bool canPause() const { return m_backend->canPause(); }
    bool canSeek() const { return m_backend->canSeek(); }
    bool canControl() const { return true; }

public slots:
    void Next() { m_backend->requestNext(); }
    void Previous() { m_backend->requestPrevious(); }
    void Pause() { m_backend->requestPause(); }
    void PlayPause() { m_backend->requestPlayPause(); }
    void Stop() { m_backend->requestStop(); }
    void Play() { m_backend->requestPlay(); }
    void Seek(qlonglong offset) { m_backend->requestSeekBy(offset); }
    void SetPosition(const QDBusObjectPath &trackId, qlonglong position)
    {
        m_backend->requestSetPosition(trackId.path(), position);
    }
    void OpenUri(const QString &uri) { m_backend->requestOpenUri(uri); }

signals:
    void Seeked(qlonglong position);

public:
    void notifySeeked(qlonglong position) { emit Seeked(position); }

private:
    MprisMediaSessionBackend *m_backend;
};

// --------------------------------------------------------------------- backend

MprisMediaSessionBackend::MprisMediaSessionBackend(QObject *parent)
    : MediaSessionBackend(parent)
    , m_bus(QDBusConnection::sessionBus())
{
}

MprisMediaSessionBackend::~MprisMediaSessionBackend()
{
    shutdown();
}

bool MprisMediaSessionBackend::available() const
{
    return m_registered;
}

void MprisMediaSessionBackend::initialize(QWindow *)
{
    if (m_registered)
        return;
    if (!m_bus.isConnected()) {
        qWarning() << "[MPRIS] 会话总线不可用，系统媒体控制已跳过";
        return;
    }
    if (!m_root)
        m_root = new MprisRootAdaptor(this);
    if (!m_player)
        m_player = new MprisPlayerAdaptor(this);

    if (!m_bus.registerObject(kPath, this, QDBusConnection::ExportAdaptors)) {
        qWarning() << "[MPRIS] 注册对象路径失败:" << kPath;
        return;
    }
    if (!m_bus.registerService(kService)) {
        qWarning() << "[MPRIS] 注册服务名失败（可能被占用）:" << kService;
        m_bus.unregisterObject(kPath);
        return;
    }

    m_registered = true;
    m_clock.start();
    m_baseElapsedMs = 0;
    emit availableChanged();
}

void MprisMediaSessionBackend::shutdown()
{
    if (!m_registered)
        return;
    m_bus.unregisterService(kService);
    m_bus.unregisterObject(kPath);
    m_registered = false;
    emit availableChanged();
}

QString MprisMediaSessionBackend::playbackStatusText() const
{
    switch (m_status) {
    case Playing:
        return QStringLiteral("Playing");
    case Paused:
        return QStringLiteral("Paused");
    case Stopped:
        break;
    }
    return QStringLiteral("Stopped");
}

QString MprisMediaSessionBackend::loopStatusText() const
{
    switch (m_track.cycle) {
    case 1:
        return QStringLiteral("Track");
    case 3:
        return QStringLiteral("None");
    default:
        return QStringLiteral("Playlist");
    }
}

bool MprisMediaSessionBackend::shuffle() const
{
    return m_track.cycle == 2;
}

QString MprisMediaSessionBackend::trackPath() const
{
    const QByteArray digest = QCryptographicHash::hash(m_track.mediaId.toUtf8(),
                                                       QCryptographicHash::Md5).toHex();
    return kTrackPrefix + QString::fromLatin1(digest);
}

QVariantMap MprisMediaSessionBackend::metadata() const
{
    QVariantMap map;
    map.insert(QStringLiteral("mpris:trackid"), QVariant::fromValue(QDBusObjectPath(trackPath())));
    if (m_track.durationMs > 0)
        map.insert(QStringLiteral("mpris:length"), qlonglong(m_track.durationMs) * 1000);
    if (!m_track.cover.isEmpty())
        map.insert(QStringLiteral("mpris:artUrl"), m_track.cover.toString());
    if (!m_track.title.isEmpty())
        map.insert(QStringLiteral("xesam:title"), m_track.title);
    if (!m_track.artist.isEmpty())
        map.insert(QStringLiteral("xesam:artist"), QStringList{m_track.artist});
    if (!m_track.album.isEmpty())
        map.insert(QStringLiteral("xesam:album"), m_track.album);
    if (!m_track.source.isEmpty())
        map.insert(QStringLiteral("xesam:url"), m_track.source.toString());
    return map;
}

qint64 MprisMediaSessionBackend::extrapolatedMs() const
{
    qint64 position = m_basePositionMs;
    if (m_status == Playing && m_clock.isValid())
        position += m_clock.elapsed() - m_baseElapsedMs;
    if (m_track.durationMs > 0)
        position = qMin(position, m_track.durationMs);
    return qMax<qint64>(0, position);
}

qlonglong MprisMediaSessionBackend::positionUs() const
{
    return qlonglong(extrapolatedMs()) * 1000;
}

void MprisMediaSessionBackend::setStatus(PlaybackStatus status)
{
    if (m_status == status)
        return;
    const qint64 position = extrapolatedMs();
    m_status = status;
    m_basePositionMs = position;
    m_baseElapsedMs = m_clock.isValid() ? m_clock.elapsed() : 0;
    updateProperties({{QStringLiteral("PlaybackStatus"), playbackStatusText()}});
}

void MprisMediaSessionBackend::setTrack(const MediaTrackInfo &info)
{
    const bool trackChanged = info.mediaId != m_track.mediaId
                              || info.title != m_track.title
                              || info.artist != m_track.artist
                              || info.album != m_track.album
                              || info.cover != m_track.cover
                              || info.durationMs != m_track.durationMs;
    const bool capsChanged = info.canNext != m_track.canNext
                             || info.canPrevious != m_track.canPrevious
                             || info.canSeek != m_track.canSeek
                             || info.hasMedia != m_track.hasMedia;
    const bool modeChanged = info.cycle != m_track.cycle;

    m_track = info;
    if (trackChanged) {
        m_basePositionMs = 0;
        m_baseElapsedMs = m_clock.isValid() ? m_clock.elapsed() : 0;
    }

    QVariantMap changed;
    if (trackChanged) {
        changed.insert(QStringLiteral("Metadata"), metadata());
        changed.insert(QStringLiteral("CanSeek"), canSeek());
    }
    if (capsChanged) {
        changed.insert(QStringLiteral("CanGoNext"), canGoNext());
        changed.insert(QStringLiteral("CanGoPrevious"), canGoPrevious());
        changed.insert(QStringLiteral("CanPlay"), canPlay());
        changed.insert(QStringLiteral("CanPause"), canPause());
    }
    if (modeChanged) {
        changed.insert(QStringLiteral("LoopStatus"), loopStatusText());
        changed.insert(QStringLiteral("Shuffle"), shuffle());
    }
    if (!changed.isEmpty())
        updateProperties(changed);
}

void MprisMediaSessionBackend::setPosition(qint64 positionMs, qint64 durationMs)
{
    m_basePositionMs = qMax<qint64>(0, positionMs);
    m_baseElapsedMs = m_clock.isValid() ? m_clock.elapsed() : 0;
    if (durationMs > 0 && durationMs != m_track.durationMs) {
        m_track.durationMs = durationMs;
        updateProperties({{QStringLiteral("Metadata"), metadata()}});
    }
}

void MprisMediaSessionBackend::setVolume(qreal volume)
{
    const double value = qBound(0.0, double(volume), 1.0);
    if (qFuzzyCompare(m_volume, value))
        return;
    m_volume = value;
    updateProperties({{QStringLiteral("Volume"), value}});
}

void MprisMediaSessionBackend::setRate(qreal rate)
{
    const double value = qBound(0.0, double(rate), 8.0);
    if (qFuzzyCompare(m_rate, value))
        return;
    m_rate = value;
    updateProperties({{QStringLiteral("Rate"), value}});
}

void MprisMediaSessionBackend::requestPlayPause()
{
    emit togglePlayPauseRequested();
}

void MprisMediaSessionBackend::requestSeekBy(qlonglong offsetUs)
{
    const qint64 target = qMax<qint64>(0, extrapolatedMs() + qint64(offsetUs / 1000));
    m_basePositionMs = target;
    m_baseElapsedMs = m_clock.isValid() ? m_clock.elapsed() : 0;
    if (m_player)
        m_player->notifySeeked(qlonglong(target) * 1000);
    emit seekRequested(target);
}

void MprisMediaSessionBackend::requestSetPosition(const QString &trackId, qlonglong positionUs)
{
    if (!trackId.isEmpty() && trackId != trackPath())
        return;
    const qint64 target = qMax<qint64>(0, qint64(positionUs / 1000));
    m_basePositionMs = target;
    m_baseElapsedMs = m_clock.isValid() ? m_clock.elapsed() : 0;
    if (m_player)
        m_player->notifySeeked(qlonglong(target) * 1000);
    emit seekRequested(target);
}

void MprisMediaSessionBackend::requestVolume(double volume)
{
    const double value = qBound(0.0, volume, 1.0);
    m_volume = value;
    updateProperties({{QStringLiteral("Volume"), value}});
    emit volumeRequested(value);
}

void MprisMediaSessionBackend::requestOpenUri(const QString &uri)
{
    const QUrl url(uri);
    if (!url.isValid())
        return;
    emit uriRequested(url);
}

void MprisMediaSessionBackend::updateProperties(const QVariantMap &changed)
{
    if (!m_registered || changed.isEmpty())
        return;
    QDBusMessage message = QDBusMessage::createSignal(
        kPath, QStringLiteral("org.freedesktop.DBus.Properties"),
        QStringLiteral("PropertiesChanged"));
    message << kPlayerInterface << changed << QStringList();
    m_bus.send(message);
}

#include "MprisMediaSessionBackend.moc"
