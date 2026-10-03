// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QueMusic 1.0

Item {
    id: favouritePage
    property int setMode: 0
    property list<int> chooseIndex: []
    property var favSortOptions: [
        { label: "默认顺序", mode: 0, desc: false },
        { label: "标题 A→Z", mode: 1, desc: false },
        { label: "标题 Z→A", mode: 1, desc: true },
        { label: "歌手 A→Z", mode: 2, desc: false },
        { label: "歌手 Z→A", mode: 2, desc: true },
        { label: "时长 小→大", mode: 3, desc: false },
        { label: "时长 大→小", mode: 3, desc: true }
    ]

    QSortModel {
        id: songSort
        model: FavoriteSongs
        options: favouritePage.favSortOptions
        nameRole: "title"
    }

    QSortModel {
        id: listSort
        model: FavoritePlaylists
        options: favouritePage.favSortOptions
        nameRole: "title"
    }
    QMenu {
        id: sortMenu
        model: favSortOptions.map(o => o.label)
        current: favouriteChildPage.lastIndex === 0 ? songSort.menuIndex : listSort.menuIndex
        onClicked: (i) => {
            const target = favouriteChildPage.lastIndex === 0 ? songSort : listSort;
            target.selectMenu(i);
            favouritePage.setMode = 0;
            favouritePage.chooseIndex = [];
            songs.scrollTop();
            lists.scrollTop();
        }
    }

    function chooseTotal(): int {
        return favouriteChildPage.lastIndex === 0 ? FavoriteSongs.count : FavoritePlaylists.count;
    }

    function isAllChosen(): bool {
        const total = favouritePage.chooseTotal();
        return total > 0 && favouritePage.chooseIndex.length >= total;
    }

    function toggleAllChoose(): void {
        const total = favouritePage.chooseTotal();
        if (total === 0) return;
        if (favouritePage.isAllChosen()) {
            favouritePage.chooseIndex = [];
            return;
        }
        const all = [];
        for (let i = 0; i < total; i++) all.push(i);
        favouritePage.chooseIndex = all;
    }

    QPages {
        id: favouriteChildPage
        x: 24
        y: 68
        width: parent.width - 48
        height: parent.height - 68
        pageList: [songsLoader,listsLoader,singer,historyLoader]

        // 顶部常驻显示
        Item {
            x: 0
            y: -44
            height: 40
            width: favouriteChildPage
            z: 10
            Text {
                x: 0
                y: 0
                height: 40
                verticalAlignment: Text.AlignVCenter
                text: "收藏内容"
                font.weight: Font.DemiBold
                font.pixelSize: Style.settings.pageTitle
                color: Style.fontColor
            }
        }

        QBlurTapBar {
            x: 0
            y: 12
            z: 5
            model: ["歌曲","歌单","歌手","历史"]
            tabWidth: 75
            width: 306
            rectXy: Qt.rect(0, 12, width, 40)
            blurSource: favouriteChildPage.pageList[favouriteChildPage.lastIndex]
            onTabChange: (index) => {
                favouriteChildPage.stack(index);
                favouritePage.setMode = 0;
                favouritePage.chooseIndex = [];
            }
        }

        // 右侧操作区
        Row {
            x: parent.width - width
            y: 13
            z: 2
            spacing: 8
            QButton {
                id: sortBtn
                height: 38
                text: "排序"
                iconCharacter: "\uf10b"
                buttonColor: Style.primaryColor
                onClicked: sortMenu.popup(sortBtn, 0, sortBtn.height + 6)
            }
            QButton {
                visible: favouriteChildPage.lastIndex === 4
                height: 38
                text: "清空历史"
                iconCharacter: "\uf08e"
                onClicked: Options.dialog.openSimpleDialog("清空", "将清空全部播放历史，是否继续？",
                    function() {
                        Playback.clearHistory()
                        Options.warn.tiped("已清空播放历史", 1)
                    })
            }
            QButton {
                height: 38
                text: favouritePage.setMode === 1 ? "取消选择" : "选择"
                iconCharacter: "\uf09f"
                buttonColor: favouritePage.setMode === 1 ? Style.containColor : Style.primaryColor
                onClicked: {
                    if(favouritePage.setMode === 1) {
                        favouritePage.setMode = 0;
                        favouritePage.chooseIndex = [];
                    } else {
                        favouritePage.setMode = 1;
                    }
                }
            }
        }

        Loader {
            id: songsLoader
            asynchronous: true
            active: visible
            visible: true
            sourceComponent: QListView {
                id: songs
                width: favouriteChildPage.width + 16
                height: favouriteChildPage.height
                model: songSort
                clip: true
                topMargin: 72
                selectedIndices: favouritePage.chooseIndex

                onClicked: (index) => {
                    if (favouritePage.setMode === 1) {
                        const idx = favouritePage.chooseIndex.indexOf(index);
                        if (idx === -1) {
                            favouritePage.chooseIndex = favouritePage.chooseIndex.concat([index]);
                        } else {
                            favouritePage.chooseIndex = favouritePage.chooseIndex.filter(v => v !== index);
                        }
                    } else {
                        const r = songSort.at(index);
                        MusicApi.getMusicInfo(r.id, 0, r.source);
                    }
                }
                Text {
                    anchors.centerIn: parent
                    visible: !FavoriteSongs.loading && FavoriteSongs.count === 0
                    text: "没有收藏的内容？快去收藏一些歌曲吧"
                    color: Style.textColor
                    font.pixelSize: 14
                }
            }
        }

        Loader {
            id: listsLoader
            asynchronous: true
            active: visible
            visible: false
            sourceComponent: QListView {
                id: lists
                width: favouriteChildPage.width + 16
                height: favouriteChildPage.height
                model: listSort
                clip: true
                isList: true
                topMargin: 72
                selectedIndices: favouritePage.chooseIndex

                onClicked: (index) => {
                    if (favouritePage.setMode === 1) {
                        const idx = favouritePage.chooseIndex.indexOf(index);
                        if (idx === -1) {
                            favouritePage.chooseIndex = favouritePage.chooseIndex.concat([index]);
                        } else {
                            favouritePage.chooseIndex = favouritePage.chooseIndex.filter(v => v !== index);
                        }
                    } else {
                        const r = listSort.at(index);
                        MusicApi.playlistSong.clear();
                        MusicApi.globalid = r.id;
                        MusicApi.getPlaylistSongs(r.id,1,20,r.source);
                        playListSongsWindow.songSource = r.source;
                        playListSongsWindow.opened({ id: r.id, title: r.title, artist: r.artist, cover: r.cover, duration: r.duration });
                        Options.exitIndex = 1;
                    }
                }
                AnimatedImage {
                    anchors.centerIn: parent
                    visible: FavoritePlaylists.loading
                    playing: visible
                    source: "qrc:/QueMusic/resources/loader.gif"
                }
                Text {
                    anchors.centerIn: parent
                    visible: !FavoritePlaylists.loading && FavoritePlaylists.count === 0
                    text: "没有收藏的内容？快去收藏一些歌单吧"
                    color: Style.textColor
                    font.pixelSize: 14
                }
            }
        }
        Item {
            id: singer
            visible: false
            width: favouriteChildPage.width
            height: favouriteChildPage.height
            Text {
                anchors.centerIn: parent
                text: "喜欢的歌手"
                color: Style.textColor
                font.pixelSize: 14
            }
        }

        Loader {
            id: historyLoader
            asynchronous: true
            active: visible
            visible: false
            sourceComponent: QListView {
                id: history
                width: favouriteChildPage.width + 16
                height: favouriteChildPage.height
                model: Playback.history
                clip: true
                topMargin: 72
                menuModel: ["加入播放列表", "收藏", "移除记录"]
                toolText0: "\uf095"
                toolText1: "\uf0c8"

                function addToQueue(e: var): void {
                    if (Options.queue.indexOfPath(e.path) !== -1) return
                    Options.queue.append({ name: e.title, path: e.path, songer: e.artist, source: e.source })
                    Options.warn.tiped("已加入播放列表", 1)
                }
                function toggleFav(e: var): void {
                    if (e.source === -1) { Options.warn.tiped("本地歌曲请使用本地收藏", 0); return }
                    if (FavoriteSongs.isFavorite(e.path, "song")) {
                        FavoriteSongs.removeFavorite(e.path, "song")
                        Options.warn.tiped("已取消收藏", 0)
                    } else {
                        FavoriteSongs.addFavorite(e.path, e.title, e.artist, e.cover, e.source, e.duration, "song")
                        Options.warn.tiped("已收藏", 1)
                    }
                }

                onClicked: (index) => {
                    const e = Playback.history.get(index)
                    Playback.playItem({ name: e.title, path: e.path, songer: e.artist, source: e.source })
                }
                // 记录列表有专属菜单项（移除记录），其余动作仍走内置实现
                menuHandler: function(choice: int, index: int): bool {
                    const e = Playback.history.get(index)
                    if (choice === 0) { history.addToQueue(e); return true }
                    if (choice === 1) { history.toggleFav(e); return true }
                    Playback.history.remove(index, 1)
                    return true
                }
                Text {
                    anchors.centerIn: parent
                    visible: Playback.history.count === 0
                    text: "还没有播放记录，去听点什么吧"
                    color: Style.textColor
                    font.pixelSize: 14
                }
            }
        }

        // 选择模式
        Rectangle {
            id: chooseArea
            x: -24
            y: favouriteChildPage.height - 60
            opacity: visible ? 1 : 0
            width: favouritePage.width
            height: 60
            visible: favouritePage.setMode !== 0
            gradient: Gradient {
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 1.0; color: Style.sideColor }
            }
            Behavior on opacity { NumberAnimation { duration: 320; easing.type: Easing.OutCubic } }
            QButton {
                shadowEnabled: false
                x: 16
                y: 12
                width: 92
                height: 36
                radius: 20
                borderWidth: 1
                buttonColor: favouritePage.isAllChosen() ? Style.themeColor : Style.fullColor
                textColor: favouritePage.isAllChosen() ? Style.primaryColor : Style.fontColor
                text: "全选"
                onClicked: favouritePage.toggleAllChoose()
            }
            Rectangle {
                x: 118
                y: 12
                width: 92
                height: 36
                radius: 20
                color: "transparent"//Style.fullColor
                Text {
                    anchors.centerIn: parent
                    text: "已选择:" + favouritePage.chooseIndex.length + "项"
                    color: Style.textColor
                    font.pixelSize: Style.settings.textmain
                }
            }
            Row {
                y: 12
                x: chooseArea.width - width - 16
                spacing: 8
                QButton {
                    shadowEnabled: false
                    height: 36
                    radius: 20
                    borderWidth: 1
                    buttonColor: "#fa4642"
                    text: "取消收藏"
                    onClicked: {
                        switch(favouritePage.setMode) {
                        case 1:
                            Options.dialog.openSimpleDialog("取消收藏",
                                "将取消收藏选中的 " + favouritePage.chooseIndex.length + " 首歌曲",
                                function() {
                                    // 先记下数量：下面清空 chooseIndex 后就读不到了
                                    const total = favouritePage.chooseIndex.length;
                                    for(var i = 0; i < total; i++) {
                                        const r = songSort.at(favouritePage.chooseIndex[i]);
                                        if (r) FavoriteSongs.removeFavorite(r.id, "song");
                                    }
                                    favouritePage.chooseIndex = [];
                                    Style.warned("已取消收藏 " + total + " 首歌曲", 1);
                                }
                            );
                            break;
                        case 2:
                            Options.dialog.openSimpleDialog("取消收藏",
                                "将取消收藏选中的 " + favouritePage.chooseIndex.length + " 个歌单",
                                function() {
                                    const total = favouritePage.chooseIndex.length;
                                    for(var i = 0; i < total; i++) {
                                        const r = listSort.at(favouritePage.chooseIndex[i]);
                                        if (r) FavoritePlaylists.removeFavorite(r.id, "playlist");
                                    }
                                    favouritePage.chooseIndex = [];
                                    Style.warned("已取消收藏 " + total + " 个歌单", 1);
                                }
                            );
                            break;
                        default:
                            break;
                        }
                    }
                }
                QButton {
                    shadowEnabled: false
                    height: 36
                    radius: 20
                    borderWidth: 1
                    text: "加入播放列表"
                    onClicked: {
                        switch(favouritePage.setMode) {
                        case 1:
                            for(let a = 0;a < favouritePage.chooseIndex.length;a++) {
                                const fav = songSort.at(favouritePage.chooseIndex[a]);
                                if (!fav)
                                    continue;
                                if (Options.queue.indexOfPath(fav.id) === -1) {
                                    Options.queue.append({ name: fav.title, path: fav.id, songer: fav.artist, source: fav.source });
                                    Options.warn.tiped("已加入播放列表", 1);
                                }
                            }
                            break;
                        default:
                            break;
                        }
                    }
                }
                QButton {
                    shadowEnabled: false
                    width: 92
                    height: 36
                    radius: 20
                    borderWidth: 1
                    buttonColor: Style.themeColor
                    textColor: Style.primaryColor
                    text: "完成"
                    onClicked: {
                        favouritePage.setMode = 0;
                        favouritePage.chooseIndex = [];
                    }
                }
            }
        }
    }

    PlayListWindow {
        id: playListSongsWindow
        mainTarget: favouriteChildPage
        winIndex: 1
        content: Item {

            QListView {
                id: playListsView
                x: 24
                y: 184
                width: playListSongsWindow.width - 32
                height: playListSongsWindow.height - 184
                model: MusicApi.playlistSong
                clip: true
                topMargin: 8
                bottomMargin: 24

                onClicked: (index) => {
                    if(Options.settings.soundQuality === 0) {
                        MusicApi.getMusicInfo(model.get(index).hash,0,playListSongsWindow.songSource);
                    } else if(Options.settings.soundQuality === 1) {
                        MusicApi.getMusicInfo(model.get(index).hashhq,0,playListSongsWindow.songSource);
                    } else {
                        MusicApi.getMusicInfo(model.get(index).hashsq,0,playListSongsWindow.songSource);
                    }
                }

                onEnded: {
                    if(MusicApi.playlistSong.count % 20 === 0 && MusicApi.playlistSong.count !== 0) {
                        const tagid = playListSongsWindow.id;
                        MusicApi.getPlaylistSongs(tagid,MusicApi.playlistSong.count / 20 + 1,20,playListSongsWindow.songSource);
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
