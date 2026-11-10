// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#include "SystemTrayManager.h"

#include <QAction>
#include <QApplication>
#include <QCoreApplication>
#include <QIcon>
#include <QMenu>
#include <QSize>
#include <QStyle>
#include <QSystemTrayIcon>

namespace {

QIcon trayIcon()
{
    QApplication *app = qobject_cast<QApplication *>(QCoreApplication::instance());
    if (!app)
        return {};

    QIcon icon = app->windowIcon();
    if (!icon.isNull() && !icon.pixmap(QSize(16, 16)).isNull())
        return icon;

    icon = QIcon::fromTheme(QStringLiteral("io.github.BroNekoX.QueMusic"));
    return icon.isNull() ? app->style()->standardIcon(QStyle::SP_DesktopIcon) : icon;
}

}

SystemTrayManager::SystemTrayManager(QObject *parent)
    : QObject(parent)
    , m_tray(new QSystemTrayIcon(this))
    , m_menu(new QMenu())
{
    QAction *showAction = m_menu->addAction(QStringLiteral("显示主界面"));
    connect(showAction, &QAction::triggered, this, &SystemTrayManager::showWindowRequested);

    m_nowPlayingAction = m_menu->addAction(QString());
    m_nowPlayingAction->setEnabled(false);
    m_nowPlayingAction->setVisible(false);

    m_menu->addSeparator();

    m_playAction = m_menu->addAction(QStringLiteral("播放"));
    connect(m_playAction, &QAction::triggered, this, &SystemTrayManager::playPauseRequested);

    QAction *previousAction = m_menu->addAction(QStringLiteral("上一首"));
    connect(previousAction, &QAction::triggered, this, &SystemTrayManager::previousRequested);

    QAction *nextAction = m_menu->addAction(QStringLiteral("下一首"));
    connect(nextAction, &QAction::triggered, this, &SystemTrayManager::nextRequested);

    m_menu->addSeparator();

    QAction *quitAction = m_menu->addAction(QStringLiteral("退出 QueMusic"));
    connect(quitAction, &QAction::triggered, this, &SystemTrayManager::quitRequested);

    m_tray->setContextMenu(m_menu);
    m_tray->setIcon(trayIcon());
    updateToolTip();

    connect(m_tray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
            emit showWindowRequested();
    });

    if (available())
        m_tray->show();
    else
        qWarning() << "[系统托盘] 当前平台不支持系统托盘，关闭窗口将直接退出";
}

SystemTrayManager::~SystemTrayManager()
{
    m_tray->hide();
    m_tray->setContextMenu(nullptr);
    delete m_menu;
}

bool SystemTrayManager::available() const
{
    return QSystemTrayIcon::isSystemTrayAvailable();
}

void SystemTrayManager::hide()
{
    m_tray->hide();
}

void SystemTrayManager::showMessage(const QString &title, const QString &body)
{
    if (m_tray->isVisible())
        m_tray->showMessage(title, body, QSystemTrayIcon::Information, 4000);
}

void SystemTrayManager::setNowPlaying(const QString &title, const QString &artist)
{
    const QString text = artist.isEmpty() ? title : title + QStringLiteral(" - ") + artist;
    if (m_nowPlaying == text)
        return;

    m_nowPlaying = text;
    m_nowPlayingAction->setText(text.isEmpty() ? QString() : QStringLiteral("正在播放：") + text);
    m_nowPlayingAction->setVisible(!text.isEmpty());
    updateToolTip();
}

void SystemTrayManager::setPlaying(bool playing)
{
    if (m_playing == playing)
        return;

    m_playing = playing;
    m_playAction->setText(playing ? QStringLiteral("暂停") : QStringLiteral("播放"));
}

void SystemTrayManager::updateToolTip()
{
    QString tip = m_title;
    if (!m_nowPlaying.isEmpty())
        tip += QLatin1Char('\n') + m_nowPlaying;
    m_tray->setToolTip(tip);
}
