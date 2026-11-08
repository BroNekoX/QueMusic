// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#ifndef SYSTEMTRAYMANAGER_H
#define SYSTEMTRAYMANAGER_H

#include <QCoreApplication>
#include <QObject>
#include <QString>
#include <QtQmlIntegration/qqmlintegration.h>

class QAction;
class QJSEngine;
class QMenu;
class QQmlEngine;
class QSystemTrayIcon;

// 系统托盘管理器：常驻托盘图标 + 原生右键菜单，托盘交互以信号回发（信号由
// main.cpp 在 C++ 侧接线：恢复窗口/播放控制/退出，QML 不再监听托盘）。
// 依赖 QtWidgets（QSystemTrayIcon/QMenu），应用必须使用 QApplication 启动。
//
// 单例模式仿 cpp/LogManager.h / WarningBuffer：create 工厂 + CppOwnership +
// QML_ELEMENT/QML_SINGLETON，QML 侧按类型名 SystemTrayManager 访问。
class SystemTrayManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool available READ isAvailable CONSTANT FINAL)   // 平台不支持时 QML 侧退回普通关闭
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged FINAL)
    Q_PROPERTY(QString iconSource READ iconSource WRITE setIconSource NOTIFY iconSourceChanged FINAL)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged FINAL)
    Q_PROPERTY(QString nowPlaying READ nowPlaying WRITE setNowPlaying NOTIFY nowPlayingChanged FINAL)
    Q_PROPERTY(bool playing READ isPlaying WRITE setPlaying NOTIFY playingChanged FINAL)

public:
    // QML 单例工厂。集成阶段 main.cpp 提前调用一次（图标/播放状态接线）；
    // 之后 QML 首次访问 SystemTrayManager 时复用同一实例。
    static SystemTrayManager *create(QQmlEngine *qmlEngine, QJSEngine *scriptEngine);

    // C++ 消费端入口；未提前创建时返回 nullptr
    static SystemTrayManager *instance() { return s_instance; }

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

    // 不能给 parent 默认值：否则引擎不会调用 create()，会出现两个实例
    // （同 cpp/AppModels.h / LogManager 的说明）
    explicit SystemTrayManager(QObject *parent);
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

    // 窗口首次隐藏到托盘时的一次性气泡提示（对应原 WindowControls.qml 的
    // trayTipShown 标志，内部记录已提示状态，之后不再弹）
    Q_INVOKABLE void notifyTrayHidden();

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

    static SystemTrayManager *s_instance;

    QSystemTrayIcon *m_tray = nullptr;
    QMenu *m_menu = nullptr;               // QMenu 的 parent 必须是 QWidget，故手动释放
    QAction *m_nowPlayingAction = nullptr; // 需要动态更新文字与可见性
    QAction *m_playAction = nullptr;       // 需要切换“播放/暂停”文字
    QString m_iconSource;
    QString m_title = QStringLiteral("QueMusic");
    QString m_nowPlaying;
    bool m_playing = false;
    bool m_trayTipShown = false;           // 一次性气泡：已提示过则不再弹
};

#endif // SYSTEMTRAYMANAGER_H
