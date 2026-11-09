// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#include "DesktopWidgetsController.h"

#include "AppSettings.h"
#include "DesktopLyricsWindow.h"
#include "DesktopMiniPlayerWindow.h"

DesktopWidgetsController *DesktopWidgetsController::s_instance = nullptr;

// 单例工厂：静态实例守卫 + C++ 所有权，QML 侧不可接管（与 SystemTrayManager 同一模式）
DesktopWidgetsController *DesktopWidgetsController::create(QQmlEngine *qmlEngine, QJSEngine *scriptEngine)
{
    Q_UNUSED(qmlEngine)
    Q_UNUSED(scriptEngine)
    static DesktopWidgetsController *instance = nullptr;
    if (!instance) {
        instance = new DesktopWidgetsController(QCoreApplication::instance());
        QQmlEngine::setObjectOwnership(instance, QQmlEngine::CppOwnership);
    }
    return instance;
}

namespace {

// 单一窗口的开关同步（两窗口同构，模板保证行为逐字对称）：
//   on=true：已有存活实例只 show 不重复建（实例兜底 hide 过、或销毁尚未落地
//            时开关重新打开等场景复用/兜底）；无实例才全新创建——创建即显示，
//            窗口自身不再自管显隐（原构造内自管理连接已移除）。
//   on=false：立即 hide（视觉上"关闭即消失"，不等 DeferredDelete 落地）+
//            deleteLater 销毁（QPointer 防悬空，封面 QImage / tick 定时器等
//            随析构释放），并显式置空——快速反复拨动时 deleteLater 与重建
//            并存，重建走全新实例，绝不复用 pending 销毁的旧对象。
template<typename W>
void syncWidgetOnSwitch(QPointer<W> &window, bool on)
{
    if (on) {
        if (window) {
            window->show();
            return;
        }
        window = new W(nullptr);
        window->show();
    } else if (window) {
        window->hide();
        window->deleteLater();
        window = nullptr;
    }
}

} // namespace

// 构造时机（main.cpp 保证）：AppSettings::create 已先行（播种读当前开关值），
// Playback / MusicApiService 亦已就位（窗口构造时直连这两个单例）。
DesktopWidgetsController::DesktopWidgetsController(QObject *parent)
    : QObject(parent)
{
    s_instance = this;

    if (AppSettings *settings = AppSettings::instance()) {
        // 播种：启动时开关为开的部件立即创建显示（初值不发 NOTIFY，须显式同步）
        syncWidgetOnSwitch(m_lyrics, settings->desktopLyricsOn());
        syncWidgetOnSwitch(m_mini, settings->desktopMiniPlayerOn());

        // 开关联动：任一开关 NOTIFY 只走自己的分支，两窗口互不影响
        // （原两窗口类内部"连接开关做 show/hide"的自管理连接已移除）
        connect(settings, &AppSettings::desktopLyricsOnChanged,
                this, &DesktopWidgetsController::syncDesktopLyrics);
        connect(settings, &AppSettings::desktopMiniPlayerOnChanged,
                this, &DesktopWidgetsController::syncMiniPlayer);
    }
}

DesktopWidgetsController::~DesktopWidgetsController()
{
    if (s_instance == this)
        s_instance = nullptr;
}

// 桌面歌词：跟随 AppSettings::desktopLyricsOn
void DesktopWidgetsController::syncDesktopLyrics()
{
    AppSettings *settings = AppSettings::instance();
    syncWidgetOnSwitch(m_lyrics, settings && settings->desktopLyricsOn());
}

// 小窗播放器：跟随 AppSettings::desktopMiniPlayerOn
void DesktopWidgetsController::syncMiniPlayer()
{
    AppSettings *settings = AppSettings::instance();
    syncWidgetOnSwitch(m_mini, settings && settings->desktopMiniPlayerOn());
}

// 退出收尾：与开关 false 分支同一销毁路径（hide + deleteLater + 置空）；
// 调用方（SessionController::doQuitApp）随后的 sendPostedEvents 冲刷 DeferredDelete
void DesktopWidgetsController::shutdownAll()
{
    syncWidgetOnSwitch(m_lyrics, false);
    syncWidgetOnSwitch(m_mini, false);
}
