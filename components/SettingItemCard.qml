// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QueMusic 1.0

Item {
    id: settingItem
    property string label: ""
    property string tip: "这是一个提示，哈哈哈设置你的内容"
    property int controlWidth: 160
    property alias controlItem: controlContent.data
    property bool isBigItem: false
    property bool bottomLine: true

    width: settingStack.standWidth
    height: isBigItem ? 112 : 64

    // 标签
    Text {
        id: label
        x: 20; y: 13
        width: 100; height: 20
        text: settingItem.label
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: Style.settings.textmain + 1
        //font.bold: true
        color: Style.themes.fontColor
    }
    Text {
        id: tip
        x: 20; y: 33
        width: 100; height: 20
        text: settingItem.tip
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: Style.settings.textTip
        color: Style.themes.textColor
    }

    Item {
        id: controlContent
        y: settingItem.isBigItem ? 60 : 14
        x: settingItem.isBigItem ? 16 : settingItem.width - width - 16
        width: settingItem.isBigItem ? settingItem.width - 32 : settingItem.controlWidth
        height: 36
    }

    // 底分隔线
    Rectangle {
        visible: settingItem.bottomLine
        width: settingItem.width - 36
        x: 18
        y: settingItem.height - 1
        height: 1
        color: Style.themes.sideColor
    }

}
