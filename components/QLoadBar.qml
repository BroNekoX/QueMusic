// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors

import QtQuick
import QueMusic 1.0
import QtQuick.Controls.Basic
import QtQuick.Effects

Item {
    id: root
    width: 200
    height: 36
    property int to: 100
    property int position: 0
    property string valueText: root.position
    property int barHeight: 8
    property color barColor: Theme.themeColor
    property color color: Theme.sideColor
    property bool load: false
    property int animeX: 0
    property int animeWidth: 0

    Rectangle {
        id: mainBar
        x: 16
        anchors.verticalCenter: parent.verticalCenter
        layer.enabled: true
        width: root.width - 20
        height: root.barHeight
        radius: height / 2
        color: root.color
    }

    MultiEffect {
        id: multiEffect
        source: positionBar
        anchors.fill: mainBar
        maskEnabled: true
        maskSource: mainBar
        // 边缘抗锯齿
        maskThresholdMin: 0.5
        maskSpreadAtMin: 1.0
    }

    Item  {
        id: positionBar
        anchors.fill: mainBar
        visible: false
        Rectangle {
            height: root.barHeight
            x: root.load ? root.animeX : 0
            width: root.load ? root.animeWidth : root.position / root.to * mainBar.width
            color: root.barColor
        }
    }

    SequentialAnimation {
        id: loadAnime
        running: root.load
        loops: Animation.Infinite

        // ── 阶段 1：条从左边缘冒出并伸展到 70% 宽 ──
        ParallelAnimation {
            NumberAnimation {
                target: root; property: "animeX"
                from: mainBar.width * -0.1
                to: mainBar.width
                duration: 1000
                //easing.type: Easing.InOutCubic
            }
            NumberAnimation {
                target: root; property: "animeWidth"
                from: mainBar.width * 0.1
                to: mainBar.width * 1.0
                duration: 500
                //easing.type: Easing.InOutCubic
            }
        }

        // ── 阶段 2：整体右移，同时宽度收窄至 0，从右边滑出 ──
        ParallelAnimation {
            NumberAnimation {
                target: root; property: "animeX"
                from: mainBar.width * -1.0
                to: mainBar.width
                duration: 700
                //easing.type: Easing.InOutCubic
            }
            NumberAnimation {
                target: root; property: "animeWidth"
                from: mainBar.width * 1.0
                to: mainBar.width * 0.1
                duration: 700
                easing.type: Easing.InCubic
            }
        }

        PauseAnimation { duration: 300 }
    }

    Text {
        id: valueLabel
        x: 10 - width
        visible: !root.load
        anchors.verticalCenter: root.verticalCenter
        font.pixelSize: Style.textmain
        color: Theme.fontColor
        text: root.valueText
    }
}
