// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
#pragma once

#include "MediaSessionBackend.h"

class WindowsSmtcManager;

class SmtcMediaSessionBackend final : public MediaSessionBackend
{
    Q_OBJECT

public:
    explicit SmtcMediaSessionBackend(QObject *parent = nullptr);

    bool available() const override;
    void initialize(QWindow *window) override;
    void shutdown() override;
    void setStatus(PlaybackStatus status) override;
    void setTrack(const MediaTrackInfo &info) override;
    void setPosition(qint64 positionMs, qint64 durationMs) override;

private:
    WindowsSmtcManager *m_manager = nullptr;
};
