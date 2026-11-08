// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
pragma ComponentBehavior: Bound
import QtQuick
import QueMusic 1.0
import QtQuick.Controls.Basic

// 桌面部件面板：桌面歌词 / 小窗播放器开关（窗口由 C++ 原生层承载，开关写入 Config 即时生效）
Popup {
    id: desktopWidgets
    padding: 0
    margins: -1
    parent: Overlay.overlay
    width: 360
    height: 180
    x: parent.width - 380
    y: parent.height - height - 88

    background: QBlurCard {
        anchors.fill: parent
        borderRadius: Style.cubeRadius
        clip: false
        blurSource: Options.mainLayout
        shadowEffect: true
        rectXy: Qt.rect(desktopWidgets.x, desktopWidgets.y, 360, desktopWidgets.height)
    }
    contentItem: Item {
        anchors.fill: parent
        Text {
            y: 12
            x: 18
            height: 36
            text: "桌面部件"
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
                desktopWidgets.close()
            }
        }
        Column {
            id: widgetSet
            x: 12
            y: 60
            z: 2
            width: parent.width - 24
            height: parent.height - 60
            spacing: 16

            SettingItem {
                label: "桌面歌词"
                controlWidth: 120
                width: parent.width
                QSwitch {
                    height: 36; width: 120
                    anchors.right: parent.right
                    switchTrue: Config.desktopLyricsOn
                    onToggled: Config.desktopLyricsOn = !Config.desktopLyricsOn
                }
            }
            SettingItem {
                label: "小窗播放器"
                controlWidth: 120
                width: parent.width
                QSwitch {
                    height: 36; width: 120
                    anchors.right: parent.right
                    switchTrue: Config.desktopMiniPlayerOn
                    onToggled: Config.desktopMiniPlayerOn = !Config.desktopMiniPlayerOn
                }
            }
        }
    }

    enter: Transition {
        NumberAnimation { property: "x"; duration: 450; from: desktopWidgets.parent.width; to: desktopWidgets.parent.width - 380; easing.type: Easing.OutExpo }
    }
    exit: Transition {
        NumberAnimation { property: "x"; duration: 240; to: desktopWidgets.parent.width; easing.type: Easing.OutCubic }
    }
}
