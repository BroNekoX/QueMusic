// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#ifndef SESSIONCONTROLLER_H
#define SESSIONCONTROLLER_H

#include <QCoreApplication>
#include <QObject>
#include <QtQuick/QQuickWindow>
#include <QtQml/qqmlregistration.h>

class QJSEngine;
class QQmlEngine;

// 会话收尾控制器：把 layout/WindowControls.qml 的 toClosing()/quitApp()/
// restoreWindow() 收尾逻辑移植到纯 C++ 常驻层。轻量模式下 QML 引擎会被销毁，
// 退出前的会话保存（最后播放歌曲、窗口几何、播放队列）不能再依赖 QML 场景，
// 改由本类完成。
//
// 单例模式仿 cpp/LogManager.h / WarningBuffer：create 工厂 + CppOwnership +
// QML_ELEMENT/QML_SINGLETON，QML 侧按类型名 SessionController 访问（窗口
// 关闭路径 hideToTray/toClosing 直接调 markHidden/quitApp），C++ 侧（main.cpp
// 托盘接线）用 instance()。
class SessionController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

signals:
    // 窗口已隐藏到托盘（markHidden 成功记录可见性后发出，C++ 侧常驻信号）。
    // 轻量模式自动进入的触发源：EngineHost 据此启动延迟销毁定时器。
    void windowHiddenToTray();

public:
    // QML 单例工厂。集成阶段 main.cpp 提前调用一次；之后 QML 首次访问
    // SessionController 时复用同一实例。
    static SessionController *create(QQmlEngine *qmlEngine, QJSEngine *scriptEngine);

    // C++ 消费端入口；未提前创建时返回 nullptr
    static SessionController *instance() { return s_instance; }

    // 不能给 parent 默认值：否则引擎不会调用 create()，会出现两个实例
    // （同 cpp/AppModels.h / LogManager 的说明）
    explicit SessionController(QObject *parent);
    ~SessionController() override;

    // 保存退出前的会话状态：
    //   1. 最后播放歌曲 → QSettings "LastMedia/{name,artist,cover,hash,source}"
    //      五键（对应原 QML Options.lastSongs，Settings 类别 "LastMedia"；
    //      仅队列非空且 playListIndex 有效时写入，否则保留旧记录）
    //   2. 窗口几何 → AppSettings winX/winY/winW/winH（setter 内部自动写透
    //      QSettings 并落盘；仅 rememberWindow 开启且窗口非最大化/全屏时写入）
    //   3. 播放队列 → Playback::saveQueue() + flush()
    // mainWindow 为空（窗口可能已销毁）时跳过几何保存，其余照常。
    Q_INVOKABLE void saveSession(QQuickWindow *mainWindow);

    // 退出应用：先保存会话，再请求事件循环退出。
    // 对应原 QML 的 win.quitApp() → toClosing → tray.quitApplication()
    // （内部含 saveSession，WindowsControls.toClosing 的非托盘分支直接调本方法）。
    Q_INVOKABLE void quitApp(QQuickWindow *mainWindow);

    // 恢复主窗口：对应原 WindowControls.qml restoreWindow()（托盘单击/双击、
    // 托盘菜单"显示主界面"的恢复入口）。隐藏前的可见性由 markHidden() 记录
    // 到 m_restoreVisibility；尚未记录时回退读窗口自身的 windowStates（hide()
    // 不清除状态位），保证"最大化隐藏到托盘 → 恢复仍是最大化"的形态不丢。
    void restoreWindow(QQuickWindow *mainWindow);

    // 窗口隐藏到托盘前由 QML（hideToTray）调用：把隐藏前的可见性交给常驻层
    // （对应原 QML hideToTray 里的 restoreVisibility = targetWindow.visibility）。
    Q_INVOKABLE void markHidden(QQuickWindow *mainWindow);

private:
    void saveLastSong();
    void saveWindowGeometry(QQuickWindow *mainWindow);

    static SessionController *s_instance;

    // 窗口隐藏前的可见性（对应原 QML restoreVisibility 属性），供 restoreWindow 恢复
    QWindow::Visibility m_restoreVisibility = QWindow::Hidden;
};

#endif // SESSIONCONTROLLER_H
