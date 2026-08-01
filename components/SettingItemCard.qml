// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024-2026 QueMusic Contributors
//
import QtQuick

Item {
    id: settingItem
    property string label: ""
    property int controlWidth: 160
    property alias controlItem: controlContent.data
    property bool isBigItem: false
    property bool bottomLine: true

    width: settingStack.standWidth
    height: isBigItem ? 104 : 56

    // 标签
    Text {
        id: label
        x: 20; y: 10
        width: 100; height: 36
        text: settingItem.label
        //font.family: textFont.name
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: Style.settings.textmain
        color: Style.themes.fontColor
    }

    Item {
        id: controlContent
        y: settingItem.isBigItem ? 56 : 10
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
