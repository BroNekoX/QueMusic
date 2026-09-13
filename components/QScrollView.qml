// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Controls.Basic

ScrollView {
    id: view
    contentWidth: availableWidth
    wheelEnabled: false
    property real scrollToPosition: 0
    property int barMargin: 18
    onContentHeightChanged: {
        scrollToPosition = viewBar.position;
    }

    ScrollBar.vertical: ScrollBar {
        id: viewBar
        parent: view
        x: view.width - view.barMargin
        y: view.topPadding
        height: view.availableHeight
        onPressedChanged: {
            view.scrollToPosition = position;
        }
    }

    NumberAnimation {
        id: viewAnime
        target: viewBar
        property: "position"
        duration: 240
        to: view.scrollToPosition
        easing.type: Easing.OutCubic
        onFinished: viewBar.active = false
    }

    WheelHandler {
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        readonly property real wheelHeightCount: Qt.application.styleHints.wheelScrollLines / view.contentHeight * 0.25
        onWheel: (event) => {
            viewAnime.running = false;
            viewBar.active = true;
            view.scrollToPosition = Math.max(0, Math.min(view.scrollToPosition - event.angleDelta.y * wheelHeightCount, 1 - viewBar.size));
            viewAnime.running = true;
        }
    }
}