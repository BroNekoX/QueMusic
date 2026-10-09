// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QueMusic 1.0

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
        color: Theme.themeColor
        radius: 3
    }
    Text {
        x: 12
        y: 10
        height: 20
        font.pixelSize: Style.textH2
        font.bold: true
        text: root.text
        color: Theme.fontColor
        verticalAlignment: Text.AlignVCenter
    }
}
