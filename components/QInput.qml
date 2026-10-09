// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors

import QtQuick
import QueMusic 1.0

Rectangle {
    id: root
    implicitWidth: 200
    implicitHeight: 36
    radius: Style.labelRadius
    color: Theme.fullColor
    border.width: input.focus ? 2 : 1
    border.color: input.focus ? Theme.themeColor : Theme.sideColor
    signal entered()
    property alias inputText: input.text
    property alias echoMode: input.echoMode
    TextInput {
        id: input
        anchors.fill: parent
        anchors.leftMargin: 14
        anchors.rightMargin: 14
        color: Theme.fontColor
        font.pixelSize: Style.textmain
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        clip: true
        onAccepted: {
            root.entered()
            input.focus = false
        }
    }
}
