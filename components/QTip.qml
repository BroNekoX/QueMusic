// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors

import QtQuick
import QueMusic 1.0
import QtQuick.Controls.Basic

ToolTip {
    id: root
    property int radius: 8
    horizontalPadding: 10
    verticalPadding: 6
    delay: 480
    opacity: visible ? 1 : 0
    Behavior on opacity { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
    contentItem: Text {
        text: root.text
        font.pixelSize: Style.settings.text
        wrapMode: Text.Wrap
        color: "#f5f5f7"
    }
    background: Rectangle {
        anchors.fill: parent
        radius: root.radius
        color: "#e61d1d20"
        border.color: "#ff333437"
        border.width: 1
    }
}
