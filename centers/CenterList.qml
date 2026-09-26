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
        height: 46
        required property int index
        readonly property var info: ({
            name: model.name, title: model.title, singer: model.singer, artist: model.artist,
            path: model.path, fileUrl: model.fileUrl, folderId: model.folderId,
            source: model.source, duration: model.duration
        })

        Rectangle {
            anchors.fill: parent
            radius: 12
            color: area.containsMouse ? "#14ffffff" : "#08ffffff"
            Behavior on color { ColorAnimation { duration: 140 } }
            border.width: area.containsMouse ? 1 : 0
            border.color: "#22ffffff"
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
                font.weight: Font.DemiBold
                color: "#eef2f8"
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }
            Text {
                width: parent.width
                height: 18
                text: model.singer || model.artist || model.path || ""
                font.pixelSize: 11
                color: "#8d99aa"
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }
        }
        MouseArea {
            id: area
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.picked(row.index, row.info)
        }
    }
}
