// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
// 推荐页（对应 pages/HomePage.qml）喵~
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
        MusicApi.getPersonalFm(1, 24, MusicApi.songSource)
        MusicApi.getHotPlaylists(1)
        MusicApi.getNewSongs(0, 1, 20)
    }

    QScrollView {
        id: scroll
        anchors.fill: parent
        Column {
            // 用 width 而非 availableWidth：Flickable.availableWidth 无 NOTIFY，
            // 构造期为 0 后绑定不再更新，内容会整块 0 宽而不可见
            width: scroll.width
            height: implicitHeight
            spacing: 24

            // Hero：正在播放 / 上次播放
            Item {
                width: parent.width
                height: 240

                // 封面主色氛围光
                Rectangle {
                    width: 460
                    height: 300
                    x: parent.width - 340
                    y: -110
                    radius: Math.max(width, height) / 2
                    opacity: 0.5
                    gradient: Gradient {
                        GradientStop { position: 0; color: center.c2 }
                        GradientStop { position: 1; color: "#00ffffff" }
                    }
                }
                Rectangle {
                    width: 420
                    height: 280
                    x: -120
                    y: parent.height - 140
                    radius: Math.max(width, height) / 2
                    opacity: 0.45
                    gradient: Gradient {
                        GradientStop { position: 0; color: center.c1 }
                        GradientStop { position: 1; color: "#00ffffff" }
                    }
                }

                // 玻璃卡体
                Rectangle {
                    anchors.fill: parent
                    radius: 28
                    color: "#16ffffff"
                    border.width: 1
                    border.color: "#24ffffff"
                    Rectangle {
                        x: parent.width * 0.05
                        y: 1
                        width: parent.width * 0.9
                        height: 1
                        gradient: Gradient {
                            orientation: Gradient.Horizontal
                            GradientStop { position: 0; color: "#00ffffff" }
                            GradientStop { position: 0.5; color: "#3dffffff" }
                            GradientStop { position: 1; color: "#00ffffff" }
                        }
                    }
                }

                QPicture {
                    x: 32
                    y: 32
                    width: 176
                    height: 176
                    radius: 20
                    source: center.cover
                    sourceSize: Qt.size(360, 360)
                }

                Column {
                    x: 240
                    y: 40
                    width: parent.width - 270
                    spacing: 10
                    Rectangle {
                        width: heroBadge.implicitWidth + 20
                        height: 22
                        radius: 11
                        color: "#2bffffff"
                        border.width: 1
                        border.color: "#2affffff"
                        Text {
                            id: heroBadge
                            anchors.centerIn: parent
                            text: center.hasMedia ? "正在播放" : "上次播放"
                            font.pixelSize: 10
                            font.weight: Font.DemiBold
                            font.letterSpacing: 1.8
                            color: "#dfe7f2"
                        }
                    }
                    Text {
                        width: parent.width
                        text: center.hasMedia ? center.songTitle : (Options.lastSongs.name || "还没有播放记录")
                        font.pixelSize: 27
                        font.weight: Font.DemiBold
                        font.letterSpacing: 0.2
                        color: "#ffffff"
                        elide: Text.ElideRight
                    }
                    Text {
                        width: parent.width
                        text: center.hasMedia ? center.songArtist : Options.lastSongs.artist
                        font.pixelSize: 14
                        color: "#b6c1d1"
                        elide: Text.ElideRight
                    }
                }

                Row {
                    x: 240
                    y: 156
                    spacing: 12
                    QButton {
                        text: center.playing ? "暂停" : "播放"
                        iconCharacter: center.playing ? "\uf02f" : "\uf00e"
                        width: 118
                        height: 42
                        radius: 21
                        buttonColor: "#f2f5fa"
                        textColor: "#101318"
                        iconColor: "#101318"
                        shadowEnabled: false
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
                        width: 108
                        height: 42
                        radius: 21
                        buttonColor: "#1fffffff"
                        textColor: "#f2f5fa"
                        iconColor: "#f2f5fa"
                        shadowEnabled: false
                        onClicked: Playback.next(false)
                    }
                    QButton {
                        text: "队列 " + (center.queue ? center.queue.count : 0)
                        iconCharacter: "\uf098"
                        width: 108
                        height: 42
                        radius: 21
                        buttonColor: "#1fffffff"
                        textColor: "#f2f5fa"
                        iconColor: "#f2f5fa"
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
                spacing: 12
                Row {
                    width: parent.width
                    height: 26
                    spacing: 8
                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        width: 3
                        height: 14
                        radius: 1.5
                        color: center.c1
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "新歌速递"
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                        font.letterSpacing: 0.3
                        color: "#f2f5fa"
                        verticalAlignment: Text.AlignVCenter
                    }
                }
                Rectangle {
                    width: parent.width
                    height: 420
                    radius: 22
                    color: Style.themes.primaryColor
                    border.width: 1
                    border.color: "#12ffffff"
                    QListView {
                        anchors.fill: parent
                        anchors.margins: 8
                        model: MusicApi.newSongs
                        toolText0: "\uf095"
                        toolText1: "\uf0c8"
                        onClicked: i => center.playOnline(MusicApi.newSongs.get(i))
                        onEnded: {
                            if (MusicApi.newSongs.count % 20 === 0 && MusicApi.newSongs.count !== 0) {
                                MusicApi.getNewSongs(0, MusicApi.newSongs.count / 20 + 1, 20)
                                isEnd = false
                            } else if (MusicApi.newSongs.count !== 0) {
                                isEnd = true
                            }
                        }
                    }
                }
            }
        }
    }

    CenterDetail {
        id: detail
        width: parent.width
        height: parent.height
        blurSource: mainLayout
        opened: page.detailOpen
        title: page.detailTitle
        cover: page.detailCover
        onCloseClicked: page.detailOpen = false
        onPicked: (i, d) => center.playOnline(d)
    }

    function openList(d: var): void {
        page.detailTitle = d.title || "歌单"
        page.detailCover = center.coverOf(d.cover)
        MusicApi.playlistSong.clear()
        MusicApi.getPlaylistSongs(d.hash, 1, detail.pageSize)
        detail.loadMore = function(): boolean {
            const c = MusicApi.playlistSong.count
            if (c === 0 || c % detail.pageSize !== 0)
                return false
            MusicApi.getPlaylistSongs(d.hash, c / detail.pageSize + 1, detail.pageSize)
            return true
        }
        page.detailOpen = true
    }
}
