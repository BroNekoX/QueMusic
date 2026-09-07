// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors

import QtQuick
import QtQuick.Controls.Basic

Button {
    id: root
    width: 36
    height: 36
    padding: 0
    leftInset: 0
    topInset: 0
    rightInset: 0
    bottomInset: 0
    property alias source: image.source
    property bool largeicon: false
    property color hoverColor: Style.themes.hoverColor

    contentItem: Image {
        id: image
        sourceSize.width: root.largeicon ? 17 : 15
        sourceSize.height: root.largeicon ? 17 : 15
        fillMode: Image.Pad
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
    }
    background: Rectangle {
        border.width: 0
        color: root.hovered ? root.hoverColor : "transparent"
        radius: Style.settings.noControlRadius ? Style.settings.labelRadius : 8
        Behavior on color { ColorAnimation { duration: 60 } }
    }
}
