// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024-2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects

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
        color: Style.themes.primaryColor
        border.width: 2
        radius: height
        border.color: Style.themes.sideColor

    }
}