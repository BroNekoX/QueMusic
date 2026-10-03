// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
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
    // 当前桌面部件模式：0.无 1.灵动岛 2.小窗播放器 3.歌词栏
    property int desktopPlayerMode: 0
    function changeDesktopPlayerMode(index: int): void {
        desktopPlayer.desktopPlayerMode = index;
        switch(index) {
            case 0:
                // 无：全部关闭
                desktopPlayerLoader.active = false;
                desktopLyricsLoader.active = false;
                break;
            case 1:
                // 小窗播放器：开启小窗，关闭灵动岛
                desktopLyricsLoader.active = false;
                desktopPlayerLoader.active = true;
                if (desktopPlayerLoader.status === Loader.Ready) {
                    desktopPlayerLoader.item.show();
                }
                break;
            case 2:
                // 歌词栏：暂未实现
                desktopPlayerLoader.active = false;
                desktopLyricsLoader.active = true;
                break;
        }
    }

    background: QBlurCard {
        anchors.fill: parent
        borderRadius: Style.settings.cubeRadius
        clip: false
        blurSource: mainLayout
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
            font.pixelSize: Style.settings.textH2
            verticalAlignment: Text.AlignVCenter
            color: Style.fontColor
        }
        SButton {
            iconCharacter: "\uf025"
            x: parent.width - 50
            y: 12
            width: 36
            height: 36
            radius: 36
            iconSize: Style.settings.texticon + 2
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
                    required property string modelData
                    required property int index
                    width: parent.width
                    radius: Style.settings.labelRadius
                    color: desktopPlayer.desktopPlayerMode === index ? Style.themeColor : Style.primaryColor
                    height: 60
                    border.width: 2
                    border.color: Style.sideColor
                    Rectangle {
                        radius: parent.radius
                        anchors.fill: parent
                        color: Style.hoverColor
                        opacity: modeArea.containsMouse ? 1 : 0
                        Behavior on opacity { NumberAnimation { duration: 120 } }
                    }

                    Text {
                        x: 16
                        anchors.verticalCenter: parent.verticalCenter
                        font.pixelSize: Style.settings.textH2
                        text: modelData
                        color: desktopPlayer.desktopPlayerMode === index ? Style.secondaryColor : Style.fontColor
                    }
                    Text {
                        x: parent.width - 42
                        anchors.verticalCenter: parent.verticalCenter
                        text: "\uf099"
                        font.pixelSize: Style.settings.texticon
                        font.family: IconFont.name
                        visible: desktopPlayer.desktopPlayerMode === index
                        color: Style.secondaryColor
                    }
                    MouseArea {
                        id: modeArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            desktopPlayer.changeDesktopPlayerMode(index);
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
                    switchTrue: Options.smtc.mgr.available
                    onToggled: {
                    }
                }
            }
        }
    }

    // 小窗播放器异步加载完成后自动显示（避免关闭按钮 hide 后无法再次出现）
    Connections {
        target: desktopPlayerLoader
        function onStatusChanged(): void {
            if (desktopPlayerLoader.status === Loader.Ready && desktopPlayer.desktopPlayerMode === 1) {
                desktopPlayerLoader.item.show();
            }
        }
    }

    enter: Transition {
        NumberAnimation { property: "x"; duration: 450; from: window.width; to: window.width - 380; easing.type: Easing.OutExpo }
    }
    exit: Transition {
        NumberAnimation { property: "x"; duration: 240; to: window.width; easing.type: Easing.OutCubic }
    }
}
