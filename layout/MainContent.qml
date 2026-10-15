// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
pragma ComponentBehavior: Bound
import QtQuick
import QueMusic 1.0
import 'qrc:/QueMusic/pages'

Rectangle {
    id: mainContent
    color: Theme.secondaryColor
    readonly property int pageHeight: height - 60

    // 页面顺序 = 侧边栏导航下标：首页 0 / 分类 1 / 收藏 2 / 本地 3 / 下载 4 / 搜索 5
    property list<Item> pages: [homePage, playlistPage, favouritePage, filePage, downloadPage, searchPage]
    property int pageIndex: 0

    function contentIndexed(choice: int): void {
        if (choice < 0 || choice >= pages.length || choice === pageIndex)
            return;
        pages[pageIndex].visible = false;
        pages[pageIndex].active = false;
        pages[choice].active = true;
        pageIndex = choice;
    }

    // 页面加载完成后淡入 + 上滑；位移写 shiftY，由各 Loader 的 Translate 消费
    function finishedLoaderPage(choice: int): void {
        pageAnine.stop();
        pageAnine.target = pages[choice];
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
            duration: 280
            easing.type: Easing.OutCubic
        }
        NumberAnimation {
            property: "shiftY"
            target: pageAnine.target
            from: 180
            to: 0
            duration: 320
            easing.type: Easing.OutExpo
        }
    }

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
        onLoaded: { visible = true; mainContent.finishedLoaderPage(2) }
    }

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
        onLoaded: { visible = true; mainContent.finishedLoaderPage(3) }
    }

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
        onLoaded: { visible = true; mainContent.finishedLoaderPage(4) }
    }

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
        onLoaded: { visible = true; mainContent.finishedLoaderPage(5) }
    }
}
