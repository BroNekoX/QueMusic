// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
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

// QML 传入 "qrc:/xxx"，而 QIcon/QFile 只识别 ":/xxx"
QString toResourcePath(const QString &source)
{
    if (source.startsWith(QLatin1String("qrc:")))
        return source.mid(4);
    return source;
}

// QIcon::isNull() 不代表图标真的可用，必须请求一次小尺寸 pixmap 才能确认
QIcon loadIcon(const QString &source)
{
    const QString path = toResourcePath(source);
    if (path.isEmpty())
        return QIcon();

    QIcon icon(path);
    if (!icon.isNull() && !icon.pixmap(QSize(16, 16)).isNull())
        return icon;
    return QIcon();
}

// 解析顺序：QML 指定图标 → 应用窗口图标 → 系统标准图标；
// 图标为空时通知区域什么都不显示，表现为“隐藏到托盘后程序不见了”
QIcon resolveTrayIcon(const QString &source)
{
    const QIcon icon = loadIcon(source);
    if (!icon.isNull())
        return icon;

    if (qApp && !qApp->windowIcon().isNull())
        return qApp->windowIcon();

    if (auto *app = qobject_cast<QApplication *>(qApp))
        return app->style()->standardIcon(QStyle::SP_DesktopIcon);

    return QIcon();
}

} // namespace

SystemTrayManager::SystemTrayManager(QObject *parent)
    : QObject(parent)
    , m_tray(new QSystemTrayIcon(this))
    , m_menu(new QMenu())
{
    QAction *showAction = m_menu->addAction(QStringLiteral("显示主界面"));
    connect(showAction, &QAction::triggered, this, &SystemTrayManager::showWindowRequested);

    m_nowPlayingAction = m_menu->addAction(QString()); // 仅展示当前曲目
    m_nowPlayingAction->setEnabled(false);
    m_nowPlayingAction->setVisible(false);

    m_menu->addSeparator();

    m_playAction = m_menu->addAction(QStringLiteral("播放"));
    connect(m_playAction, &QAction::triggered, this, &SystemTrayManager::playPauseRequested);

    QAction *prevAction = m_menu->addAction(QStringLiteral("上一首"));
    connect(prevAction, &QAction::triggered, this, &SystemTrayManager::previousRequested);

    QAction *nextAction = m_menu->addAction(QStringLiteral("下一首"));
    connect(nextAction, &QAction::triggered, this, &SystemTrayManager::nextRequested);

    m_menu->addSeparator();

    QAction *quitAction = m_menu->addAction(QStringLiteral("退出 QueMusic"));
    connect(quitAction, &QAction::triggered, this, &SystemTrayManager::quitRequested);

    m_tray->setContextMenu(m_menu);
    connect(m_tray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        emit activated(static_cast<ActivationReason>(reason));
    });

    m_tray->setIcon(resolveTrayIcon(m_iconSource));
    updateToolTip();

    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        m_tray->show(); // 常驻显示：窗口隐藏后从托盘恢复
    } else {
        qWarning() << "[系统托盘] 当前平台不支持系统托盘，关闭窗口将直接退出";
    }

    if (m_tray->icon().isNull())
        qWarning() << "[系统托盘] 托盘图标为空，托盘项将不可见（请检查 iconSource 资源路径）";
}

SystemTrayManager::~SystemTrayManager()
{
    m_tray->setContextMenu(nullptr); // 先解绑再释放，避免悬空指针
    delete m_menu;
}

bool SystemTrayManager::isAvailable() const
{
    return QSystemTrayIcon::isSystemTrayAvailable();
}

bool SystemTrayManager::isVisible() const
{
    return m_tray->isVisible();
}

void SystemTrayManager::setVisible(bool visible)
{
    if (m_tray->isVisible() == visible)
        return;
    m_tray->setVisible(visible);
    emit visibleChanged();
}

QString SystemTrayManager::iconSource() const
{
    return m_iconSource;
}

void SystemTrayManager::setIconSource(const QString &source)
{
    if (m_iconSource == source)
        return;
    m_iconSource = source;

    const QIcon icon = resolveTrayIcon(source);
    m_tray->setIcon(icon);
    if (icon.isNull())
        qWarning() << "[系统托盘] 托盘图标加载失败:" << source;
    emit iconSourceChanged();
}

QString SystemTrayManager::title() const
{
    return m_title;
}

void SystemTrayManager::setTitle(const QString &title)
{
    if (m_title == title)
        return;
    m_title = title;
    updateToolTip();
    emit titleChanged();
}

QString SystemTrayManager::nowPlaying() const
{
    return m_nowPlaying;
}

void SystemTrayManager::setNowPlaying(const QString &text)
{
    if (m_nowPlaying == text)
        return;
    m_nowPlaying = text;

    m_nowPlayingAction->setText(text.isEmpty() ? QString()
                                               : QStringLiteral("正在播放：") + text);
    m_nowPlayingAction->setVisible(!text.isEmpty()); // 无曲目时隐藏该条目

    updateToolTip();
    emit nowPlayingChanged();
}

bool SystemTrayManager::isPlaying() const
{
    return m_playing;
}

void SystemTrayManager::setPlaying(bool playing)
{
    if (m_playing == playing)
        return;
    m_playing = playing;
    m_playAction->setText(playing ? QStringLiteral("暂停") : QStringLiteral("播放"));
    emit playingChanged();
}

void SystemTrayManager::showMessage(const QString &title, const QString &body,
                                    int icon, int msecs)
{
    if (!m_tray->isVisible())
        return;
    m_tray->showMessage(title.isEmpty() ? m_title : title, body,
                        static_cast<QSystemTrayIcon::MessageIcon>(icon), msecs);
}

void SystemTrayManager::quitApplication()
{
    QCoreApplication::quit();
}

void SystemTrayManager::updateToolTip()
{
    QString tip = m_title;
    if (!m_nowPlaying.isEmpty())
        tip += QLatin1Char('\n') + m_nowPlaying;
    m_tray->setToolTip(tip);
}
