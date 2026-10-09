// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QueMusic 1.0

Item {
    id: settingItem
    property string label: ""
    property int controlWidth: 160
    property bool isBigItem: false

    width: settingStack.standWidth
    height: isBigItem ? 76 : 36

    Text {
        id: label
        x: 0; y: 0
        width: 100; height: 36
        text: settingItem.label
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: Style.textmain
        color: Theme.fontColor
    }

}
