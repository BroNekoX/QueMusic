// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
pragma ComponentBehavior: Bound
import QtQuick
import QueMusic 1.0
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
    function changeDesktopPlayerMode(index: int): void {
        Desktop.setMode(index)
    }

    background: QBlurCard {
        anchors.fill: parent
        borderRadius: Style.cubeRadius
        clip: false
        blurSource: Options.mainLayout
        shadowEffect: true
        rectXy: Qt.rect(desktopPlayer.x, desktopPlayer.y, 360, desktopPlayer.height)
    }
    contentItem: Item {
        anchors.fill: parent
        Text {
            y: 12
            x: 18
            height: 36
            text: "桌面播放器"
            font.bold: true
            font.pixelSize: Style.textH2
            verticalAlignment: Text.AlignVCenter
            color: Theme.fontColor
        }
        SButton {
            iconCharacter: "\uf025"
            x: parent.width - 50
            y: 12
            width: 36
            height: 36
            radius: 36
            iconSize: Style.texticon + 2
            buttonColor: "transparent"
            shadowEnabled: false
            onClicked: {
                desktopPlayer.close()
            }
        }
        Column {
            id: desktopSet
            x: 12
            y: 60
            z: 2
            width: parent.width - 24
            height: parent.height - 60
            spacing: 16

            Repeater {
                model: ["无部件","小窗播放器","桌面歌词"]
                delegate: Rectangle {
                    id: widgetButton
                    required property string modelData
                    required property int index
                    width: parent.width
                    radius: Style.labelRadius
                    color: Desktop.playerMode === index ? Theme.themeColor : Theme.primaryColor
                    height: 56
                    border.width: 2
                    border.color: Theme.sideColor
                    Rectangle {
                        radius: parent.radius
                        anchors.fill: parent
                        color: Theme.hoverColor
                        opacity: modeArea.containsMouse ? 1 : 0
                        Behavior on opacity { NumberAnimation { duration: 120 } }
                    }

                    Text {
                        x: 16
                        anchors.verticalCenter: parent.verticalCenter
                        font.pixelSize: 16
                        text: widgetButton.modelData
                        color: Desktop.playerMode === widgetButton.index ? Theme.secondaryColor : Theme.fontColor
                    }
                    Text {
                        x: parent.width - 42
                        anchors.verticalCenter: parent.verticalCenter
                        text: "\uf099"
                        font.pixelSize: Style.texticon
                        font.family: Fonts.icon
                        visible: Desktop.playerMode === widgetButton.index
                        color: Theme.secondaryColor
                    }
                    MouseArea {
                        id: modeArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            desktopPlayer.changeDesktopPlayerMode(widgetButton.index);
                        }
                    }
                }
            }

            SettingItem {
                label: "Windows SMTC 播放控制器"
                controlWidth: 120
                width: parent.width
                QSwitch {
                    text: switchTrue ? "工作中" : "不支持"
                    height: 36; width: 120
                    anchors.right: parent.right
                    switchTrue: App.smtcAvailable
                    onToggled: {
                    }
                }
            }
        }
    }

    enter: Transition {
        NumberAnimation { property: "x"; duration: 450; from: desktopPlayer.parent.width; to: desktopPlayer.parent.width - 380; easing.type: Easing.OutExpo }
    }
    exit: Transition {
        NumberAnimation { property: "x"; duration: 240; to: desktopPlayer.parent.width; easing.type: Easing.OutCubic }
    }
}
