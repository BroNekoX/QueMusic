// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#ifndef SMTCCONTROLLER_H
#define SMTCCONTROLLER_H

#include <QObject>
#include <QWindow>

class Playback;
class WindowsSmtcManager;

// Windows SMTC（系统媒体传输控件）常驻控制器：把 SMTC 的整条 QML 桥接管线
// 翻译成 C++。原实现把 SMTC 注册在主窗口 HWND 上，且推送/接收逻辑活在
// QML 场景里；轻量模式下主窗口与 QML 引擎会被销毁，SMTC 必须随常驻层存活：
//   - 改用进程私有的隐藏宿主 QWindow 注册（ISystemMediaTransportControlsInterop::
//     GetForWindow），主窗口销毁不影响系统媒体控件；
//   - 推送去重（queued/infoQueued → 零延迟单发定时器）、时间线节流（5 秒周期 /
//     3 秒跳变）、prev/next 可用性等管线逻辑全部 C++ 化，构造函数一次接线。
//
// 非 Windows 平台整体为空操作（WindowsSmtcManager 本身已有空实现）。
class SmtcController : public QObject
{
    Q_OBJECT

public:
    // playback 由 main.cpp 传入（Playback.h 未暴露静态 instance() 访问器）；
    // parent 持有本控制器（main.cpp 传 &application，随应用退出销毁）。
    explicit SmtcController(Playback *playback, QObject *parent);
    ~SmtcController() override;

private:
    void push();            // 播放状态/时间线整帧推送（queued 去重合并）
    void pushInfo();        // 曲目元信息推送（infoQueued 去重合并）
    void updateControls();  // prev/next 可用性（队列首尾判断）
    void updateTimeline();  // 播放位置推送（5 秒周期 / 3 秒跳变节流）

    Playback *m_playback = nullptr;
    QWindow *m_hostWindow = nullptr;  // 隐藏宿主窗口：SMTC 注册用的 HWND（非主窗口）
    WindowsSmtcManager *m_mgr = nullptr;
    bool m_queued = false;
    bool m_infoQueued = false;
    qint64 m_lastTimelineMs = 0;      // 上次位置推送的墙钟时间（Date.now() 等价）
    qint64 m_lastPositionMs = 0;      // 上次位置推送的播放位置（ms）
};

#endif // SMTCCONTROLLER_H
