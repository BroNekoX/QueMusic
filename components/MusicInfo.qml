// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Controls.Basic

Popup {
    id: root
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
        rectXy: Qt.rect(root.x, root.y, 360, root.height)
        //color: Style.themes.primaryBlurColor
    }
    contentItem: Item {
        anchors.fill: parent
        Label {
            y: 10
            x: 18
            height: 40
            text: "音乐详情"
            font.bold: true
            font.pixelSize: Style.settings.textH2
            verticalAlignment: Text.AlignVCenter
            color: Style.themes.fontColor
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
                root.close()
            }
        }

        ScrollView {
            x: 16
            y: 60
            width: root.width - 32
            height: root.height - 60
            contentChildren: Column {
                id: desktopSet
                //z: 2
                width: root.width - 32
                spacing: 16
                Item {
                    width: parent.width
                    height: 114
                    QPicture {
                        source: mainMedia.urlStr
                        radius: 12
                        width: 112
                        height: 112
                        MouseArea {
                            anchors.fill: parent
                            onClicked: picWatch.dialog(mainMedia.urlStr || "qrc:/QueMusic/resources/app/musicpic.png",window.musicTitle);
                        }
                    }
                    Text {
                        x: 128
                        y: 8
                        text: window.musicTitle
                        font.pixelSize: 18
                        font.bold: true
                        width: parent.width - 128
                        elide: Text.ElideRight
                        color: Style.themes.fontColor
                    }
                    Text {
                        x: 128
                        y: 42
                        text: window.musicArtist
                        font.pixelSize: 16
                        width: parent.width - 128
                        elide: Text.ElideRight
                        color: Style.themes.textColor
                    }
                }
                SettingItem {
                    label: "文件名："
                    controlWidth: 120
                    width: parent.width
                    TextInput {
                        height: 36
                        anchors.right: parent.right
                        font.pixelSize: Style.settings.textmain
                        text: mainMedia.noTitle
                        color: Style.themes.textColor
                        verticalAlignment: Text.AlignVCenter
                        readOnly: true
                        selectByMouse: true
                        selectionColor: Style.themes.themeColor
                    }
                }
                SettingItem {
                    label: "歌曲名："
                    controlWidth: 120
                    width: parent.width
                    TextInput {
                        height: 36
                        anchors.right: parent.right
                        font.pixelSize: Style.settings.textmain
                        text: window.musicTitle
                        color: Style.themes.textColor
                        readOnly: true
                        selectByMouse: true
                        selectionColor: Style.themes.themeColor
                        verticalAlignment: Text.AlignVCenter
                    }
                }
                SettingItem {
                    label: "艺术家："
                    controlWidth: 120
                    width: parent.width
                    TextInput {
                        height: 36
                        anchors.right: parent.right
                        font.pixelSize: Style.settings.textmain
                        text: window.musicArtist
                        color: Style.themes.textColor
                        readOnly: true
                        selectByMouse: true
                        selectionColor: Style.themes.themeColor
                        verticalAlignment: Text.AlignVCenter
                    }
                }
                SettingItem {
                    label: "专辑："
                    controlWidth: 120
                    width: parent.width
                    TextInput {
                        height: 36
                        anchors.right: parent.right
                        font.pixelSize: Style.settings.textmain
                        text: mainMedia.album
                        color: Style.themes.textColor
                        readOnly: true
                        selectByMouse: true
                        selectionColor: Style.themes.themeColor
                        verticalAlignment: Text.AlignVCenter
                    }
                }
                SettingItem {
                    label: "音频长度："
                    controlWidth: 120
                    width: parent.width
                    TextInput {
                        height: 36
                        anchors.right: parent.right
                        font.pixelSize: Style.settings.textmain
                        text: mainMedia.duration.toString()
                        color: Style.themes.textColor
                        readOnly: true
                        selectByMouse: true
                        selectionColor: Style.themes.themeColor
                        verticalAlignment: Text.AlignVCenter
                    }
                }
                SettingItem {
                    label: "日期："
                    controlWidth: 120
                    width: parent.width
                    TextInput {
                        height: 36
                        anchors.right: parent.right
                        font.pixelSize: Style.settings.textmain
                        text: mainMedia.date
                        color: Style.themes.textColor
                        readOnly: true
                        selectByMouse: true
                        selectionColor: Style.themes.themeColor
                        verticalAlignment: Text.AlignVCenter
                    }
                }
                SettingItem {
                    label: "音频格式："
                    controlWidth: 120
                    width: parent.width
                    TextInput {
                        height: 36
                        anchors.right: parent.right
                        font.pixelSize: Style.settings.textmain
                        text: mainMedia.type
                        color: Style.themes.textColor
                        readOnly: true
                        selectByMouse: true
                        selectionColor: Style.themes.themeColor
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }
    }


    enter: Transition {
        NumberAnimation { property: "x"; duration: 420; from: root.parent.width; to: root.parent.width - 380; easing.type: Easing.OutExpo }
    }
    exit: Transition {
        NumberAnimation { property: "x"; duration: 210; to: root.parent.width; easing.type: Easing.InCubic }
    }
}