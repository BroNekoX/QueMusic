// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// Portions based on QWindowKit example code:
// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
#include <QtWidgets/QApplication>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QSettings>
#include <QDebug>
#include <QFont>
#include <QVersionNumber>
#include <QTimer>
#include "cpp/AccountManager.h"
#include "cpp/AppSettings.h"
#include "cpp/LogManager.h"
#include "cpp/platform/WindowsAppIdentity.h"
#include "cpp/playback/Playback.h"
#include "cpp/SystemTrayManager.h"
#include "cpp/resident/EngineHost.h"
#include "cpp/resident/SessionController.h"
#include "cpp/resident/SmtcController.h"
#include "cpp/resident/UpdateChecker.h"
#include "cpp/resident/WarningBuffer.h"
#include "DesktopLyricsWindow.h"
#include "DesktopMiniPlayerWindow.h"
#include "api/MusicApiService.h"

// 渲染相关开关必须早于 QApplication
static void applyRenderOptions(const QString &configPath)
{
    QSettings opt(configPath + QStringLiteral("/BroNekoX/QueMusic.ini"), QSettings::IniFormat);
    switch (opt.value(QStringLiteral("Options/gpuRenderMode"), 0).toInt()) {
    case 1: qputenv("QSG_RHI_BACKEND", "opengl"); break;
    case 2: qputenv("QSG_RHI_BACKEND", "vulkan"); break;
    case 3: qputenv("QT_QUICK_BACKEND", "software"); break;
    }
    if (opt.value(QStringLiteral("Options/timerAnimator"), 0).toBool())
        qputenv("QSG_NO_VSYNC", "1");
    if (!opt.value(QStringLiteral("Options/qmlAnimator"), 0).toBool())
        qputenv("QSG_USE_SIMPLE_ANIMATION_DRIVER", "1");
}

static void applyCoreMetadata(QCoreApplication &application)
{
    application.setOrganizationName(QStringLiteral("BroNekoX"));
    application.setOrganizationDomain(QStringLiteral("com.bronekox.quemusic"));
    application.setApplicationName(QStringLiteral("QueMusic"));
    application.setApplicationVersion(QStringLiteral("0.7.2"));
}

static void setupApplicationMetadata(QApplication &application)
{
    applyCoreMetadata(application);
    // QIcon 只识别 ":/xxx" 资源路径（"qrc:/xxx" 会加载失败）
    application.setWindowIcon(QIcon(QStringLiteral(":/QueMusic/resources/icon.ico")));
    // Wayland app_id / Flatpak / 桌面集成：必须与 .desktop 文件名、AppStream <id> 一致
    application.setDesktopFileName(QStringLiteral("io.github.BroNekoX.QueMusic"));
}


