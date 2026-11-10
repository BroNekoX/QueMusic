// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// Portions based on QWindowKit example code:
// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
#include <QApplication>
#include <QIcon>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlEngine>
#include <QStandardPaths>
#include <QtQuick/QQuickWindow>
#include <QCoreApplication>
#include <QSettings>
#include <QDebug>
#include <QFont>
#include <QVersionNumber>
#include <QTimer>
#include "cpp/AccountManager.h"
#include "cpp/LogManager.h"
#include "widgets/App.h"
#include "widgets/Config.h"
#include "widgets/AppSettings.h"
#include "cpp/platform/WindowsAppIdentity.h"
#include "cpp/playback/Playback.h"
#include "api/MusicApiService.h"
#include <QWKQuick/qwkquickglobal.h>

extern void qml_register_types_QueMusic();

// 渲染相关开关必须早于 QApplication
static void applyRenderOptions(const QString &configPath)
{
    const Config::RenderOptions options = Config::readRenderOptions(configPath);
    switch (options.gpuRenderMode) {
    case 1: qputenv("QSG_RHI_BACKEND", "opengl"); break;
    case 2: qputenv("QSG_RHI_BACKEND", "vulkan"); break;
    case 3: qputenv("QT_QUICK_BACKEND", "software"); break;
    }
    if (options.timerAnimator)
        qputenv("QSG_NO_VSYNC", "1");
    if (!options.qmlAnimator)
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
#if defined(Q_OS_WIN)
    application.setWindowIcon(QIcon(QStringLiteral(":/QueMusic/resources/icon.ico")));
#else
    application.setWindowIcon(QIcon(QStringLiteral(":/QueMusic/resources/icon.png")));
#endif
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
    QApplication application(argc, argv);   // 原生托盘/菜单依赖 QtWidgets
    application.setQuitOnLastWindowClosed(false);

    QQuickWindow::setDefaultAlphaBuffer(true);
    setupApplicationMetadata(application);

    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, configPath);

    QQmlEngine engine;
    qml_register_types_QueMusic();   // 注册 QML_ELEMENT 类型
    engine.addImportPath(QStringLiteral("qrc:/"));

    LogManager::create(&engine, &engine);   // 日志单例：接管 Qt 消息，写入安装目录/logs
    AccountManager *accountManager = AccountManager::create(&engine, &engine);   // 账号单例

    MusicApiService::setSharedAccountManager(accountManager);   // 在线音乐 API 单例
    MusicApiService::create(nullptr, nullptr);
    Playback::create(nullptr, nullptr);
    // 注入 QML 上下文属性（各单例定义见 cpp/AppModels.h）
    engine.rootContext()->setContextProperty(QStringLiteral("configDir"), configPath);

    QWK::registerTypes(&engine);
    App::create(&engine, nullptr);
    App::instance()->showMainWindow();

    // 首次运行才写注册表/快捷方式
    QTimer::singleShot(0, &application, [] { WindowsAppIdentity::ensureShellIntegration(); });

    const int exitCode = application.exec();
    AppSettings::sync();
    delete MusicApiService::instance();
    return exitCode;
}
