// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// 统一系统媒体集成入口：接一次线，按平台选后端。
#pragma once

#include "MediaSessionBackend.h"

#include <QElapsedTimer>
#include <QObject>
#include <QWindow>

class Playback;

class MediaSession final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool available READ available NOTIFY availableChanged FINAL)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged FINAL)
    Q_PROPERTY(QString backendName READ backendName CONSTANT FINAL)

public:
    explicit MediaSession(QObject *parent = nullptr);
    ~MediaSession() override;

    bool available() const { return m_backend && m_backend->available(); }
    bool enabled() const { return m_enabled; }
    void setEnabled(bool on);
    QString backendName() const;

    Q_INVOKABLE void shutdown();

signals:
    void availableChanged();
    void enabledChanged();
    void raiseRequested();
    void quitRequested();

private:
    enum Update { Status = 1, Info = 2, Position = 4, Levels = 8 };

    void bindPlayback();
    void applyEnabled();
    void schedule(int flags);
    void flush();
    void updatePosition(bool force);
    MediaSessionBackend::PlaybackStatus statusOf() const;
    MediaTrackInfo trackInfo() const;

    MediaSessionBackend *m_backend = nullptr;
    Playback *m_playback = nullptr;
    QWindow m_anchor;
    QElapsedTimer m_clock;
    qint64 m_lastTimeline = 0;
    qint64 m_lastPosition = -1;
    int m_pending = 0;
    bool m_enabled = true;
    bool m_flushPosted = false;
};
