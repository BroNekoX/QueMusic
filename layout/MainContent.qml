// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QueMusic 1.0
import 'qrc:/QueMusic/pages'

// 主体内容区域
Rectangle {
    id: mainContent
    color: Style.secondaryColor //Style.blurOverlayColor
    readonly property int pageHeight: height - 60

    // 页面数组，便于管理
    property list<Item> pages: [
        homePage,   // 0: 首页
        playlistPage,  // 1: 歌单页
        null,     // 2: 空页面
        favouritePage, // 3: 收藏页
        filePage,      // 4: 本地文件页
        downloadPage,   // 5: 下载页
        searchPage    // 6: 搜索页
    ]

    property int pageIndex: 0
    function contentIndexed(choice: int): void {
        if(choice !== mainContent.pageIndex) {
            mainContent.pages[mainContent.pageIndex].visible = false;
            mainContent.pages[mainContent.pageIndex].active = false;
            mainContent.pages[choice].active = true;
            mainContent.pageIndex = choice;
        }
    }
    function finishedLoaderPage(choice: int): void {
        pageAnine.stop();
        pageAnine.target = mainContent.pages[choice];
        pageAnine.start();
    }

    ParallelAnimation {
        id: pageAnine
        property var target
        NumberAnimation {
            property: "opacity"
            target: pageAnine.target
            from: 0
            to: 1
            duration: 320
            easing.type: Easing.OutCubic
        }
        // 位移写 shiftY，由各 Loader 的 Translate 消费；直接动画 y 会逐帧重排整页
        NumberAnimation {
            property: "shiftY"
            target: pageAnine.target
            from: 180
            to: 0
            duration: 320
            easing.type: Easing.OutExpo
        }
    }

    // Home
    Loader {
        id: homePage
        x: 0
        y: 60
        opacity: 1
        width: mainContent.width
        height: mainContent.pageHeight
        visible: true
        active: true
        property real shiftY: 0
        transform: Translate { y: homePage.shiftY }
        sourceComponent: HomePage {}
        onLoaded: { visible = true; mainContent.finishedLoaderPage(0) }
    }

    // 分类
    Loader {
        id: playlistPage
        x: 0
        y: 60
        opacity: 1
        width: mainContent.width
        height: mainContent.pageHeight
        visible: false
        active: false
        property real shiftY: 0
        transform: Translate { y: playlistPage.shiftY }
        sourceComponent: PlaylistPage {}
        onLoaded: { visible = true; mainContent.finishedLoaderPage(1) }
    }

    // 收藏
    Loader {
        id: favouritePage
        x: 0
        y: 60
        opacity: 1
        width: mainContent.width
        height: mainContent.pageHeight
        visible: false
        active: false
        property real shiftY: 0
        transform: Translate { y: favouritePage.shiftY }
        sourceComponent: FavouritePage {}
        onLoaded: { visible = true; mainContent.finishedLoaderPage(3) }
    }


    // 本地文件
    Loader {
        id: filePage
        x: 0
        y: 60
        opacity: 1
        width: mainContent.width
        height: mainContent.pageHeight
        visible: false
        active: false
        property real shiftY: 0
        transform: Translate { y: filePage.shiftY }
        sourceComponent: FilePage {}
        onLoaded: { visible = true; mainContent.finishedLoaderPage(4) }
    }

    // 下载
    Loader {
        id: downloadPage
        x: 0
        y: 60
        opacity: 1
        width: mainContent.width
        height: mainContent.pageHeight
        visible: false
        active: false
        property real shiftY: 0
        transform: Translate { y: downloadPage.shiftY }
        sourceComponent: DownloadPage {}
        onLoaded: { visible = true; mainContent.finishedLoaderPage(5) }
    }

    // 搜索页
    Loader {
        id: searchPage
        x: 0
        y: 60
        opacity: 1
        asynchronous: true
        width: mainContent.width
        height: mainContent.pageHeight
        visible: false
        active: false
        property real shiftY: 0
        transform: Translate { y: searchPage.shiftY }
        sourceComponent: SearchPage {}
        onLoaded: { visible = true; mainContent.finishedLoaderPage(6) }
    }
}
