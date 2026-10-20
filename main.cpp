// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// Portions based on QWindowKit example code:
// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
#include <QtWidgets/QApplication>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlContext>
#include <QStandardPaths>
#include <QtQuick/QQuickWindow>
#include <QSettings>
#include <QTimer>
#include "cpp/AccountManager.h"
#include "cpp/LogManager.h"
#include "cpp/platform/WindowsAppIdentity.h"
#include "api/MusicApiService.h"
#include <QWKQuick/qwkquickglobal.h>

extern void qml_register_types_QueMusic();

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

static void setupApplicationMetadata(QApplication &application)
{
    application.setOrganizationName(QStringLiteral("BroNekoX"));
    application.setOrganizationDomain(QStringLiteral("com.bronekox.quemusic"));
    // QIcon 只识别 ":/xxx" 资源路径（"qrc:/xxx" 会加载失败）
    application.setWindowIcon(QIcon(QStringLiteral(":/QueMusic/resources/icon.ico")));
    application.setApplicationName(QStringLiteral("QueMusic"));
    // Wayland app_id / Flatpak / 桌面集成：必须与 .desktop 文件名、AppStream <id> 一致
    application.setDesktopFileName(QStringLiteral("io.github.BroNekoX.QueMusic"));
}

int main(int argc, char *argv[])
{
    // AppUserModelID 必须早于 UI；注册表/快捷方式延后到事件循环
    WindowsAppIdentity::applyProcessIdentity();

    const QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    applyRenderOptions(configPath);

    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication application(argc, argv);   // 用 QApplication：托盘图标/原生托盘菜单依赖 QtWidgets
    application.setQuitOnLastWindowClosed(false);   // 常驻托盘：退出统一走 QML 的 Qt.quit()

    QQuickWindow::setDefaultAlphaBuffer(true);
    setupApplicationMetadata(application);

    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, configPath);

    QQmlApplicationEngine engine;
    qml_register_types_QueMusic();   // 注册 QML_ELEMENT 类型
    engine.addImportPath(QStringLiteral("qrc:/"));

    LogManager::create(&engine, &engine);   // 日志单例：接管 Qt 消息，写入安装目录/logs
    AccountManager *accountManager = AccountManager::create(&engine, &engine);   // 账号单例

    MusicApiService::setSharedAccountManager(accountManager);   // 在线音乐 API 单例
    // 注入 QML 上下文属性（各单例定义见 cpp/AppModels.h）
    engine.rootContext()->setContextProperty(QStringLiteral("configDir"), configPath);

    QWK::registerTypes(&engine);
    engine.load(QUrl(QStringLiteral("qrc:/QueMusic/main.qml")));

    // 首次运行才写注册表/快捷方式
    QTimer::singleShot(0, &application, [] { WindowsAppIdentity::ensureShellIntegration(); });

    return application.exec();
}
