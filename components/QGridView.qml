// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
import QtQuick
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
    clip: true
    signal topScroll()
    function scrollTop(): void {
        listViewAnime.stop();
        scrollToY = view.originY - view.topMargin;
        contentY = view.originY - view.topMargin;
    }

    QScrollBar.vertical: QScrollBar {
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
        readonly property real wheelHeightCount: Theme.wheelStep
        readonly property int scrollBottom: view.contentHeight - view.height + view.bottomMargin + view.originY
        onWheel: (event) => {
            listViewAnime.running = false;
            view.scrollToY = Math.max(view.originY - view.topMargin, Math.min( view.scrollToY - (event.angleDelta.y * wheelHeightCount), scrollBottom));
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
