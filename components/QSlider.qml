// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors

import QtQuick
import QueMusic 1.0
import QtQuick.Controls.Basic
import QtQuick.Effects

Slider {
    id: slider
    width: 200
    height: 36
    from: 0
    to: 100
    stepSize: 1
    snapMode: Slider.SnapOnRelease
    property string valueText: slider.value
    property bool leftText: false

    background: Rectangle {
        x: slider.leftPadding
        y: slider.topPadding + slider.availableHeight / 2 - height / 2
        implicitWidth: 200
        height: 6
        width: slider.availableWidth
        radius: height / 2
        color: Theme.sideColor

        Rectangle {
            width: slider.visualPosition * parent.width
            height: parent.height
            color: Theme.themeColor
            radius: height / 2
            Behavior on color { ColorAnimation { duration: 160 } }
        }
    }

    handle: Rectangle {
        x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
        y: slider.topPadding + slider.availableHeight / 2 - height / 2
        implicitWidth: 15
        implicitHeight: 15
        scale: slider.pressed ? 1.3 : 1
        radius: width / 2
        color: "#ffffff"
        Behavior on scale { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }

        RectangularShadow {
            anchors.fill: parent
            z: -1
            offset.y: 1
            radius: parent.width / 2
            blur: 6
            spread: 0
            color: "#40000000"
        }
    }

    Text {
        id: valueLabel
        x: slider.leftText ? 0 - width - 10 : slider.width + 10
        anchors.verticalCenter: slider.verticalCenter
        font.pixelSize: Style.textmain
        color: Theme.fontColor
        text: slider.valueText
    }
}
