// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// macOS：Now Playing，控制中心与媒体键都走这里。
#pragma once

#include "MediaSessionBackend.h"

class MacMediaSessionBackend final : public MediaSessionBackend
{
    Q_OBJECT

public:
    explicit MacMediaSessionBackend(QObject *parent = nullptr);
    ~MacMediaSessionBackend() override;

    bool available() const override;
    void initialize(QWindow *window) override;
    void shutdown() override;
    void setStatus(PlaybackStatus status) override;
    void setTrack(const MediaTrackInfo &info) override;
    void setPosition(qint64 positionMs, qint64 durationMs) override;

private:
    void publish();

    MediaTrackInfo m_track;
    qint64 m_positionMs = 0;
    PlaybackStatus m_status = Stopped;
    bool m_active = false;
};
