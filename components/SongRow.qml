// SongRow.qml — 紧凑歌曲行（用于主页卡片列表）
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024-2026 QueMusic Contributors
//
import QtQuick

Item {
    id: root
    height: 40
    width: 200

    property string title: ""
    property string artist: ""
    property url cover: "qrc:/QueMusic/resources/app/musicpic.png"
    property bool highlighted: false
    signal clicked

    // 悬停背景
    Rectangle {
        anchors.fill: parent
        radius: 8
        color: Style.themes.hoverColor
        opacity: rowArea.containsMouse ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 80 } }
    }

    // 封面
    QPicture {
        x: 0
        y: 3
        width: 34
        height: 34
        radius: 8
        source: root.cover
    }

    // 标题
    Text {
        x: 42
        y: 3
        width: root.width - 42 - (root.highlighted ? 48 : 40)
        height: 19
        text: root.title
        color: root.highlighted ? Style.themes.themeColor : Style.themes.fontColor
        font.bold: true
        font.pixelSize: Style.settings.textmain
        elide: Text.ElideRight
        verticalAlignment: Text.AlignVCenter
        Behavior on color { ColorAnimation { duration: 120 } }
    }

    // 歌手
    Text {
        x: 42
        y: 21
        width: root.width - 42 - 8
        height: 16
        text: root.artist
        color: Style.themes.textColor
        font.pixelSize: Style.settings.textTip
        elide: Text.ElideRight
        verticalAlignment: Text.AlignVCenter
    }

    MouseArea {
        id: rowArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.clicked()

        // 当前播放标记
        Text {
            x: parent.width - 44
            y: 0
            width: 36
            height: 40
            visible: root.highlighted
            text: "\uf00e"
            font.family: iconFont.name
            font.pixelSize: Style.settings.texticon
            color: Style.themes.themeColor
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        // 悬停播放按钮
        SButton {
            id: playBtn
            x: parent.width - 40
            y: 4
            width: 32
            height: 32
            radius: 16
            iconCharacter: "\uf00e"
            iconSize: Style.settings.texticon
            iconColor: Style.themes.themeColor
            buttonColor: "transparent"
            hoverColor: Style.themes.hoverColor
            shadowEnabled: false
            visible: !root.highlighted && (rowArea.containsMouse || playBtn.hovered)
            onClicked: root.clicked()
        }
    }
}
