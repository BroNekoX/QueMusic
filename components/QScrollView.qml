// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
import QtQuick
import QueMusic 1.0
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
        //parent: view
        anchors.right: view.right
        anchors.rightMargin: 10
        anchors.top: view.top
        anchors.bottom: view.bottom
        onPressedChanged: {
            view.scrollToY = view.contentY;
        }
    }
    rebound: Transition {
        NumberAnimation {
            properties: "y"
            duration: 420
            easing.type: Easing.Bezier
            easing.bezierCurve: [ 0.16, 0.03, 0.00, 1.00, 1, 1 ]
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