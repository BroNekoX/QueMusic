// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024-2026 QueMusic Contributors
//
// Portions based on QWindowKit example code:
// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
#include <QtGui/QGuiApplication>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlContext>
#include <QtQuick/QQuickWindow>
#include <QDebug>
#include "cpp/CoverHelper.h"
#include "cpp/ColorExtractor.h"
#include "cpp/GetWave.h"
#include "cpp/DownloadManager.h"
#include "cpp/FolderModel.h"
#include "cpp/Favorites.h"
#include <QWKQuick/qwkquickglobal.h>
#include <qstylehints.h>


int main(int argc, char *argv[]) {
    //qputenv("QSG_RENDER_LOOP", "windows");
    qputenv("QSG_INFO", "1");
    //qputenv("QSG_NO_VSYNC", "1");
    qputenv("QSG_USE_SIMPLE_ANIMATION_DRIVER", "1");
    qmlRegisterType<CoverHelper>("CoverHelper", 1, 0, "CoverHelper");
    qmlRegisterType<ColorExtractor>("ColorExtractor", 1, 0, "ColorExtractor");
    qmlRegisterType<GetWave>("GetWave", 1, 0, "GetWave");
    qmlRegisterType<DownloadManager>("DownloadManager", 1, 0, "DownloadManager");

    //qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");

    //qputenv("QSG_RHI_BACKEND", "opengl"); // other options: d3d11, d3d12, vulkan
    //qputenv("QSG_RHI_HDR", "scrgb"); // other options: hdr10, p3
    //qputenv("QT_QPA_DISABLE_REDIRECTION_SURFACE", "1");

    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QGuiApplication application(argc, argv);

    QQuickWindow::setDefaultAlphaBuffer(true);
    QQmlApplicationEngine engine;

    // settings
    //qDebug()<<avcodec_version();
    //qDebug()<<avcodec_license();

    application.setOrganizationName("BroNekoX");
    application.setOrganizationDomain("com.bronekox.quemusic");
    application.setWindowIcon(QIcon("qrc:/QPlayer/resources/icon.ico"));
    application.setApplicationName("QueMusic");

    // 创建模型实例
    FolderModel *myFolderModel = new FolderModel(&engine);
    myFolderModel->setFilterType("my");

    FolderModel *localFolderModel = new FolderModel(&engine);
    localFolderModel->setFilterType("local");

    SongModel *songModel = new SongModel(&engine);

    FavoritesModel *favSongModel = new FavoritesModel(&engine);
    favSongModel->setFilterType("song");    // 仅显示歌曲收藏

    FavoritesModel *favPlaylistModel = new FavoritesModel(&engine);
    favPlaylistModel->setFilterType("playlist");

    FavoritesModel *favArtistModel = new FavoritesModel(&engine);
    favArtistModel->setFilterType("artist");
    //int wheelLine = application.styleHints();
    //int recommendedLines = QGuiApplication::styleHints()->wheelScrollLines();

    // 暴露给 QML
    engine.rootContext()->setContextProperty("myFolderModel", myFolderModel);
    engine.rootContext()->setContextProperty("localFolderModel", localFolderModel);
    engine.rootContext()->setContextProperty("songModel", songModel);
    engine.rootContext()->setContextProperty("favoritesSong", favSongModel);
    engine.rootContext()->setContextProperty("favoritesList", favPlaylistModel);
    engine.rootContext()->setContextProperty("favoritesArtist", favArtistModel);
    const bool curveRenderingAvailable = true;

    engine.rootContext()->setContextProperty(QStringLiteral("$curveRenderingAvailable"), QVariant(curveRenderingAvailable));
    QWK::registerTypes(&engine);
    engine.load(QUrl(QStringLiteral("qrc:/QueMusic/main.qml")));
    return application.exec();
}
