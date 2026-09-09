// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
// 本地音乐页（对应 pages/FilePage.qml）
// 本地模型只有 name/singer/path，没有 cover/title，故用轻量 CenterList
import QtQuick
import QueMusic 1.0

Item {
    id: page
    anchors.fill: parent

    Component.onCompleted: {
        myFolderModel.loadFromDatabase()
        localFolderModel.loadFromDatabase()
    }

    Row {
        anchors.fill: parent
        spacing: 18
        Column {
            width: 240
            height: parent.height
            spacing: 8
            Text {
                width: parent.width
                height: 26
                text: "我的文件夹"
                font.pixelSize: 15
                font.weight: Font.DemiBold
                color: "#f5f7fb"
                verticalAlignment: Text.AlignVCenter
            }
            CenterList {
                width: parent.width
                height: 190
                model: myFolderModel
                onPicked: (i, d) => page.loadFolder(d.folderId)
            }
            Text {
                width: parent.width
                height: 26
                text: "本地目录"
                font.pixelSize: 15
                font.weight: Font.DemiBold
                color: "#f5f7fb"
                verticalAlignment: Text.AlignVCenter
            }
            CenterList {
                width: parent.width
                height: parent.height - 276
                model: localFolderModel
                onPicked: (i, d) => page.loadFolder(d.folderId)
            }
        }
        Column {
            width: parent.width - 258
            height: parent.height
            spacing: 8
            Text {
                width: parent.width
                height: 26
                text: "歌曲 " + songModel.rowCount()
                font.pixelSize: 15
                font.weight: Font.DemiBold
                color: "#f5f7fb"
                verticalAlignment: Text.AlignVCenter
            }
            CenterList {
                width: parent.width
                height: parent.height - 34
                model: songModel
                onPicked: (i, d) => center.playLocal(d.path, d.name)
            }
        }
    }

    function loadFolder(id) {
        songModel.folderId = id
        songModel.loadByFolder(id)
    }
}
