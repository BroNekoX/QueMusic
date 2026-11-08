// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#include "SessionController.h"

#include "AppSettings.h"
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
    saveSession(mainWindow);
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
