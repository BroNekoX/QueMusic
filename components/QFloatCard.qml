// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Effects
import QueMusic 1.0

Rectangle {
    id: root
    width: 240
    height: 120
    radius: Style.cubeRadius
    color: Style.backmode === 0 ? Theme.primaryColor : Theme.primaryBlurColor
    border.color: Theme.sideColor
    border.width: 1
    signal clicked()
    transform: Translate {
        id: translateTransform
        y: hoverArea.hovered ? -4 : 0
        Behavior on y { NumberAnimation { duration: 320; easing.type: Easing.OutCubic } }
    }
    RectangularShadow {
        id: cardShadow
        anchors.fill: parent
        z: -1
        offset.x: 0
        offset.y: 4 - translateTransform.y
        radius: Style.cubeRadius
        blur: hoverArea.hovered ? 28 : 14
        spread: 0
        color: Theme.shadowColor
        Behavior on blur { NumberAnimation { duration: 240 } }
    }
    TapHandler {
        onTapped: root.clicked();
    }
    HoverHandler {
        id: hoverArea
    }
}
