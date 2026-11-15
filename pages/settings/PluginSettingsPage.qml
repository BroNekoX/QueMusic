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
    property color cardColor: Theme.primaryColor
    property int contentY: 0
    property int scrollToY: 0

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
        Item {
            id: musicMod
            visible: false
            width: downloadChildPage.width
            height: downloadChildPage.height
            Rectangle {
                y: 64
                width: page.standWidth
                height: warnModText.implicitHeight + 48
                color: Theme.containColor
                radius: Style.cubeRadius
                border.color: Theme.sideColor
                border.width: 1
                Text {
                    x: 24
                    y: 24
                    font.family: Fonts.icon
                    height: warnModText.implicitHeight
                    text: "\uf11a"
                    color: Theme.themeColor
                    font.pixelSize: Style.texticon
                }
                Text {
                    id: warnModText
                    x: 48
                    y: 24
                    width: parent.width - 64
                    text: "音乐源插件还未开发完成，等待开发者更新喵"
                    wrapMode: Text.Wrap
                    color: Theme.textColor
                    font.bold: false
                    font.pixelSize: Style.textmain
                }
            }
            Text {
                anchors.fill: parent
                text: "音乐源"
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                color: Theme.textColor
                font.pixelSize: 14
            }
        }
        MarketPage {
            id: modShop
            visible: false
            width: downloadChildPage.width
            height: downloadChildPage.height
        }
    }
}
