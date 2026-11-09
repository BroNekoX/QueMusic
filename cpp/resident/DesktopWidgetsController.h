// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#ifndef DESKTOPWIDGETSCONTROLLER_H
#define DESKTOPWIDGETSCONTROLLER_H

#include <QCoreApplication>
#include <QObject>
#include <QPointer>
#include <QtQmlIntegration/qqmlintegration.h>

class DesktopLyricsWindow;
class DesktopMiniPlayerWindow;
class QJSEngine;
class QQmlEngine;

// 桌面部件生命周期控制器：桌面歌词（DesktopLyricsWindow）与小窗播放器
// （DesktopMiniPlayerWindow）两个独立窗口的创建/销毁唯一入口。
// - 按开关创建：AppSettings::desktopLyricsOn / desktopMiniPlayerOn 为 true 时
//   创建对应窗口并显示，两窗口互不依赖、各自独立分支；构造时按当前开关值
//   播种（启动时开关为开的部件立即创建显示——setter 对不变值早退不发
//   NOTIFY，初值须显式同步，见 fb29b41 后开关能正确读回 true）；
// - 关闭即销毁：开关为 false 时（设置页开关或部件窗口关闭按钮 requestClose
//   写回）立即 hide + deleteLater 销毁，不保留任何缓存实例——再次打开开关
//   走全新创建（封面 QImage、tick 定时器等随析构一并释放）。窗口类不再
//   连接开关自管显隐（所有权移交本控制器），业务订阅与 requestClose 写回
//   链路保持不变；
// - 快速反复拨动：deleteLater 与重建并存时互不干扰——销毁前显式置空指针，
//   重建走全新实例；已存活实例再收到 true 只 show 不重复建。
// 单例模式仿 SystemTrayManager / EngineHost：create 工厂 + CppOwnership，
// main.cpp 在 AppSettings::create 之后提前调用（播种要读当前开关值），
// 之后 QML 首次访问复用同一实例。
class DesktopWidgetsController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    // QML 单例工厂。main.cpp 在引擎装配前提前调用一次（必须在 AppSettings::
    // create 之后，构造函数要按当前开关值播种）；之后 QML 首次访问时复用
    // 同一实例。
    static DesktopWidgetsController *create(QQmlEngine *qmlEngine, QJSEngine *scriptEngine);

    // C++ 消费端入口；未提前创建时返回 nullptr
    static DesktopWidgetsController *instance() { return s_instance; }

    // 不能给 parent 默认值：否则引擎不会调用 create()，会出现两个实例
    // （同 SystemTrayManager / EngineHost 的说明）
    explicit DesktopWidgetsController(QObject *parent);
    ~DesktopWidgetsController() override;

    // 退出收尾：销毁所有存活部件窗口（SessionController::doQuitApp 调用；
    // 其后的 sendPostedEvents(DeferredDelete) 把 deleteLater 冲刷落地）
    void shutdownAll();

private:
    static DesktopWidgetsController *s_instance;

    // 开关 → 窗口同步（各窗口独立分支，仅响应自己的开关 NOTIFY）
    void syncDesktopLyrics();
    void syncMiniPlayer();

    // 存活实例（QPointer 防悬空；false 分支销毁前显式置空，不留缓存）
    QPointer<DesktopLyricsWindow> m_lyrics;
    QPointer<DesktopMiniPlayerWindow> m_mini;
};

#endif // DESKTOPWIDGETSCONTROLLER_H
