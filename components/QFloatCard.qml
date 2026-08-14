// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Effects

Rectangle {
    id: root
    width: 240
    height: 120
    radius: Style.settings.cubeRadius
    color: Style.themes.primaryColor
    border.color: Style.themes.fullColor
    border.width: 1
    signal clicked()
    property alias controlItem: cardArea.data
    transform: Translate {
        id: translateTransform
        x: 0
        y: 0
        Behavior on y { NumberAnimation { duration: 320; easing.type: Easing.OutCubic } }
    }
    RectangularShadow {
        id: cardShadow
        anchors.fill: parent
        z: -1
        offset.x: 3
        offset.y: -translateTransform.y
        radius: Style.settings.cubeRadius
        blur: 10
        spread: 0
        color: Style.themes.shadowColor
        Behavior on blur { NumberAnimation { duration: 240 } }
    }
    MouseArea {
        id: cardArea
        anchors.fill: parent
        hoverEnabled: true
        onEntered: {
            cardShadow.blur = 24;
            translateTransform.y = -5;
        }
        onExited: {
            cardShadow.blur = 10;
            translateTransform.y = 0;
        }
        onClicked: root.clicked();
    }
}