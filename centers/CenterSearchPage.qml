// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
// 搜索页（对应 pages/SearchPage.qml）：搜索框移入本页
import QtQuick
import QueMusic 1.0
import 'qrc:/QueMusic/components'

Item {
    id: page
    anchors.fill: parent

    property int searchTab: 0
    property bool detailOpen: false
    property string detailTitle: ""
    property url detailCover: ""

    Column {
        anchors.fill: parent
        spacing: 14
        Row {
            width: parent.width
            height: 38
            spacing: 12
            QInput {
                id: input
                width: Math.min(420, parent.width * 0.4)
                height: 38
                radius: 19
                inputText: center.searchKey
                onEntered: center.doSearch(input.inputText)
            }
            QButton {
                text: "搜索"
                iconCharacter: "\uf100"
                height: 38
                onClicked: center.doSearch(input.inputText)
            }
            CenterTabs {
                model: ["歌曲", "歌单", "专辑", "歌词"]
                tabWidth: 64
                height: 32
                anchors.verticalCenter: parent.verticalCenter
                currentIndex: page.searchTab
                onTabClicked: i => page.switchTab(i)
            }
        }

        QListView {
            width: parent.width
            height: parent.height - 52
            visible: page.searchTab === 0 || page.searchTab === 3
            model: MusicApi.searchSongsResults
            toolText0: "\uf095"
            toolText1: "\uf0c8"
            onClicked: i => center.playOnline(MusicApi.searchSongsResults.get(i))
            onToolClicked: (i, tool) => center.toolAction(tool, MusicApi.searchSongsResults.get(i))
            onMenuClicked: (i, choice) => center.menuAction(choice, MusicApi.searchSongsResults.get(i))
        }

        CenterGrid {
            title: "歌单 / 专辑"
            width: parent.width
            visible: page.searchTab === 1 || page.searchTab === 2
            source: MusicApi.searchSongsResults
            cardWidth: 148
            onPicked: i => page.openList(MusicApi.searchSongsResults.get(i))
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
    function switchTab(index) {
        page.searchTab = index
        MusicApi.searchSongsResults.clear()
        MusicApi.nowIndex = index
        if (center.searchKey !== "")
            MusicApi.searchSongs(center.searchKey, index, 1, 30)
    }
}
