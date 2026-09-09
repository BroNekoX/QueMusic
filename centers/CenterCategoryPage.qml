// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
// 分类页（对应 pages/PlaylistPage.qml）：歌单 / 榜单 / 歌手
import QtQuick
import QueMusic 1.0
import 'qrc:/QueMusic/components'

Item {
    id: page
    anchors.fill: parent

    property int categoryTab: 0
    property int menuIndex: 0
    property bool detailOpen: false
    property string detailTitle: ""
    property url detailCover: ""

    Component.onCompleted: {
        MusicApi.getPlaylistMenu(3)
        MusicApi.getAllToplist()
        MusicApi.getHotSingers()
    }

    QScrollView {
        id: scroll
        anchors.fill: parent
        contentChildren: Column {
            width: scroll.availableWidth
            height: implicitHeight
            spacing: 14

            Row {
                width: parent.width
                height: 36
                spacing: 12
                Text {
                    text: "分类"
                    font.pixelSize: 16
                    font.weight: Font.DemiBold
                    color: "#f5f7fb"
                    height: parent.height
                    verticalAlignment: Text.AlignVCenter
                }
                CenterTabs {
                    model: ["歌单", "榜单", "歌手"]
                    tabWidth: 64
                    height: 32
                    currentIndex: page.categoryTab
                    onTabClicked: i => page.categoryTab = i
                }
            }

            // 分类标签可能很多，横向滚动而不是换行（保持胶囊指示位置可算）
            Flickable {
                width: parent.width
                height: 32
                contentWidth: catTabs.width
                clip: true
                visible: page.categoryTab === 0
                boundsBehavior: Flickable.StopAtBounds
                CenterTabs {
                    id: catTabs
                    height: 32
                    model: MusicApi.allPlaylistMenu.map(m => m.title)
                    tabWidth: 76
                    currentIndex: page.menuIndex
                    onTabClicked: i => {
                        page.menuIndex = i
                        MusicApi.musicPlaylists.clear()
                        MusicApi.getMusicPlaylists(MusicApi.allPlaylistMenu[i].id, 1, 30)
                    }
                }
            }

            CenterGrid {
                title: "分类歌单"
                width: parent.width
                visible: page.categoryTab === 0
                source: MusicApi.musicPlaylists
                cardWidth: 148
                onPicked: i => page.openList(MusicApi.musicPlaylists.get(i))
            }
            CenterGrid {
                title: "排行榜"
                width: parent.width
                visible: page.categoryTab === 1
                source: MusicApi.toplistList
                cardWidth: 148
                badgeOf: m => m.source === 1 ? "网易云" : "酷狗"
                onPicked: i => {
                    var d = MusicApi.toplistList.get(i)
                    page.showDetail(d.title || "榜单", d.cover)
                    MusicApi.getMusicToplist(1, 30, Number(d.hash || d.rankid), d.source)
                }
            }
            CenterGrid {
                title: "热门歌手"
                width: parent.width
                visible: page.categoryTab === 2
                source: MusicApi.singerList
                cardWidth: 116
                cardHeight: 168
                round: true
                onPicked: i => {
                    var d = MusicApi.singerList.get(i)
                    page.showDetail(d.title || "歌手", d.cover)
                    MusicApi.getSingerSongs(d.hash, 1, 50, MusicApi.songSource)
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

    function showDetail(t, c) {
        page.detailTitle = t
        page.detailCover = center.coverOf(c)
        MusicApi.playlistSong.clear()
        page.detailOpen = true
    }
    function openList(d) {
        page.showDetail(d.title || "歌单", d.cover)
        MusicApi.getPlaylistSongs(d.hash, 1, 50)
    }
}