int main(int argc, char *argv[])
{
    bool selfCheckMode = false;
    for (int i = 1; i < argc; ++i) {
        if (qstrcmp(argv[i], "--self-check") == 0) {
            selfCheckMode = true;
            break;
        }
    }

    if (selfCheckMode) {
        QCoreApplication app(argc, argv);
        applyCoreMetadata(app);

        MusicApiService *api = MusicApiService::create(nullptr, nullptr);
        const QVariantMap report = api->selfCheck();
        const bool ok = report.value(QStringLiteral("ok")).toBool();
        qInfo().noquote() << "[self-check]" << (ok ? "PASS" : "FAIL")
                          << "passed=" << report.value(QStringLiteral("passed")).toInt()
                          << "failed=" << report.value(QStringLiteral("failed")).toInt()
                          << "elapsed=" << report.value(QStringLiteral("elapsedMs")).toInt()
                          << "ms";
        const QStringList issues = report.value(QStringLiteral("issues")).toStringList();
        for (const QString &issue : issues)
            qWarning().noquote() << "[self-check]" << issue;
        delete api;
        return ok ? 0 : 1;
    }

    // AppUserModelID 必须早于 UI；注册表/快捷方式延后到事件循环
    WindowsAppIdentity::applyProcessIdentity();

    const QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    applyRenderOptions(configPath);

    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication application(argc, argv);   // 用 QApplication：托盘图标/原生托盘菜单依赖 QtWidgets
    application.setQuitOnLastWindowClosed(false);   // 常驻托盘：退出统一走 SessionController::quitApp

    QQuickWindow::setDefaultAlphaBuffer(true);
    setupApplicationMetadata(application);

    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, configPath);

    LogManager::create(nullptr, nullptr);   // 日志单例：接管 Qt 消息，写入安装目录/logs
    AccountManager *accountManager = AccountManager::create(nullptr, nullptr);   // 账号单例

    // 设置单例（QML 侧名字仍是 Config）：必须先于 MusicApiService/Playback 构造，
    // 它们的初始化与信号联动都依赖它
    AppSettings::create(nullptr, nullptr);

    MusicApiService::setSharedAccountManager(accountManager);   // 在线音乐 API 单例
    MusicApiService::create(nullptr, nullptr);
    Playback *playback = Playback::create(nullptr, nullptr);

    // ---- 常驻层单例（均先于 QML 引擎加载创建，QML 首次访问复用同一实例）----
    // 警告缓冲：构造即 connect Playback/MusicApiService 的 warned，跨 QML 场景重建保留警告
    WarningBuffer::create(nullptr, nullptr);
    // 静默更新检查：等价原 main.qml autoUpdateTimer（3 秒后检查，开关在内部判断）
    UpdateChecker::create(nullptr, nullptr)->startDeferredCheck();
    // 会话收尾控制器：托盘"退出"与窗口恢复由此接管（原 WindowControls.qml 收尾逻辑）；
    // QML 场景（WindowControls）后续也按单例名 SessionController 直接调 markHidden/quitApp
    SessionController *sessionController = SessionController::create(nullptr, nullptr);

    // 引擎宿主：QML 引擎/主窗口的创建、销毁（轻量模式）与状态机归其管理。
    // 必须在 SessionController::create 之后（构造函数要接它的 windowHiddenToTray
    // 信号），且先于 createEngine()（常驻单例须先就位，QML 首次访问才复用同一实例）。
    // 配置路径存入宿主，createEngine 内 setContextProperty("configDir", ...) 用。
    EngineHost::create(nullptr, nullptr);
    EngineHost::instance()->init(configPath);

    // 系统托盘：C++ 侧实例化并接线（原 main.qml SystemTrayManager 实例块迁出 QML）
    SystemTrayManager *tray = SystemTrayManager::create(nullptr, nullptr);
    tray->setIconSource(QStringLiteral(":/QueMusic/resources/icon.ico"));
    tray->setTitle(QStringLiteral("QueMusic"));
    // 播放状态 → 托盘（原 main.qml playing/nowPlaying 的 QML 绑定，nowPlaying 格式一致）
    const auto updateNowPlaying = [tray, playback] {
        const QString artist = playback->musicArtist();
        tray->setNowPlaying(playback->musicTitle()
                            + (artist.isEmpty() ? QString()
                                                : QStringLiteral(" - ") + artist));
    };
    updateNowPlaying();
    tray->setPlaying(playback->inplay());
    QObject::connect(playback, &Playback::musicTitleChanged, tray, updateNowPlaying);
    QObject::connect(playback, &Playback::musicArtistChanged, tray, updateNowPlaying);
    QObject::connect(playback, &Playback::inplayChanged, tray, [tray, playback] {
        tray->setPlaying(playback->inplay());
    });
    // 托盘菜单播放控制（原 main.qml onPlayPauseRequested/onPreviousRequested/onNextRequested）
    QObject::connect(tray, &SystemTrayManager::playPauseRequested, playback, &Playback::togglePlay);
    QObject::connect(tray, &SystemTrayManager::previousRequested, playback, &Playback::previous);
    QObject::connect(tray, &SystemTrayManager::nextRequested, playback,
                     [playback] { playback->next(false); });

    // Windows SMTC：以进程私有的隐藏宿主窗口注册，随常驻层存活
    // （SMTC 的 QML 桥接管线迁出 QML；主窗口销毁不影响系统媒体控件）
#ifdef Q_OS_WIN
    new SmtcController(playback, &application);
#endif

    // 桌面部件 Widgets 窗口：进程级常驻，构造即按 AppSettings 开关自管显隐
    // （QApplication 非 QWidget，无 parent；窗口显隐/关闭由 AppSettings 开关自管）
    new DesktopLyricsWindow(nullptr);
    new DesktopMiniPlayerWindow(nullptr);
    // 托盘菜单"显示主界面" / 左键单击、双击托盘图标 → 恢复主界面（统一走
    // EngineHost::restore：轻量模式下重新装配引擎，Active 时按记忆形态恢复窗口）
    QObject::connect(tray, &SystemTrayManager::showWindowRequested, &application,
                     [] { EngineHost::instance()->restore(); });
    QObject::connect(tray, &SystemTrayManager::activated, &application,
                     [](SystemTrayManager::ActivationReason reason) {
        // 左键单击 / 双击托盘图标：恢复主界面
        if (reason == SystemTrayManager::Trigger || reason == SystemTrayManager::DoubleClick)
            EngineHost::instance()->restore();
    });
    // 托盘菜单"退出" → 会话保存 + 退出（原 onQuitRequested → win.quitApp()）；
    // 主窗口惰性取自 EngineHost（轻量模式下为 null，仅跳过几何保存）
    QObject::connect(tray, &SystemTrayManager::quitRequested, &application,
                     [sessionController] {
        sessionController->quitApp(EngineHost::instance()->mainWindow());
    });

    // 装配 QML 引擎并加载主界面（原引擎装配段整体迁入 EngineHost）：
    // 常驻单例均已就位，窗口由 main.qml Component.onCompleted 自动 show，
    // 记住窗口开启时按 Config 恢复几何
    EngineHost::instance()->createEngine();

    // 首次运行才写注册表/快捷方式
    QTimer::singleShot(0, &application, [] { WindowsAppIdentity::ensureShellIntegration(); });

    const int exitCode = application.exec();
    delete MusicApiService::instance();
    return exitCode;
}
