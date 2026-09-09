// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
// 列表详情：歌单 / 榜单 / 歌手歌曲共用（读取 MusicApi.playlistSong）
import QtQuick
import QueMusic 1.0
import 'qrc:/QueMusic/components'

Rectangle {
    id: root
    radius: 24
    color: "#a30b0c12"
    border.width: 1
    border.color: "#1cffffff"

    property bool opened: false
    property string title: ""
    property url cover: "qrc:/QueMusic/resources/app/musicpic.png"

    signal closeClicked
    signal picked(int index, var data)
    signal queued(int index, var data)
    signal faved(int index, var data)
    signal downloaded(int index, var data)

    x: root.opened ? 0 : parent.width + 16
    visible: x < parent.width
    Behavior on x { NumberAnimation { duration: 380; easing.type: Easing.OutExpo } }

    Column {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 12
        Row {
            width: parent.width
            height: 68
            spacing: 14
            QPicture {
                width: 68
                height: 68
                radius: 14
                source: root.cover
                sourceSize: Qt.size(160, 160)
            }
            Column {
                width: parent.width - 82
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8
                Text {
                    width: parent.width
                    text: root.title || "歌单"
                    font.pixelSize: 18
                    font.weight: Font.DemiBold
                    color: "#f5f7fb"
                    elide: Text.ElideRight
                }
                Row {
                    spacing: 8
                    QButton {
                        text: "返回"
                        iconCharacter: "\uf0dc"
                        height: 32
                        fontSize: 12
                        onClicked: root.closeClicked()
                    }
                    QButton {
                        text: "全部加入队列"
                        iconCharacter: "\uf098"
                        height: 32
                        fontSize: 12
                        onClicked: addAll()
                    }
                }
            }
        }

        QListView {
            width: parent.width
            height: parent.height - 80
            model: MusicApi.playlistSong
            isEnd: MusicApi.playlistSong.count > 0
            onClicked: i => root.picked(i, MusicApi.playlistSong.get(i))
            onToolClicked: (i, tool) => {
                var d = MusicApi.playlistSong.get(i)
                if (tool === 0) root.queued(i, d)
                else if (tool === 1) root.faved(i, d)
            }
            onMenuClicked: (i, choice) => {
                if (choice === 0) root.downloaded(i, MusicApi.playlistSong.get(i))
            }
        }
    }

    function addAll() {
        var m = MusicApi.playlistSong
        var n = 0
        for (var i = 0; i < m.count; i++) {
            var d = m.get(i)
            if (d && d.hash && Playback.indexOfPath(d.hash) === -1) {
                Playback.queue.append({ name: d.title, path: d.hash, songer: d.artist, source: MusicApi.songSource })
                n++
            }
        }
        mainWarn.tiped("已加入 " + n + " 首", 1)
    }
}
