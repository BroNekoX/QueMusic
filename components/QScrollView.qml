// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Controls.Basic

Flickable {
    id: view
    contentWidth: width
    synchronousDrag: true
    acceptedButtons: Qt.NoButton
    property int scrollToY: view.contentY
    onDraggingChanged: view.scrollToY = view.contentY
    contentHeight: contentItem.childrenRect.height

    clip: true

    ScrollBar.vertical: ScrollBar {
        id: viewBar
        anchors.right: view.right
        anchors.rightMargin: 10
        anchors.top: view.top
        anchors.bottom: view.bottom
        onPressedChanged: {
            view.scrollToY = view.contentY;
        }
    }

    WheelHandler {
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        readonly property real wheelHeightCount: Qt.application.styleHints.wheelScrollLines * 0.25
        onWheel: (event) => {
            viewAnime.running = false;
            view.scrollToY = Math.max(0, Math.min( view.scrollToY - (event.angleDelta.y * wheelHeightCount), view.contentHeight - view.height));
            viewBar.active = true;
            event.accepted = true;
            viewAnime.running = true;
        }
    }

    NumberAnimation {
        id: viewAnime
        target: view
        property: "contentY"
        duration: 240
        to: view.scrollToY
        easing.type: Easing.OutCubic
        onFinished: viewBar.active = false
    }
}