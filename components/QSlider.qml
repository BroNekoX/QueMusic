// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Controls.Basic

Slider {
    id: slider
    width: 200
    height: 36
    from: 0
    to: 100
    stepSize: 1
    snapMode: Slider.SnapOnRelease
    property real valueText: slider.value
    property bool leftText: false

    background: Rectangle {
        x: slider.leftPadding
        y: slider.topPadding + slider.availableHeight / 2 - height / 2
        implicitWidth: 200
        height: 6
        width: slider.availableWidth
        radius: height / 2
        color: Style.themes.borderColor

        Rectangle {
            // slider.visualPosition 可视比例
            width: slider.visualPosition * parent.width
            height: parent.height
            color: Style.themes.themeColor
            radius: height /2
        }
    }

    handle: Rectangle {
        x: slider.leftPadding + slider.visualPosition * (slider.availableWidth-width)
        y: slider.topPadding + slider.availableHeight /2 - height/2
        implicitWidth: 16
        implicitHeight: 16
        radius: implicitHeight / 2
        color: slider.pressed ? Style.themes.secondaryColor : Style.themes.primaryColor
        border.width: 3
        border.color: Style.themes.themeColor
    }

    Text {
        id: valueLabel
        x: slider.leftText ? 0 - width - 10 : slider.width + 10
        anchors.verticalCenter: slider.verticalCenter
        font.pixelSize: Style.settings.textmain
        color: Style.themes.fontColor
        text: slider.valueText
    }
}