// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#ifndef SYSTEMTRAYMANAGER_H
#define SYSTEMTRAYMANAGER_H

#include <QObject>
#include <QString>
#include <QtQmlIntegration/qqmlintegration.h>

class QAction;
class QMenu;
class QSystemTrayIcon;

// 系统托盘管理器：常驻托盘图标 + 原生右键菜单，托盘交互以信号回发给 QML。
// 依赖 QtWidgets（QSystemTrayIcon/QMenu），应用必须使用 QApplication 启动。
class SystemTrayManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool available READ isAvailable CONSTANT)   // 平台不支持时 QML 侧退回普通关闭
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged)
    Q_PROPERTY(QString iconSource READ iconSource WRITE setIconSource NOTIFY iconSourceChanged)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(QString nowPlaying READ nowPlaying WRITE setNowPlaying NOTIFY nowPlayingChanged)
    Q_PROPERTY(bool playing READ isPlaying WRITE setPlaying NOTIFY playingChanged)

public:
    // 与 QSystemTrayIcon 的枚举一一对应，供 QML 直接使用
    enum ActivationReason {
        Unknown = 0,
        Context = 1,
        DoubleClick = 2,
        Trigger = 3,
        MiddleClick = 4
    };
    Q_ENUM(ActivationReason)

    enum MessageIcon {
        NoIcon = 0,
        Information = 1,
        Warning = 2,
        Critical = 3
    };
    Q_ENUM(MessageIcon)

    explicit SystemTrayManager(QObject *parent = nullptr);
    ~SystemTrayManager() override;

    bool isAvailable() const;

    bool isVisible() const;
    void setVisible(bool visible);

    QString iconSource() const;
    void setIconSource(const QString &source);

    QString title() const;
    void setTitle(const QString &title);

    QString nowPlaying() const;
    void setNowPlaying(const QString &text);

    bool isPlaying() const;
    void setPlaying(bool playing);

    Q_INVOKABLE void showMessage(const QString &title, const QString &body,
                                 int icon = Information, int msecs = 4000);

    // 显式结束进程：窗口可能正隐藏在托盘里，QWindow::close() 对不可见窗口不生效
    Q_INVOKABLE void quitApplication();

signals:
    void visibleChanged();
    void iconSourceChanged();
    void titleChanged();
    void nowPlayingChanged();
    void playingChanged();

    // 托盘图标被点击（Trigger/DoubleClick 用于恢复窗口）
    void activated(SystemTrayManager::ActivationReason reason);

    void showWindowRequested();
    void playPauseRequested();
    void previousRequested();
    void nextRequested();
    void quitRequested();

private:
    void updateToolTip();

    QSystemTrayIcon *m_tray = nullptr;
    QMenu *m_menu = nullptr;               // QMenu 的 parent 必须是 QWidget，故手动释放
    QAction *m_nowPlayingAction = nullptr; // 需要动态更新文字与可见性
    QAction *m_playAction = nullptr;       // 需要切换“播放/暂停”文字
    QString m_iconSource;
    QString m_title = QStringLiteral("QueMusic");
    QString m_nowPlaying;
    bool m_playing = false;
};

#endif // SYSTEMTRAYMANAGER_H
