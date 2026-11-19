// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//

pragma ComponentBehavior: Bound
import QtQuick
import QueMusic 1.0

Item {
    id: page
    property int standWidth: 0
    property int containWidth: 0
    property int containX: 0
    property color cardColor: Options.cardColor
    property int contentY: 0
    property int scrollToY: 0
    property Item blurSource

    y: 60
    width: parent ? parent.width : 0
    height: parent ? parent.height - 60 : 0
    visible: false

    Text {
        x: 24 + page.containX
        y: 24
        height: 36
        color: Theme.fontColor
        verticalAlignment: Text.AlignVCenter
        text: "插件"
        font.pixelSize: Style.pageTitle
        font.weight: Font.DemiBold
    }

    QBlurTapBar {
        x: 24 + page.containX
        y: 70
        z: 5
        model: ["歌词界面","功能","音乐源","插件市场"]
        tabWidth: 100
        width: 404
        rectXy: Qt.rect(0, 10, width, 40)
        blurSource: downloadChildPage
        onTabChange: (index) => {
            page.contentY = index;
            downloadChildPage.stack(index);
        }
    }
    QPages {
        x: 24 + page.containX
        y: 60
        width: page.standWidth
        height: parent.height - 60
        id: downloadChildPage
        pageList: [uiMod,toolMod,musicMod,modShop]
        PluginManager {
            id: uiMod
            visible: true
            width: downloadChildPage.width
            height: downloadChildPage.height
            pluginType: "lyrics"
        }
        PluginManager {
            id: toolMod
            visible: false
            width: downloadChildPage.width
            height: downloadChildPage.height
            pluginType: "function"
        }
        SourcePluginManager {
            id: musicMod
            visible: false
            width: downloadChildPage.width
            height: downloadChildPage.height
            blurSource: page.blurSource
        }
        MarketPage {
            id: modShop
            visible: false
            width: downloadChildPage.width
            height: downloadChildPage.height
        }
    }
}
