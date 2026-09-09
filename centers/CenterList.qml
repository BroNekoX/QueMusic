// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
// 轻量列表：用于没有 cover/title/artist 角色的模型（本地歌曲 / 文件夹）
// 具备标准角色的模型请直接用项目 QListView
import QtQuick

ListView {
    id: root
    clip: true
    spacing: 4
    signal picked(int index, var data)

    delegate: Item {
        id: row
        width: root.width
        height: 44
        required property int index
        readonly property var info: ({
            name: model.name, title: model.title, singer: model.singer, artist: model.artist,
            path: model.path, fileUrl: model.fileUrl, folderId: model.folderId,
            source: model.source, duration: model.duration
        })

        Rectangle {
            anchors.fill: parent
            radius: 10
            color: "#ffffff"
            opacity: area.containsMouse ? 0.10 : 0
            Behavior on opacity { NumberAnimation { duration: 120 } }
        }
        Column {
            x: 14
            width: row.width - 28
            height: row.height
            Text {
                width: parent.width
                height: 24
                text: model.name || model.title || ""
                font.pixelSize: 13
                font.bold: true
                color: "#f2f5fa"
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }
            Text {
                width: parent.width
                height: 18
                text: model.singer || model.artist || model.path || ""
                font.pixelSize: 11
                color: "#9fb2c2"
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }
        }
        MouseArea {
            id: area
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.picked(index, row.info)
        }
    }
}
