// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
pragma ComponentBehavior: Bound
import QtQuick
import QueMusic 1.0

Rectangle {
    id: root
    color: Theme.secondaryColor
    Component.onCompleted: {
        quemainAnime.start();
    }

    SequentialAnimation {
        id: quemainAnime
        PauseAnimation { duration: 1000 }
        ScriptAction { script: appTextRow.allText = ["Q","u","e","M","u","s","i","c"," ","P","r","o","j","e","c","t"] }
        NumberAnimation {
            target: appIcon
            property: "x"
            from: root.width / 2 - 32
            to: root.width / 2 - appText.width / 2 - 40
            duration: 480
            easing.type: Easing.InOutQuart
        }
        ScriptAction { script: appIcon.x = Qt.binding(function() { return root.width / 2 - appText.width / 2 - 40 }) }
        NumberAnimation {
            target: controlBar
            property: "y"
            from: root.height
            to: root.height - 78
            duration: 480
            easing.type: Easing.OutExpo
        }
    }

    Image {
        id: appIcon
        x: root.width / 2 - 32
        y: root.height / 2 - 32
        width: 64
        height: 64
        source: "qrc:/QueMusic/resources/icon.png"
        sourceSize: Qt.size(64, 64)
    }
    Text {
        id: appText
        height: 64
        text: "QueMusic Project"
        font.family: Fonts.text
        font.pixelSize: 36
        font.bold: true
        verticalAlignment: Text.AlignVCenter
        color: Theme.fontColor
        visible: false
    }
    Row {
        id: appTextRow
        y: root.height / 2 - 32
        x: root.width / 2 - width / 2 + 40
        height: 64
        property list<string> allText
        Repeater {
            model: appTextRow.allText
            delegate: Text {
                id: textChar
                required property string modelData
                required property int index
                text: modelData
                height: 64
                font.family: Fonts.text
                font.pixelSize: 36
                font.bold: true
                color: Theme.fontColor
                verticalAlignment: Text.AlignVCenter
                opacity: 0
                SequentialAnimation {
                    Component.onCompleted: running = true;
                    PauseAnimation {
                        duration: textChar.index * 24 + 240
                    }
                    ParallelAnimation {
                        NumberAnimation {
                            target: textChar
                            properties: "y"
                            from: 64
                            to: 0
                            duration: 640
                            easing.type: Easing.OutExpo
                        }
                        NumberAnimation {
                            target: textChar
                            properties: "opacity"
                            from: 0
                            to: 1
                            duration: 420
                            easing.type: Easing.OutCubic
                        }
                    }
                }
            }
        }
    }
    Rectangle {
        id: controlBar
        y: root.height
        height: 78
        width: root.width
        color: Theme.primaryColor
        QButton {
            x: parent.width - 124
            y: 21
            height: 36
            width: 100
            text: "开始使用"
            radius: 12
            buttonColor: Theme.themeColor
            textColor: Theme.primaryColor
            iconColor: Theme.primaryColor
            shadowEnabled: false
            onClicked: {
                settingsView.readyStart();
            }
        }
    }
}