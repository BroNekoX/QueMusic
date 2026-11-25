// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// 系统媒体集成后端接口：Windows SMTC / Linux MPRIS2 / macOS Now Playing。
#pragma once

#include <QObject>
#include <QString>
#include <QUrl>

class QWindow;

struct MediaTrackInfo
{
    QString title;
    QString artist;
    QString album;
    QString mediaId;
    QUrl cover;
    QUrl source;
    qint64 durationMs = 0;
    int cycle = 0;
    bool hasMedia = false;
    bool canNext = false;
    bool canPrevious = false;
    bool canSeek = false;
};

class MediaSessionBackend : public QObject
{
    Q_OBJECT

public:
    enum PlaybackStatus { Stopped = 0, Playing = 1, Paused = 2 };
    Q_ENUM(PlaybackStatus)

    explicit MediaSessionBackend(QObject *parent = nullptr) : QObject(parent) {}
    ~MediaSessionBackend() override = default;

    virtual bool available() const = 0;
    virtual void initialize(QWindow *window) = 0;
    virtual void shutdown() = 0;
    virtual void setStatus(PlaybackStatus status) = 0;
    virtual void setTrack(const MediaTrackInfo &info) = 0;
    virtual void setPosition(qint64 positionMs, qint64 durationMs) = 0;
    virtual void setVolume(qreal volume) { Q_UNUSED(volume) }
    virtual void setRate(qreal rate) { Q_UNUSED(rate) }

signals:
    void availableChanged();
    void playRequested();
    void pauseRequested();
    void togglePlayPauseRequested();
    void stopRequested();
    void nextRequested();
    void previousRequested();
    void seekRequested(qint64 positionMs);
    void volumeRequested(qreal volume);
    void uriRequested(const QUrl &url);
    void raiseRequested();
    void quitRequested();
};
