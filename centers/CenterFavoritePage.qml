// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
// 收藏页（对应 pages/FavouritePage.qml）
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
            Text {
                text: "收藏"
                font.pixelSize: 16
                font.weight: Font.DemiBold
                color: "#f5f7fb"
                height: parent.height
                verticalAlignment: Text.AlignVCenter
            }
            CenterTabs {
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
            QListView {
                anchors.fill: parent
                visible: page.favTab === 0
                model: FavoriteSongs
                isList: false
                onClicked: i => {
                    var d = FavoriteSongs.get(i)
                    if (d.source === -1) center.playLocal(d.favId, d.title)
                    else MusicApi.getMusicInfo(d.favId, 0, d.source)
                }
                onToolClicked: (i, tool) => center.toolAction(tool, FavoriteSongs.get(i))
                onMenuClicked: (i, choice) => center.menuAction(choice, FavoriteSongs.get(i))
            }
            QListView {
                anchors.fill: parent
                visible: page.favTab === 1
                model: FavoritePlaylists
                isList: true
                headerModel: ["标题", "创建者", "曲目", "操作"]
                onClicked: i => page.openList(FavoritePlaylists.get(i))
            }
            QListView {
                anchors.fill: parent
                visible: page.favTab === 2
                model: FavoriteArtists
                isList: true
                headerModel: ["歌手", "流派", "曲目", "操作"]
                onClicked: i => page.openSinger(FavoriteArtists.get(i))
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
        MusicApi.getPlaylistSongs(d.favId, 1, 50)
        page.detailOpen = true
    }
    function openSinger(d) {
        page.detailTitle = d.title || "歌手"
        page.detailCover = center.coverOf(d.cover)
        MusicApi.playlistSong.clear()
        MusicApi.getSingerSongs(d.favId, 1, 50, MusicApi.songSource)
        page.detailOpen = true
    }
}
