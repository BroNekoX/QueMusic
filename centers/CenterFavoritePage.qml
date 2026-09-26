// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
// 收藏页（对应 pages/FavouritePage.qml）喵~
import QtQuick
import QueMusic 1.0
import 'qrc:/QueMusic/components'

Item {
    id: page
    anchors.fill: parent

    property int favTab: 0
    property bool detailOpen: false
    property string detailTitle: ""
    property url detailCover: ""

    Column {
        anchors.fill: parent
        spacing: 14
        Row {
            width: parent.width
            height: 36
            spacing: 12
            Row {
                height: parent.height
                spacing: 8
                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 3
                    height: 14
                    radius: 1.5
                    color: center.c1
                }
                Text {
                    text: "收藏"
                    font.pixelSize: 16
                    font.weight: Font.DemiBold
                    font.letterSpacing: 0.3
                    color: "#f2f5fa"
                    height: parent.height
                    verticalAlignment: Text.AlignVCenter
                }
            }
            CenterTabs {
                anchors.verticalCenter: parent.verticalCenter
                model: ["歌曲", "歌单", "歌手"]
                tabWidth: 64
                height: 32
                currentIndex: page.favTab
                onTabClicked: i => page.favTab = i
            }
        }

        Item {
            width: parent.width
            height: parent.height - 50
            Rectangle {
                anchors.fill: parent
                radius: 22
                color: Style.themes.primaryColor
                border.width: 1
                border.color: "#12ffffff"
            }
            QListView {
                anchors.fill: parent
                anchors.margins: 8
                visible: page.favTab === 0
                model: FavoriteSongs
                isList: false
                onClicked: i => {
                    const d = FavoriteSongs.get(i)
                    if (d.source === -1) center.playLocal(d.id, d.title)
                    else MusicApi.getMusicInfo(d.id, 0, d.source)
                }

            }
            QListView {
                anchors.fill: parent
                anchors.margins: 8
                visible: page.favTab === 1
                model: FavoritePlaylists
                isList: true
                headerModel: ["标题", "创建者", "曲目", "操作"]
                onClicked: i => page.openList(FavoritePlaylists.get(i))
            }
            QListView {
                anchors.fill: parent
                anchors.margins: 8
                visible: page.favTab === 2
                model: FavoriteArtists
                isList: true
                headerModel: ["歌手", "流派", "曲目", "操作"]
                onClicked: i => page.openSinger(FavoriteArtists.get(i))
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
        MusicApi.getPlaylistSongs(d.id, 1, detail.pageSize)
        detail.loadMore = function(): boolean {
            const c = MusicApi.playlistSong.count
            if (c === 0 || c % detail.pageSize !== 0)
                return false
            MusicApi.getPlaylistSongs(d.id, c / detail.pageSize + 1, detail.pageSize)
            return true
        }
        page.detailOpen = true
    }
    function openSinger(d: var): void {
        page.detailTitle = d.title || "歌手"
        page.detailCover = center.coverOf(d.cover)
        MusicApi.playlistSong.clear()
        MusicApi.getSingerSongs(d.id, 1, 50, MusicApi.songSource)
        detail.loadMore = function(): boolean {
            const c = MusicApi.playlistSong.count
            if (c === 0 || c % 50 !== 0)
                return false
            MusicApi.getSingerSongs(d.id, c / 50 + 1, 50, MusicApi.songSource)
            return true
        }
        page.detailOpen = true
    }
}
