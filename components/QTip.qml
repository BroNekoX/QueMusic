// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Controls.Basic

ToolTip {
    id: root
    property int radius: height / 2
    horizontalPadding: 12
    verticalPadding: 8
    delay: 360
    opacity: visible ? 1 : 0
    Behavior on opacity { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }
    //属性: parent visible text
    background: Rectangle {
        anchors.fill: parent
        color: "#fffafbfd"
        border.width: 2
        radius: height
        border.color: "#ffeaebed"

    }
}