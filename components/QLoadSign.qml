// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QueMusic 1.0

Item {
    id: root
    width: 36
    height: 36
    property bool loader: false
    visible: false
    function loadAction() {
        root.visible = true;
        loadAnime.running = true;
    }
    function finish() {
        loadAnime.running = false;
        finishAnime.running = true;
    }
    Connections {
        target: MusicApi
        function onFinished() {
            root.finish()
        }
        function onLoaded() {
            root.loadAction()
        }
    }

    ParallelAnimation {
        id: loadAnime
        NumberAnimation {
            target: loadImage
            property: "scale"
            from: 0.5
            to: 1.0
            duration: 240
            easing.type: Easing.OutExpo
        }
        NumberAnimation {
            target: loadImage
            property: "opacity"
            from: 0.0
            to: 1.0
            duration: 240
            easing.type: Easing.OutExpo
        }
    }

    SequentialAnimation {
        id: finishAnime
        ParallelAnimation {
            NumberAnimation {
                target: loadImage
                property: "scale"
                to: 0.5
                duration: 240
            }
            NumberAnimation {
                target: loadImage
                property: "opacity"
                to: 0.0
                duration: 240
            }
        }
        ScriptAction {
            script: root.visible = false
        }
    }
    AnimatedImage {
        id: loadImage
        x: 6
        y: 6
        width: root.width - 12
        height: root.height - 12
        source: "qrc:/QueMusic/resources/loader.gif"
        playing: true
        paused: false
    }
}