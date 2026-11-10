#include "SmtcBridge.h"

#include "WindowsSmtcManager.h"

#include "audio/AudioEngine.h"
#include "playback/Playback.h"

#include <QTimer>

SmtcBridge::SmtcBridge(QObject *parent)
    : QObject(parent)
    , m_manager(new WindowsSmtcManager(this))
{
    m_anchor.setFlags(Qt::Tool | Qt::FramelessWindowHint);
    m_anchor.resize(1, 1);
    m_anchor.create();

    connect(m_manager, &WindowsSmtcManager::availableChanged,
            this, &SmtcBridge::availableChanged);
    m_manager->initialize(&m_anchor);

    Playback *playback = Playback::create(nullptr, nullptr);
    if (!playback)
        return;

    AudioEngine *player = playback->player();
    QueueModel *queue = playback->queue();

    connect(m_manager, &WindowsSmtcManager::playPressed,
            playback, &Playback::togglePlay);
    connect(m_manager, &WindowsSmtcManager::pausePressed,
            playback, &Playback::togglePlay);
    connect(m_manager, &WindowsSmtcManager::nextPressed, this, [playback] {
        playback->next(false);
    });
    connect(m_manager, &WindowsSmtcManager::previousPressed,
            playback, &Playback::previous);
    connect(m_manager, &WindowsSmtcManager::seekRequested, this,
            [player](qint64 position) { player->setPosition(position); });

    connect(player, &AudioEngine::sourceChanged, this, [this, playback] {
        playback->clearAb();
        push();
    });
    connect(player, &AudioEngine::durationChanged, this, &SmtcBridge::push);
    connect(player, &AudioEngine::metaDataChanged, this, &SmtcBridge::pushInfo);
    connect(player, &AudioEngine::playbackStateChanged, this, &SmtcBridge::push);
    connect(player, &AudioEngine::positionChanged, this, &SmtcBridge::pushTimeline);

    connect(playback, &Playback::inplayChanged, this, &SmtcBridge::push);
    connect(playback, &Playback::musicTitleChanged, this, &SmtcBridge::pushInfo);
    connect(playback, &Playback::musicArtistChanged, this, &SmtcBridge::pushInfo);
    connect(queue, &QueueModel::countChanged, this, &SmtcBridge::push);
    connect(queue, &QueueModel::playListIndexChanged, this, &SmtcBridge::push);

    m_clock.start();
    updateControls();
}

SmtcBridge::~SmtcBridge()
{
    shutdown();
}

bool SmtcBridge::available() const
{
    return m_manager && m_manager->isAvailable();
}

void SmtcBridge::shutdown()
{
    if (m_manager)
        m_manager->shutdown();
}

void SmtcBridge::push()
{
    if (m_queued)
        return;
    m_queued = true;
    QTimer::singleShot(0, this, [this] {
        m_queued = false;
        updateControls();
        if (!available())
            return;

        Playback *playback = Playback::create(nullptr, nullptr);
        AudioEngine *player = playback->player();
        if (player->duration() > 0)
            m_manager->updateTimeline(0, player->duration());

        if (playback->inplay()) {
            m_manager->setPlaybackStatus(WindowsSmtcManager::Playing);
        } else if (player->playbackState() == AudioEngine::PlayingState
                   || player->playbackState() == AudioEngine::PausedState) {
            m_manager->setPlaybackStatus(WindowsSmtcManager::Paused);
        } else if (player->playbackState() == AudioEngine::StoppedState) {
            m_manager->setPlaybackStatus(WindowsSmtcManager::Stopped);
        } else {
            m_manager->setPlaybackStatus(WindowsSmtcManager::Closed);
        }
    });
}

void SmtcBridge::pushInfo()
{
    if (m_infoQueued)
        return;
    m_infoQueued = true;
    QTimer::singleShot(0, this, [this] {
        m_infoQueued = false;
        if (!available())
            return;

        Playback *playback = Playback::create(nullptr, nullptr);
        AudioEngine *player = playback->player();
        QueueModel *queue = playback->queue();

        QString mediaId;
        if (queue->rowCount() > 0 && queue->playListIndex() >= 0) {
            const QVariantMap item = queue->get(queue->playListIndex());
            mediaId = item.value(QStringLiteral("path")).toString();
        }
        m_manager->updateMediaInfo(playback->musicTitle(), playback->musicArtist(),
                                   player->albumTitle(), player->urlStr(), mediaId);
    });
}

void SmtcBridge::pushTimeline()
{
    if (!available())
        return;

    Playback *playback = Playback::create(nullptr, nullptr);
    AudioEngine *player = playback->player();
    const qint64 position = player->position();
    const qint64 jump = qAbs(position - m_lastPosition);
    if (m_clock.elapsed() - m_lastTimeline < 5000 && jump < 3000
            && player->duration() - position > 5000)
        return;

    m_lastTimeline = m_clock.elapsed();
    m_lastPosition = position;
    m_manager->updateTimeline(position, player->duration());
}

void SmtcBridge::updateControls()
{
    if (!available())
        return;
    Playback *playback = Playback::create(nullptr, nullptr);
    QueueModel *queue = playback->queue();
    const int index = queue->playListIndex();
    m_manager->setControlsEnabled(true, true, index < queue->rowCount() - 1, index > 0);
}
