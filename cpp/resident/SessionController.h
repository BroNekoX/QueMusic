// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#ifndef SESSIONCONTROLLER_H
#define SESSIONCONTROLLER_H

#include <QObject>
#include <QtQuick/QQuickWindow>

// 会话收尾控制器：把 layout/WindowControls.qml 的 toClosing()/quitApp() 收尾逻辑
// 移植到纯 C++ 常驻层。轻量模式下 QML 引擎会被销毁，退出前的会话保存（最后播放
// 歌曲、窗口几何、播放队列）不能再依赖 QML 场景，改由本类完成。
//
// 设计取舍：普通类（非 QML 单例），由 main 持有并在退出流程中调用：
//   - 窗口关闭按钮 / 系统关闭 → "最小化到托盘"分支仍由 QML/托盘层处理；
//   - 真正退出（托盘菜单"退出"等）→ main 调 quitApp(mainWindow)，
//     对应原 QML 的 win.quitApp()。
// 集成阶段（main.cpp）负责创建实例并接线；本文件不参与 QML 类型注册。
class SessionController : public QObject
{
    Q_OBJECT

public:
    explicit SessionController(QObject *parent = nullptr);

    // 保存退出前的会话状态：
    //   1. 最后播放歌曲 → QSettings "LastMedia/{name,artist,cover,hash,source}"
    //      五键（对应原 QML Options.lastSongs，Settings 类别 "LastMedia"；
    //      仅队列非空且 playListIndex 有效时写入，否则保留旧记录）
    //   2. 窗口几何 → AppSettings winX/winY/winW/winH（setter 内部自动写透
    //      QSettings 并落盘；仅 rememberWindow 开启且窗口非最大化/全屏时写入）
    //   3. 播放队列 → Playback::saveQueue() + flush()
    // mainWindow 为空（窗口可能已销毁）时跳过几何保存，其余照常。
    void saveSession(QQuickWindow *mainWindow);

    // 退出应用：先保存会话，再请求事件循环退出。
    // 与原 QML 链路 quitApp → toClosing → tray.quitApplication() 等价
    // （SystemTrayManager::quitApplication 内部即 QCoreApplication::quit()）。
    void quitApp(QQuickWindow *mainWindow);

    // 恢复主窗口：对应原 WindowControls.qml restoreWindow()（托盘单击/双击、
    // 托盘菜单"显示主界面"的恢复入口）。隐藏前的可见性由 markHidden() 记录
    // 到 m_restoreVisibility；尚未记录时回退读窗口自身的 windowStates（hide()
    // 不清除状态位），保证"最大化隐藏到托盘 → 恢复仍是最大化"的形态不丢。
    void restoreWindow(QQuickWindow *mainWindow);

private:
    void saveLastSong();
    void saveWindowGeometry(QQuickWindow *mainWindow);

    // 窗口隐藏前的可见性（对应原 QML restoreVisibility 属性），供 restoreWindow 恢复
    QWindow::Visibility m_restoreVisibility = QWindow::Hidden;
};

#endif // SESSIONCONTROLLER_H
