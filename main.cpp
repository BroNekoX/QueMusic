// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// Portions based on QWindowKit example code:
// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
#include <QtGui/QGuiApplication>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlContext>
#include <QtQuick/QQuickWindow>
#include <QSettings>
#include <QFileInfo>
#include "cpp/FolderModel.h"
#include "cpp/Favorites.h"
#include "cpp/AccountManager.h"
#include "api/MusicApiService.h"
#include "meshgradient/MeshGradientItem.h"
#include <QWKQuick/qwkquickglobal.h>

#include <QtQml/QQmlExtensionPlugin>
Q_IMPORT_QML_PLUGIN(MeshGradientItemPlugin)

extern void qml_register_types_QueMusic();
extern void qml_register_types_MeshGradientItem();

int main(int argc, char *argv[])
{
    // 从Options.ini读取设置，设置一些高级项喵~
    QSettings opt(QFileInfo(QString::fromLocal8Bit(argv[0])).absolutePath()
                  + QStringLiteral("/BroNekoX/QueMusic.ini"), QSettings::IniFormat);
    switch (opt.value(QStringLiteral("Options/gpuRenderMode"), 0).toInt()) {
    case 1: qputenv("QSG_RHI_BACKEND", "opengl"); break;
    case 2: qputenv("QSG_RHI_BACKEND", "vulkan"); break;
    case 3: qputenv("QSG_RHI_BACKEND", "d3d12"); break;
    case 4: qputenv("QT_QUICK_BACKEND", "software"); break;
    }
    if (opt.value(QStringLiteral("Options/timerAnimator"), 0).toBool())
        qputenv("QSG_NO_VSYNC", "1");
    if (opt.value(QStringLiteral("Options/qmlAnimator"), 0).toBool() == false)
        qputenv("QSG_USE_SIMPLE_ANIMATION_DRIVER", "1");
    qputenv("QSG_INFO", "1");

    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QGuiApplication application(argc, argv);

    QQuickWindow::setDefaultAlphaBuffer(true);
    //QQuickWindow::setTextRenderType(QQuickWindow::CurveTextRendering);
    QQmlApplicationEngine engine;

    // 显式注册QML_ELEMENT 类型
    qml_register_types_QueMusic();
    qml_register_types_MeshGradientItem();

    application.setOrganizationName("BroNekoX");
    application.setOrganizationDomain("com.bronekox.quemusic");
    application.setWindowIcon(QIcon("qrc:/QPlayer/resources/icon.ico"));
    application.setApplicationName("QueMusic");

    // 配置统一使用软件目录下的 INI 文件
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       QCoreApplication::applicationDirPath());

    // 创建模型实例
    FolderModel *myFolderModel = new FolderModel(&engine);
    myFolderModel->setFilterType("my");
    FolderModel *localFolderModel = new FolderModel(&engine);
    localFolderModel->setFilterType("local");
    SongModel *songModel = new SongModel(&engine);
    FavoritesModel *favSongModel = new FavoritesModel(&engine);
    favSongModel->setFilterType("song");
    FavoritesModel *favPlaylistModel = new FavoritesModel(&engine);
    favPlaylistModel->setFilterType("playlist");
    FavoritesModel *favArtistModel = new FavoritesModel(&engine);
    favArtistModel->setFilterType("artist");

    // 暴露给 QML
    engine.rootContext()->setContextProperty("myFolderModel", myFolderModel);
    engine.rootContext()->setContextProperty("localFolderModel", localFolderModel);
    engine.rootContext()->setContextProperty("songModel", songModel);
    engine.rootContext()->setContextProperty("favoritesSong", favSongModel);
    engine.rootContext()->setContextProperty("favoritesList", favPlaylistModel);
    engine.rootContext()->setContextProperty("favoritesArtist", favArtistModel);
    AccountManager *accountManager = new AccountManager(&engine);
    engine.rootContext()->setContextProperty("accountManager", accountManager);
    // 在线音乐 API 单例
    MusicApiService::setSharedAccountManager(accountManager);
    engine.rootContext()->setContextProperty("configDir", QCoreApplication::applicationDirPath());
    engine.rootContext()->setContextProperty("$curveRenderingAvailable", true);

    QWK::registerTypes(&engine);
    engine.load(QUrl(QStringLiteral("qrc:/QueMusic/main.qml")));
    return application.exec();
}
