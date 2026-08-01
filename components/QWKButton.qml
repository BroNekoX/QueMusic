// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024-2026 QueMusic Contributors
//
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
    Component.onCompleted: {
        if(largeicon) {
            image.sourceSize.width = 17
            image.sourceSize.height = 17
            image.width = 17
            image.height = 17
        }
    }

    contentItem: Item {
        Image {
            id: image
            anchors.centerIn: parent
            //mipmap: true
            width: 15
            height: 15
            sourceSize.width: 15
            sourceSize.height: 15
            fillMode: Image.PreserveAspectFit
        }
    }
    background: Rectangle {
        border.width: 0
        color: root.hovered ? root.hoverColor : "transparent"
        radius: Style.settings.noControlRadius ? Style.settings.labelRadius : 8
        Behavior on color { ColorAnimation { duration: 60 } }
    }
}