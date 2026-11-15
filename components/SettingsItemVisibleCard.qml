// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QueMusic 1.0

SettingItemCard {
    id: cardRoot
    property bool display: false
    property real transY: 0
    height: 0
    visible: false
    transform: Translate { y: cardRoot.transY }
    Component.onCompleted: {
        if(display) {
            height = 64;
            visible = true;
        }
    }
    onDisplayChanged: {
        if(display) {
            outAnime.running = false;
            visible = true;
            onAnime.running = true;
        } else {
            onAnime.running = false;
            outAnime.running = true;
        }
    }

    ParallelAnimation {
        id: onAnime
        NumberAnimation {
            target: cardRoot
            property: "height"
            from: 0
            to: 64
            duration: 360
            easing.type: Easing.OutExpo
        }
        NumberAnimation {
            target: cardRoot
            property: "opacity"
            from: 0
            to: 1
            duration: 320
            easing.type: Easing.OutCubic
        }
        NumberAnimation {
            target: cardRoot
            property: "transY"
            from: -64
            to: 0
            duration: 360
            easing.type: Easing.OutExpo
        }
    }
    ParallelAnimation {
        id: outAnime
        NumberAnimation {
            target: cardRoot
            property: "height"
            to: 0
            duration: 180
            easing.type: Easing.OutCubic
        }
        NumberAnimation {
            target: cardRoot
            property: "opacity"
            to: 0
            duration: 120
            easing.type: Easing.OutCubic
        }
        NumberAnimation {
            target: cardRoot
            property: "transY"
            to: -64
            duration: 180
            easing.type: Easing.OutCubic
        }
        onFinished: cardRoot.visible = false;
    }
}
