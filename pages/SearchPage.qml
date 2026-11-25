// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
pragma ComponentBehavior: Bound
import QtQuick
import QueMusic 1.0

Item {
    id: searchPage

    property int searchTab: 0

    Component.onCompleted: Options.searchTab = searchTab

    Connections {
        target: Options
        function onReSearch(): void {
            searchResult.scrollTop();
        }
    }

    Item {
        id: searchContent
        x: 0
        y: 0
        width: parent.width
        height: parent.height

        Item {
            x: 24
            y: 24
            width: parent.width - 48
            height: 40
            z: 10
            Text {
                height: 40
                verticalAlignment: Text.AlignVCenter
                text: "搜索结果"
                font.pixelSize: Style.pageTitle
                font.weight: Font.DemiBold
                color: Theme.fontColor
                QLoadSign {
                    id: searchLoad
                    x: parent.width
                    y: 2
                }
            }
            QDrop {
                width: 128
                height: 36
                radius: 18
                anchors.right: parent.right
                cardRadius: Style.labelRadius
                choice: Math.max(0, MusicApi.sourceIds.indexOf(MusicApi.songSource))
                model: MusicApi.sourceNames
                onTransformed: (choiced) => {
                    MusicApi.songSource = MusicApi.sourceIdAt(choiced);
                    MusicApi.searchSongsResults.clear();
                    if (searchPage.searchTab === 2)
                        MusicApi.searchMvs(Options.searchText, 1, 20);
                    else
                        MusicApi.searchSongs(Options.searchText, MusicApi.nowIndex, 1, 20);
                    Options.exitIndex = 1;
                }
            }
        }

        QBlurTapBar {
            x: 24
            y: 80
            z: 5
            model: ["歌曲","歌单","MV","歌词"]
            tabWidth: 75
            width: 306
            rectXy: Qt.rect(0, 12, width, 40)
            blurSource: searchResult
            onTabChange: (index) => {
                searchPage.searchTab = index
                Options.searchTab = index
                MusicApi.searchSongsResults.clear()
                searchResult.scrollTop()
                if (index === 2) {
                    MusicApi.searchMvs(Options.searchText, 1, 20)
                    return
                }
                MusicApi.nowIndex = index
                MusicApi.searchSongs(Options.searchText, index, 1, 20)
            }
        }
        // 右侧操作区
        Row {
            x: parent.width - width - 24
            y: 80
            z: 6
            // 刷新：重新拉取当前页签 / 当前歌曲列表
            QButton {
                id: refreshBtn
                height: 38
                text: ""
                iconCharacter: "\uf11e"
                buttonColor: Theme.primaryColor
                tipText: "重新获取当前列表"
                onClicked: {
                    MusicApi.searchSongsResults.clear()
                    searchResult.scrollTop()
                    if (searchPage.searchTab === 2) {
                        MusicApi.searchMvs(Options.searchText, 1, 20)
                        return
                    }
                    MusicApi.searchSongs(Options.searchText, searchPage.searchTab, 1, 20)
                }
            }
        }

        QListView {
            id: searchResult
            x: 24
            y: 68
            width: parent.width - 32
            height: parent.height - 68
            model: MusicApi.searchSongsResults
            clip: true
            topMargin: 72
            bottomMargin: 24
            isList: searchPage.searchTab === 1
            rowActions: searchPage.searchTab !== 2
            headerModel: searchPage.searchTab === 2
                         ? ["MV 标题","歌手","时长"]
                         : (searchResult.isList ? ["标题","创建者","曲目","操作"]
                                                : ["标题","歌手","时长","操作"])
            menuModel: searchPage.searchTab === 2
                       ? ["播放 MV"]
                       : (searchResult.isList ? ["收藏","歌单信息"]
                                              : ["下载到本地","下一首播放","添加到列表","收藏","歌曲信息"])
            menuHandler: searchPage.searchTab === 2

            function openMv(index: int): void {
                const mv = model.get(index)
                if (mv && MusicApi.playMv(mv))
                    Desktop.openMv()
            }

            onEnded: {
                const count = MusicApi.searchSongsResults.count
                if (count === 0)
                    return
                if (count % 20 !== 0) {
                    isEnd = true
                    return
                }
                const page = count / 20 + 1
                if (searchPage.searchTab === 2)
                    MusicApi.searchMvs(Options.searchText, page, 20)
                else
                    MusicApi.searchSongs(Options.searchText, searchPage.searchTab, page, 20)
                isEnd = false
            }

            onClicked: (index) => {
                const item = model.get(index)
                if (!item)
                    return
                if (searchPage.searchTab === 2) {
                    searchResult.openMv(index)
                } else if (searchPage.searchTab === 1) {
                    MusicApi.playlistSong.clear()
                    MusicApi.globalid = item.hash
                    MusicApi.getPlaylistSongs(item.hash, 1, 20)
                    playListSongsWindow.opened(item)
                    Options.exitIndex = 2
                } else {
                    MusicApi.playSong(item)
                }
            }

            onMenuClicked: (choice, index) => {
                if (searchPage.searchTab === 2)
                    searchResult.openMv(index)
            }
        }
    }

    PlayListWindow {
        id: playListSongsWindow
        mainTarget: searchContent
        winIndex: 2
        content: Item {

            QListView {
                id: playListsView
                x: 24
                y: 170
                width: playListSongsWindow.width - 32
                height: playListSongsWindow.height - 170
                model: MusicApi.playlistSong
                clip: true
                topMargin: 8
                bottomMargin: 24

                onClicked: (index) => MusicApi.playSong(model.get(index))

                onEnded: {
                    if(MusicApi.playlistSong.count % 20 === 0 && MusicApi.playlistSong.count !== 0) {
                        const tagid = playListSongsWindow.id;
                        MusicApi.getPlaylistSongs(tagid,MusicApi.playlistSong.count / 20 + 1,20);
                        isEnd = false;
                    } else {
                        if(MusicApi.playlistSong.count !== 0) {
                            isEnd = true;
                        }
                    }
                }
            }
        }
    }
}
