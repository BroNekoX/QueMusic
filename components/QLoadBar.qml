// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024-2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Effects

Item {
    id: root
    width: 200
    height: 36
    property int progress
    Text {
        x: 20
        height: 36
        width: 40
        text: root.progress + "%"
        font.pixelSize: Style.settings.text
        color: Style.themes.textColor
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
    }
    MultiEffect {
        id: multiEffect
        z: 2
        source: progressbar
        anchors.fill: root
        maskEnabled: true
        maskSource: mask
        // 属性抗锯齿
        maskThresholdMin: 0.5
        maskSpreadAtMin: 1.0

    }
    Rectangle {
        id: progressbar
        visible: false
        x: 80
        y: 14
        width: root.progress
        height: 8
        color: Style.themes.themeColor
    }

    Rectangle {
        id: mask
        x: 80
        y: 14
        z: 1
        width: 100
        height: 8
        radius: 4
        color: Style.themes.sideColor
    }
}