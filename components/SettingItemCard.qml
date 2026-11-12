// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QueMusic 1.0

Item {
    id: settingItem
    property string label: ""
    property string tip: "这是一个提示，快设置你的可爱内容喵"
    property int controlWidth: 160
    property bool isBigItem: false
    property bool bottomLine: true

    Component.onCompleted: {
        if(!children[3]) return;
        if(isBigItem) {
            children[3].x = 16;
            children[3].y = 60;
            children[3].width = Qt.binding(function(): real { return settingItem.width - 32 });
            children[3].height = 40;
        } else {
            children[3].x = Qt.binding(function(): real { return settingItem.width - children[3].width - 16 });
            children[3].y = 14;
            children[3].width = Qt.binding(function(): real { return settingItem.controlWidth });
            children[3].height = 36;
        }
    }

    width: settingStack.standWidth
    height: isBigItem ? 116 : 64

    Text {
        id: label
        x: 20; y: 13
        width: 100; height: 20
        text: settingItem.label
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: Style.textmain + 1
        color: Theme.fontColor
    }
    Text {
        id: tip
        x: 20; y: 33
        width: 100; height: 20
        text: settingItem.tip
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: Style.textTip
        color: Theme.textColor
    }

    // 底分隔线
    Rectangle {
        visible: settingItem.bottomLine
        width: settingItem.width - 36
        x: 18
        y: settingItem.height - 1
        height: 1
        color: Theme.sideColor
    }

}
