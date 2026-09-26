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

    // 分类列表到达后自动加载第一个分类的歌单
    Connections {
        target: MusicApi
        function onAllPlaylistMenuChanged(): void {
            if (page.menuIndex === 0 && MusicApi.musicPlaylists.count === 0
                    && MusicApi.allPlaylistMenu.length > 0)
                MusicApi.getCategoryPlaylists(MusicApi.allPlaylistMenu[0].id, 1, 30)
        }
    }

    QScrollView {
        id: scroll
        anchors.fill: parent
        Column {
            width: scroll.width
            height: implicitHeight
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
                        text: "分类"
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
                        MusicApi.getCategoryPlaylists(MusicApi.allPlaylistMenu[i].id, 1, 30)
                    }
                }
            }

            CenterGrid {
                // B 站没有歌单实体，分类下展示的是该子分区的热门稿件
                title: MusicApi.songSource === 2 ? "分区热门" : "分类歌单"
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
                badgeOf: m => m.source === 0 ? "酷狗" : m.source === 1 ? "网易云" : "B站"
                onPicked: i => {
                    const d = MusicApi.toplistList.get(i)
                    page.showDetail(d.title || "榜单", d.cover, page.toplistLoader(Number(d.hash || d.rankid), d.source))
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
                    const d = MusicApi.singerList.get(i)
                    page.showDetail(d.title || "歌手", d.cover, page.singerLoader(d.hash))
                    MusicApi.getSingerSongs(d.hash, 1, 50, MusicApi.songSource)
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

    function showDetail(t: var, c: var, more: var): void {
        page.detailTitle = t
        page.detailCover = center.coverOf(c)
        detail.loadMore = more
        MusicApi.playlistSong.clear()
        page.detailOpen = true
    }
    function openList(d: var): void {
        page.showDetail(d.title || "歌单", d.cover, page.playlistLoader(d.hash))
        MusicApi.getPlaylistSongs(d.hash, 1, detail.pageSize)
    }
    // 下一页加载器：整页返回才继续，注入给 CenterDetail.onEnded
    function playlistLoader(id: var): var {
        return function(): boolean {
            const c = MusicApi.playlistSong.count
            if (c === 0 || c % detail.pageSize !== 0)
                return false
            MusicApi.getPlaylistSongs(id, c / detail.pageSize + 1, detail.pageSize)
            return true
        }
    }
    function toplistLoader(id: var, src: var): var {
        return function(): boolean {
            const c = MusicApi.playlistSong.count
            if (c === 0 || c % 30 !== 0)
                return false
            MusicApi.getMusicToplist(c / 30 + 1, 30, id, src)
            return true
        }
    }
    function singerLoader(id: var): var {
        return function(): boolean {
            const c = MusicApi.playlistSong.count
            if (c === 0 || c % 50 !== 0)
                return false
            MusicApi.getSingerSongs(id, c / 50 + 1, 50, MusicApi.songSource)
            return true
        }
    }
}
