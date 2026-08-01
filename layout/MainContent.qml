// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024-2026 QueMusic Contributors
//
import QtQuick
import QueMusic 1.0
import 'qrc:/QueMusic/pages'

// 主体内容区域
Rectangle {
    id: mainContent
    color: Style.themes.primaryColor //Style.themes.blurOverlayColor
    readonly property int pageHeight: height - 60

    // 页面数组，便于管理
    property var pages: [
        homePage,   // 0: 首页
        playlistPage,  // 1: 歌单页
        nullPage,     // 2: 空页面
        favouritePage, // 3: 收藏页
        filePage,      // 4: 本地文件页
        downloadPage,   // 5: 下载页
        searchPage    // 6: 搜索页
    ]

    property int pageIndex: 0
    //signal stackChange(int index)
    function contentIndexed(choice) {
        if(choice !== mainContent.pageIndex) {
            pageAnine.stop()
            pageAnimeo.target = mainContent.pages[choice]
            pageAnimey.target = mainContent.pages[choice]
            pageAnine.start()
            mainContent.pages[mainContent.pageIndex].visible = false
            mainContent.pages[choice].visible = true
            mainContent.pageIndex = choice
        }
    }

    ParallelAnimation {
        id: pageAnine
        NumberAnimation {
            id: pageAnimeo
            property: "opacity"
            from: 0
            to: 1
            duration: 300
            easing.type: Easing.OutExpo
        }
        NumberAnimation {
            id: pageAnimey
            property: "y"
            from: 180
            to: 60
            duration: 300
            easing.type: Easing.OutExpo
        }
    }

    // Home
    HomePage {
        id: homePage
        x: 0
        y: 60
        opacity: 1
        width: mainContent.width
        height: mainContent.pageHeight
        visible: true
    }

    // 分类
    PlaylistPage {
        id: playlistPage
        x: 0
        y: 60
        opacity: 1
        width: mainContent.width
        height: mainContent.pageHeight
        visible: false
    }

    // null
    Item {
        id: nullPage
        x: 0
        y: 60
        opacity: 1
        visible: false
    }

    // 收藏
    FavouritePage {
        id: favouritePage
        x: 0
        y: 60
        opacity: 1
        width: mainContent.width
        height: mainContent.pageHeight
        visible: false
    }


    // 本地文件
    FilePage {
        id: filePage
        x: 0
        y: 60
        opacity: 1
        width: mainContent.width
        height: mainContent.pageHeight
        visible: false
    }

    // 下载
    DownloadPage {
        id: downloadPage
        x: 0
        y: 60
        opacity: 1
        width: mainContent.width
        height: mainContent.pageHeight
        visible: false
    }
    
    // 搜索页
    SearchPage {
        id: searchPage
        x: 0
        y: 60
        opacity: 1
        width: mainContent.width
        height: mainContent.pageHeight
        visible: false
    }
}