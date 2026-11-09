// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#include "SessionController.h"

#include "AppSettings.h"
#include "DesktopWidgetsController.h"
#include "EngineHost.h"
#include "QueueModel.h"
#include "audio/AudioEngine.h"
#include "playback/Playback.h"

#include <QCoreApplication>
#include <QQmlEngine>
#include <QSettings>

SessionController *SessionController::s_instance = nullptr;

// 单例工厂：静态实例守卫 + C++ 所有权，QML 侧不可接管（与 LogManager 同一模式）
SessionController *SessionController::create(QQmlEngine *qmlEngine, QJSEngine *scriptEngine)
{
    Q_UNUSED(qmlEngine)
    Q_UNUSED(scriptEngine)
    static SessionController *instance = nullptr;
    if (!instance) {
        instance = new SessionController(QCoreApplication::instance());
        QQmlEngine::setObjectOwnership(instance, QQmlEngine::CppOwnership);
    }
    return instance;
}

SessionController::SessionController(QObject *parent)
    : QObject(parent)
{
    s_instance = this;
}

SessionController::~SessionController()
{
    if (s_instance == this)
        s_instance = nullptr;
}

void SessionController::saveSession(QQuickWindow *mainWindow)
{
    saveLastSong();
    saveWindowGeometry(mainWindow);

    // 队列持久化：对应原 QML :48-49 的 Playback.saveQueue() / Playback.flush()。
    // 注意：Playback.h 未暴露静态 instance() 访问器，此处复用其 QML 单例工厂
    // Playback::create（幂等：已存在则返回同一实例，main.cpp 启动时也是走这个入口）。
    Playback *playback = Playback::create(nullptr, nullptr);
    playback->saveQueue();
    playback->flush();
}

void SessionController::quitApp(QQuickWindow *mainWindow)
{
    // 收尾整体延迟到下一轮事件：调用方可能是 QML onClosing（窗口关闭事件
    // 处理栈）或托盘菜单动作，若在此栈上同步销毁引擎与窗口，返回时栈上
    // 仍持有已析构对象的上下文（onClosing 的 JS 帧），触发释放后访问崩溃。
    QMetaObject::invokeMethod(this, [this, mainWindow] { doQuitApp(mainWindow); },
                              Qt::QueuedConnection);
}

void SessionController::doQuitApp(QQuickWindow *mainWindow)
{
    // 退出顺序敏感，三件事必须在 quit() 之前、事件循环仍存活时完成：
    //   1. 停住音频引擎——stop() 内部经输出线程 BlockingQueuedConnection 投递，
    //      一旦拖到 QApplication 析构阶段再执行，事件投递与线程收尾相互卡死，
    //      实测阻塞数十秒后触发 Qt 断言崩溃；
    //   2. 销毁 QML 引擎——窗口与边框原子消失；否则 QML 内容在析构深处先没、
    //      原生边框滞留数秒才随 HWND 销毁。DeferredDelete 需手动冲刷：
    //      quit() 之后事件循环不再处理删除队列，窗口不会真正落地销毁；
    //   3. quit()
    Playback *playback = Playback::create(nullptr, nullptr);
    if (AudioEngine *engine = playback ? playback->player() : nullptr)
        engine->stop();

    EngineHost *host = EngineHost::instance();
    if (host && host->mainWindow()) {
        host->destroyEngine();
    }

    // 桌面部件窗口随退出立即销毁——留到 QApplication 收尾阶段会在屏幕上
    // 滞留数十秒；控制器统一 deleteLater（同开关 false 分支），随下方
    // DeferredDelete 冲刷立即销毁
    if (auto *widgets = DesktopWidgetsController::instance())
        widgets->shutdownAll();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    QCoreApplication::quit();
}

