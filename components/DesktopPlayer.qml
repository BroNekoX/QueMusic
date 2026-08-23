// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
import QtQuick
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
    // 当前桌面部件模式：0.无 1.灵动岛 2.小窗播放器 3.歌词栏
    property int desktopPlayerMode: 0
    property bool openTool: false

    background: QBlurCard {
        anchors.fill: parent
        borderRadius: Style.settings.cubeRadius
        clip: false
        blurSource: mainLayout
        shadowEffect: true
        rectXy: Qt.rect(desktopPlayer.x, desktopPlayer.y, 360, desktopPlayer.height)
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

            SettingItem {
                label: "启用桌面部件"
                controlWidth: 120
                width: parent.width
                QSwitch {
                    height: 36; width: 120
                    anchors.right: parent.right
                    switchTrue: desktopPlayer.openTool
                    onToggled: {
                        desktopSpot.active = false;
                        desktopPlayerLoader.active = false;
                        desktopLyricsLoader.active = false;
                        desktopPlayer.openTool = !desktopPlayer.openTool;
                        desktopPlayer.desktopPlayerMode = 0;
                    }
                }
            }

            SettingItem {
                label: "桌面部件"
                controlWidth: 120
                width: parent.width
                z: 5
                opacity: desktopPlayer.openTool ? 1.0 : 0.5
                QDrop {
                    height: 36; width: 120
                    anchors.right: parent.right
                    enabled: desktopPlayer.openTool
                    choice: desktopPlayer.desktopPlayerMode
                    model: ["无","灵动岛","小窗播放器","桌面歌词"]
                    onTransformed: (choiced) => {
                        desktopPlayer.desktopPlayerMode = choiced;
                        switch(choiced) {
                            case 0:
                                // 无：全部关闭
                                desktopSpot.active = false;
                                desktopPlayerLoader.active = false;
                                desktopLyricsLoader.active = false;
                                break;
                            case 1:
                                // 灵动岛：开启灵动岛，关闭小窗
                                desktopLyricsLoader.active = false;
                                desktopPlayerLoader.active = false;
                                desktopSpot.active = true;
                                break;
                            case 2:
                                // 小窗播放器：开启小窗，关闭灵动岛
                                desktopSpot.active = false;
                                desktopLyricsLoader.active = false;
                                desktopPlayerLoader.active = true;
                                if (desktopPlayerLoader.status === Loader.Ready) {
                                    desktopPlayerLoader.item.show();
                                }
                                break;
                            case 3:
                                // 歌词栏：暂未实现
                                desktopSpot.active = false;
                                desktopPlayerLoader.active = false;
                                desktopLyricsLoader.active = true;
                                break;
                        }
                    }
                }
            }
        }
    }

    // 小窗播放器异步加载完成后自动显示（避免关闭按钮 hide 后无法再次出现）
    Connections {
        target: desktopPlayerLoader
        function onStatusChanged() {
            if (desktopPlayerLoader.status === Loader.Ready && desktopPlayer.desktopPlayerMode === 2) {
                desktopPlayerLoader.item.show();
            }
        }
    }

    enter: Transition {
        NumberAnimation { property: "x"; duration: 420; from: desktopPlayer.parent.width; to: desktopPlayer.parent.width - 380; easing.type: Easing.OutExpo }
    }
    exit: Transition {
        NumberAnimation { property: "x"; duration: 210; to: desktopPlayer.parent.width; easing.type: Easing.InCubic }
    }
}