// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors

import QtQuick
import QueMusic 1.0
import QtQuick.Controls.Basic

Rectangle {
    id: root
    implicitWidth: 200
    implicitHeight: 36
    radius: Style.settings.labelRadius
    color: Style.themes.fullColor
    border.width: input.focus ? 2 : 1
    border.color: input.focus ? Style.themes.themeColor : Style.themes.sideColor
    Behavior on border.color { ColorAnimation { duration: 160 } }
    signal entered()
    property alias inputText: input.text
    TextInput {
        id: input
        anchors.fill: parent
        anchors.leftMargin: 14
        anchors.rightMargin: 14
        color: Style.themes.fontColor
        font.pixelSize: Style.settings.textmain
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        clip: true
        onAccepted: {
            root.entered()
            input.focus = false
        }
    }
}
