// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#pragma once

#include <QObject>
#include <QString>

class QAction;
class QMenu;
class QSystemTrayIcon;

// 原生系统托盘：QSystemTrayIcon + QMenu，需要 QApplication（QtWidgets）。
class SystemTrayManager final : public QObject
{
    Q_OBJECT

public:
    explicit SystemTrayManager(QObject *parent = nullptr);
    ~SystemTrayManager() override;

    bool available() const;
    void hide();

    void showMessage(const QString &title, const QString &body);
    void setNowPlaying(const QString &title, const QString &artist);
    void setPlaying(bool playing);

signals:
    void showWindowRequested();
    void playPauseRequested();
    void previousRequested();
    void nextRequested();
    void quitRequested();

private:
    void updateToolTip();

    QSystemTrayIcon *m_tray = nullptr;
    QMenu *m_menu = nullptr;
    QAction *m_nowPlayingAction = nullptr;
    QAction *m_playAction = nullptr;
    QString m_title = QStringLiteral("QueMusic");
    QString m_nowPlaying;
    bool m_playing = false;
};
