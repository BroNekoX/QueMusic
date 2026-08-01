// QHead.qml
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024-2026 QueMusic Contributors
//
import QtQuick

Item {
    id: root
    width: 200
    height: 32
    property string text: ""
    Rectangle {
        x: 2
        y: 10
        width: 6
        height: 20
        color: Style.themes.themeColor
        radius: 3
    }
    Text {
        x: 12
        y: 10
        height: 20
        font.pixelSize: Style.settings.textH2
        font.bold: true
        text: root.text
        color: Style.themes.fontColor
        verticalAlignment: Text.AlignVCenter
        //horizontalAlignment: Text.AlignHCenter
    }
}