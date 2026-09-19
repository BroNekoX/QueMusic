// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Controls.Basic
import QueMusic 1.0

GridView {
    id: view
    contentWidth: view.width - 16
    synchronousDrag: true
    reuseItems: true
    onDraggingChanged: view.scrollToY = view.contentY
    topMargin: 6
    bottomMargin: 24
    rightMargin: 16
    property int scrollToY: view.contentY
    property bool enabled: true
    clip: true
    signal topScroll()
    function scrollTop(): void {
        listViewAnime.stop();
        scrollToY = - view.topMargin;
        contentY = - view.topMargin;
    }

    ScrollBar.vertical: ScrollBar {
        id: viewBar
        parent: view
        anchors.top: view.top
        anchors.right: view.right
        anchors.bottom: view.bottom
        onPressedChanged: {
            view.scrollToY = view.contentY
        }
    }
    WheelHandler {
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        enabled: view.enabled
        readonly property real wheelHeightCount: Qt.application.styleHints.wheelScrollLines * 0.25
        readonly property int scrollBottom: view.contentHeight - view.height + view.bottomMargin
        onWheel: (event) => {
            listViewAnime.running = false;
            view.scrollToY = Math.max(- view.topMargin, Math.min( view.scrollToY - (event.angleDelta.y * wheelHeightCount), scrollBottom));
            viewBar.active = true;
            event.accepted = true;
            listViewAnime.running = true;
            if(event.angleDelta.y > 0 && view.scrollToY === - view.topMargin) {
                view.topScroll()
            }
        }
    }
    NumberAnimation {
        id: listViewAnime
        target: view
        property: "contentY"
        duration: 240
        to: view.scrollToY
        easing.type: Easing.OutCubic
        onFinished: viewBar.active = false
    }
}
