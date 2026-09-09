// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
// 推荐页（对应 pages/HomePage.qml）
import QtQuick
import QueMusic 1.0
import 'qrc:/QueMusic/components'

Item {
    id: page
    anchors.fill: parent

    property bool detailOpen: false
    property string detailTitle: ""
    property url detailCover: ""

    Component.onCompleted: {
        MusicApi.getPersonalFm(1, 10, MusicApi.songSource)
        MusicApi.getHotPlaylists(1, 1, 12)
        MusicApi.getNewSongs(0, 1, 12)
    }

    QScrollView {
        id: scroll
        anchors.fill: parent
        contentChildren: Column {
            width: scroll.availableWidth
            height: implicitHeight
            spacing: 22

            // Hero：正在播放 / 上次播放
            Item {
                width: parent.width
                height: 210
                Rectangle {
                    anchors.fill: parent
                    radius: 24
                    opacity: 0.94
                    gradient: Gradient {
                        GradientStop { position: 0; color: center.c1 }
                        GradientStop { position: 1; color: center.c2 }
                    }
                }
                QPicture {
                    x: 26
                    y: 26
                    width: 158
                    height: 158
                    radius: 18
                    source: center.cover
                    sourceSize: Qt.size(320, 320)
                }
                Column {
                    x: 208
                    y: 38
                    width: parent.width - 236
                    spacing: 8
                    Text {
                        width: parent.width
                        text: center.hasMedia ? "正在播放" : "上次播放"
                        font.pixelSize: 12
                        color: "#e9f4f6"
                    }
                    Text {
                        width: parent.width
                        text: center.hasMedia ? center.songTitle : (Options.lastSongs.name || "还没有播放记录")
                        font.pixelSize: 26
                        font.weight: Font.DemiBold
                        color: "#ffffff"
                        elide: Text.ElideRight
                    }
                    Text {
                        width: parent.width
                        text: center.hasMedia ? center.songArtist : Options.lastSongs.artist
                        font.pixelSize: 14
                        color: "#e2f1f4"
                        elide: Text.ElideRight
                    }
                }
                Row {
                    x: 208
                    y: 142
                    spacing: 10
                    QButton {
                        text: center.playing ? "暂停" : "播放"
                        iconCharacter: center.playing ? "\uf02f" : "\uf00e"
                        width: 116
                        height: 40
                        buttonColor: "#ffffff"
                        textColor: "#14161c"
                        iconColor: "#14161c"
                        onClicked: {
                            if (!center.hasMedia && Options.lastSongs.hash !== "") {
                                MusicApi.getMusicInfo(Options.lastSongs.hash, 0, Options.lastSongs.source)
                                return
                            }
                            Playback.togglePlay()
                        }
                    }
                    QButton {
                        text: "下一首"
                        iconCharacter: "\uf0d9"
                        width: 106
                        height: 40
                        buttonColor: "#2affffff"
                        textColor: "#ffffff"
                        iconColor: "#ffffff"
                        shadowEnabled: false
                        onClicked: Playback.next(false)
                    }
                    QButton {
                        text: "队列 " + (center.queue ? center.queue.count : 0)
                        iconCharacter: "\uf098"
                        width: 106
                        height: 40
                        buttonColor: "#2affffff"
                        textColor: "#ffffff"
                        iconColor: "#ffffff"
                        shadowEnabled: false
                        onClicked: center.showQueue = !center.showQueue
                    }
                }
            }

            CenterGrid {
                title: "私人漫游"
                width: parent.width
                source: MusicApi.personalFm
                cardWidth: 132
                cardHeight: 184
                onPicked: i => center.playOnline(MusicApi.personalFm.get(i))
            }

            CenterGrid {
                title: "推荐歌单"
                width: parent.width
                source: MusicApi.hotPlayLists
                cardWidth: 156
                onPicked: i => page.openList(MusicApi.hotPlayLists.get(i))
            }

            Column {
                width: parent.width
                spacing: 8
                Text {
                    width: parent.width
                    height: 26
                    text: "新歌速递"
                    font.pixelSize: 15
                    font.weight: Font.DemiBold
                    color: "#f5f7fb"
                    verticalAlignment: Text.AlignVCenter
                }
                QListView {
                    width: parent.width
                    height: 380
                    model: MusicApi.newSongs
                    toolText0: "\uf095"
                    toolText1: "\uf0c8"
                    onClicked: i => center.playOnline(MusicApi.newSongs.get(i))
                    onToolClicked: (i, tool) => center.toolAction(tool, MusicApi.newSongs.get(i))
                    onMenuClicked: (i, choice) => center.menuAction(choice, MusicApi.newSongs.get(i))
                }
            }
        }
    }

    CenterDetail {
        width: parent.width
        height: parent.height
        opened: page.detailOpen
        title: page.detailTitle
        cover: page.detailCover
        onCloseClicked: page.detailOpen = false
        onPicked: (i, d) => center.playOnline(d)
        onQueued: (i, d) => center.enqueue(d)
        onFaved: (i, d) => center.toggleFavorite(d)
        onDownloaded: (i, d) => center.download(d)
    }

    function openList(d) {
        page.detailTitle = d.title || "歌单"
        page.detailCover = center.coverOf(d.cover)
        MusicApi.playlistSong.clear()
        MusicApi.getPlaylistSongs(d.hash, 1, 50)
        page.detailOpen = true
    }
}