// 对应原 WindowControls.qml restoreWindow()：
//   不可见 → 按 hide 前记忆的可见性恢复（Maximized/FullScreen/其余 showNormal）；
//   最小化 → showNormal；最后 raise + requestActivate 抢焦点。
void SessionController::restoreWindow(QQuickWindow *mainWindow)
{
    if (!mainWindow)
        return;
    if (!mainWindow->isVisible()) {
        // m_restoreVisibility 尚未被 markHidden 记录（初值 Hidden）时，
        // 回退读 windowStates：hide() 不会清除最大化/全屏状态位。
        QWindow::Visibility visibility = m_restoreVisibility;
        if (visibility == QWindow::Hidden) {
            const Qt::WindowStates states = mainWindow->windowStates();
            if (states & Qt::WindowMaximized)
                visibility = QWindow::Maximized;
            else if (states & Qt::WindowFullScreen)
                visibility = QWindow::FullScreen;
        }
        if (visibility == QWindow::Maximized)
            mainWindow->showMaximized();
        else if (visibility == QWindow::FullScreen)
            mainWindow->showFullScreen();
        else
            mainWindow->showNormal();
    } else if (mainWindow->visibility() == QWindow::Minimized) {
        mainWindow->showNormal();
    }
    mainWindow->raise();
    mainWindow->requestActivate();
}

// 窗口隐藏到托盘前记录可见性（原 WindowControls.qml hideToTray 的
// restoreVisibility = targetWindow.visibility）；restoreWindow 按此恢复形态。
// hideToTray 入口已挡住不可见窗口，故这里不会记录到 Hidden。
void SessionController::markHidden(QQuickWindow *mainWindow)
{
    if (mainWindow) {
        m_restoreVisibility = mainWindow->visibility();
        // 成功记录隐藏（真实发生了窗口隐藏）才通知：EngineHost 据此启动
        // 轻量模式自动进入的延迟定时器
        emit windowHiddenToTray();
    }
}

// 最后播放歌曲：对应原 QML :29-39。
// 原 QML 写 Options.lastSongs（components/OptionsLastSongs.qml，Settings 类别
// "LastMedia"，属性 name/artist/cover/hash/source），此处直接写同类别同名五键，
// 老用户的 INI 数据无需迁移。与 QML 语义一致：队列非空且 playListIndex 有效时
// 才写入，否则保留旧记录。
void SessionController::saveLastSong()
{
    Playback *playback = Playback::create(nullptr, nullptr);
    QueueModel *queue = playback->queue();
    const int index = queue->playListIndex();
    // 原 QML 只判 count > 0 && idx >= 0（get(idx) 越界返回 undefined，item.path
    // 会抛 TypeError）。C++ 侧补一个上界检查，仅在原本会异常的场景下行为不同。
    if (queue->count() <= 0 || index < 0 || index >= queue->count())
        return;

    // 原 QML 用 q.get(idx)（QVariantMap，字段 name/path/songer/source），
    // C++ 侧直接用 trackAt() 拿 QueueTrack，字段一致且免去 QVariant 转换。
    const QueueTrack item = queue->trackAt(index);

    // 封面：原 QML (p && p.urlStr) || 默认图 —— urlStr 为空串时同样回退默认图。
    AudioEngine *player = playback->player();
    const QString cover = (player && !player->urlStr().isEmpty())
                              ? player->urlStr()
                              : QStringLiteral("qrc:/QueMusic/resources/app/musicpic.png");

    QSettings settings;
    settings.setValue(QStringLiteral("LastMedia/name"), playback->musicTitle());
    settings.setValue(QStringLiteral("LastMedia/artist"), playback->musicArtist());
    settings.setValue(QStringLiteral("LastMedia/cover"), cover);
    settings.setValue(QStringLiteral("LastMedia/hash"), item.path);
    settings.setValue(QStringLiteral("LastMedia/source"), item.source);
}

// 窗口几何：对应原 QML :40-47（Config.rememberWindow 守卫 + 非最大化/全屏才写入）。
// AppSettings::setWinX 等会写透 QSettings("Options/winX") 并 NOTIFY，落盘自动完成。
// AppSettings::instance() 在 self-check 等未提前创建的场景返回 nullptr，判空容错。
void SessionController::saveWindowGeometry(QQuickWindow *mainWindow)
{
    AppSettings *settings = AppSettings::instance();
    if (!settings || !mainWindow || !settings->rememberWindow())
        return;
    const QWindow::Visibility visibility = mainWindow->visibility();
    if (visibility == QWindow::Maximized || visibility == QWindow::FullScreen)
        return;
    settings->setWinX(mainWindow->x());
    settings->setWinY(mainWindow->y());
    settings->setWinW(mainWindow->width());
    settings->setWinH(mainWindow->height());
}
