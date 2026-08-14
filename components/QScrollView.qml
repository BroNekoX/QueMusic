// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Controls.Basic

ScrollView {
    id: view
    //contentHeight: uiContent.height + 50
    contentWidth: availableWidth
    wheelEnabled: false
    property real scrollToPosition: 0.0
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
        //active: view.ScrollBar.horizontal.active
        //Behavior on position { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }
        onPressedChanged: {
            view.scrollToPosition = position;
        }
    }

    SequentialAnimation {
        id: viewAnime
        NumberAnimation {
            target: viewBar
            property: "position"
            duration: 240
            to: view.scrollToPosition
            easing.type: Easing.OutCubic
        }
        ScriptAction {
            script: viewBar.active = false
        }
    }

    WheelHandler {
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        onWheel: (event) => {
                     var wheelheight = event.angleDelta.y * 0.25 * Qt.application.styleHints.wheelScrollLines / view.contentHeight;
                     viewBar.active = true;
                     view.scrollToPosition = Math.max(0, Math.min(view.scrollToPosition - wheelheight, 1 - viewBar.size));
                     //viewBar.position = scrollToPosition
                     viewAnime.stop();
                     viewAnime.start();
                 }
    }
}