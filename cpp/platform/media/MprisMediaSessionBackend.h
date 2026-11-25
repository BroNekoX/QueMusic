// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// Linux/BSD：MPRIS2，KDE/GNOME 的媒体键与媒体面板都走这里。
#pragma once

#include "MediaSessionBackend.h"

#include <QDBusConnection>
#include <QElapsedTimer>
#include <QString>
#include <QVariantMap>

class MprisRootAdaptor;
class MprisPlayerAdaptor;

class MprisMediaSessionBackend final : public MediaSessionBackend
{
    Q_OBJECT

public:
    explicit MprisMediaSessionBackend(QObject *parent = nullptr);
    ~MprisMediaSessionBackend() override;

    bool available() const override;
    void initialize(QWindow *window) override;
    void shutdown() override;
    void setStatus(PlaybackStatus status) override;
    void setTrack(const MediaTrackInfo &info) override;
    void setPosition(qint64 positionMs, qint64 durationMs) override;
    void setVolume(qreal volume) override;
    void setRate(qreal rate) override;

    // 供 MPRIS adaptor 查询
    QString playbackStatusText() const;
    QString loopStatusText() const;
    bool shuffle() const;
    QVariantMap metadata() const;
    QString trackPath() const;
    qlonglong positionUs() const;
    double volume() const { return m_volume; }
    double rate() const { return m_rate; }
    bool canGoNext() const { return m_track.canNext; }
    bool canGoPrevious() const { return m_track.canPrevious; }
    bool canPlay() const { return m_track.hasMedia; }
    bool canPause() const { return m_track.hasMedia; }
    bool canSeek() const { return m_track.canSeek; }

    // 供 MPRIS adaptor 触发
    void requestPlay() { emit playRequested(); }
    void requestPause() { emit pauseRequested(); }
    void requestPlayPause();
    void requestStop() { emit stopRequested(); }
    void requestNext() { emit nextRequested(); }
    void requestPrevious() { emit previousRequested(); }
    void requestSeekBy(qlonglong offsetUs);
    void requestSetPosition(const QString &trackId, qlonglong positionUs);
    void requestVolume(double volume);
    void requestOpenUri(const QString &uri);
    void requestRaise() { emit raiseRequested(); }
    void requestQuit() { emit quitRequested(); }

private:
    void updateProperties(const QVariantMap &changed);
    qint64 extrapolatedMs() const;

    QDBusConnection m_bus;
    MprisRootAdaptor *m_root = nullptr;
    MprisPlayerAdaptor *m_player = nullptr;
    MediaTrackInfo m_track;
    PlaybackStatus m_status = Stopped;
    QElapsedTimer m_clock;
    qint64 m_basePositionMs = 0;
    qint64 m_baseElapsedMs = 0;
    qreal m_volume = 1.0;
    qreal m_rate = 1.0;
    bool m_registered = false;
};
