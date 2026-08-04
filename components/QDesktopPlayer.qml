// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024-2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic

Popup {
    id: desktopPlayer
    padding: 0
    margins: -1
    parent: Overlay.overlay
    width: 360
    height: parent.height - 180
    x: parent.width - 380
    y: 80
    background: QBlurCard {
        anchors.fill: parent
        borderRadius: Style.settings.cubeRadius
        clip: false
        blurSource: mainLayout
        shadowEffect: true
        rectXy: Qt.rect(desktopPlayer.x, desktopPlayer.y, 320, desktopPlayer.height)
        //color: Style.themes.primaryBlurColor
    }
    contentItem: Item {
        anchors.fill: parent
        Label {
            y: 10
            x: 18
            height: 40
            text: "桌面播放器"
            font.bold: true
            font.pixelSize: Style.settings.textH2
            verticalAlignment: Text.AlignVCenter
            color: Style.themes.fontColor
        }
        SButton {
            iconCharacter: "\uf11b"
            x: parent.width - 50
            y: 10
            width: 40
            height: 40
            radius: 40
            buttonColor: "transparent"
            hoverColor: Qt.rgba(1.0,0.5,0.5,0.5)
            shadowEnabled: false
            onClicked: {
            }
        }
        Row {
            id: desktopSet
            x: 12
            y: 60
            z: 2
            width: parent.width - 24
            height: parent.height - 60
            spacing: 16
            clip: true

            SettingItem {
                label: "桌面歌词模式"
                controlWidth: 120
                width: parent.width
                z: 5
                Behavior on height { NumberAnimation { duration: 320; easing.type: Easing.OutExpo } }
                QDrop {
                    property int desktopPlayerMode: 0
                    height: 36; width: 120
                    anchors.right: parent.right
                    choice: desktopPlayerMode
                    model: ["无","灵动岛","小窗播放器","歌词栏"]
                    onTransformed: (choiced) => {
                        switch(choiced) {
                            case 0:
                                desktopPlayerMode = 0
                                desktopSpot.active = false
                                break;
                            case 1:
                                desktopPlayerMode = 1
                                desktopSpot.active = true
                                break;
                            case 2:
                                desktopPlayerMode = 2
                                desktopSpot.active = false
                                break;
                            case 3:
                                desktopPlayerMode = 3
                                desktopSpot.active = false
                                break;
                        }
                    }
                }
            }
        }
    }
    enter: Transition {
        NumberAnimation { property: "x"; duration: 420; from: playList.parent.width; to: playList.parent.width - 380; easing.type: Easing.OutExpo }
    }
    exit: Transition {
        NumberAnimation { property: "x"; duration: 210; to: playList.parent.width; easing.type: Easing.InCubic }
    }
}