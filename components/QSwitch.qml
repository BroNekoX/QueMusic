// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors

import QtQuick
import QueMusic 1.0
import QtQuick.Effects

Rectangle {
    id: root
    width: 160
    height: 36
    color: isBack ? Theme.secondaryColor : "transparent"
    radius: isBack ? Style.labelRadius : 0
    signal toggled(bool switchTrue)

    property string text: switchTrue ? "开" : "关"
    property bool isBack: false
    property bool switchTrue: false
    property bool letRight: false

    Text {
        x: root.letRight ? root.width - 120 : 0
        y: 0
        width: root.width / 2
        height: root.height
        text: root.text
        color: Theme.textColor
        font.pixelSize: Style.text
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
    }

    Rectangle {
        id: track
        x: root.letRight ? root.width - 52 : root.width / 2
        y: 6
        width: 44
        height: 24
        radius: 12
        color: root.switchTrue ? Theme.themeColor : Theme.sideColor
        Behavior on color { ColorAnimation { duration: 220 } }

        Rectangle {
            id: knob
            x: root.switchTrue ? 22 : 2
            y: 2
            width: 20
            height: 20
            radius: 10
            color: "#ffffff"
            Behavior on x { NumberAnimation { duration: 300; easing.type: Easing.OutBack; easing.overshoot: 1.1 } }

            RectangularShadow {
                anchors.fill: knob
                z: -1
                offset.y: 1
                radius: 10
                blur: 5
                spread: 0
                color: "#33000000"
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            root.toggled(root.switchTrue)
        }
    }
}
