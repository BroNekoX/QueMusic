// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
#include "SmtcMediaSessionBackend.h"

#include "WindowsSmtcManager.h"

SmtcMediaSessionBackend::SmtcMediaSessionBackend(QObject *parent)
    : MediaSessionBackend(parent)
    , m_manager(new WindowsSmtcManager(this))
{
    connect(m_manager, &WindowsSmtcManager::availableChanged,
            this, &MediaSessionBackend::availableChanged);
    connect(m_manager, &WindowsSmtcManager::playPressed,
            this, &MediaSessionBackend::playRequested);
    connect(m_manager, &WindowsSmtcManager::pausePressed,
            this, &MediaSessionBackend::pauseRequested);
    connect(m_manager, &WindowsSmtcManager::nextPressed,
            this, &MediaSessionBackend::nextRequested);
    connect(m_manager, &WindowsSmtcManager::previousPressed,
            this, &MediaSessionBackend::previousRequested);
    connect(m_manager, &WindowsSmtcManager::seekRequested,
            this, &MediaSessionBackend::seekRequested);
}

bool SmtcMediaSessionBackend::available() const
{
    return m_manager->isAvailable();
}

void SmtcMediaSessionBackend::initialize(QWindow *window)
{
    m_manager->initialize(window);
}

void SmtcMediaSessionBackend::shutdown()
{
    m_manager->shutdown();
}

void SmtcMediaSessionBackend::setStatus(PlaybackStatus status)
{
    static const int map[] = {WindowsSmtcManager::Stopped, WindowsSmtcManager::Playing,
                              WindowsSmtcManager::Paused};
    m_manager->setPlaybackStatus(map[qBound(0, int(status), 2)]);
}

void SmtcMediaSessionBackend::setTrack(const MediaTrackInfo &info)
{
    m_manager->updateMediaInfo(info.title, info.artist, info.album,
                               info.cover.toString(), info.mediaId);
    m_manager->setControlsEnabled(true, true,
                                  info.canNext, info.canPrevious);
}

void SmtcMediaSessionBackend::setPosition(qint64 positionMs, qint64 durationMs)
{
    m_manager->updateTimeline(positionMs, durationMs);
}
