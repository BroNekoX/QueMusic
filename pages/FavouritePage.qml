// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QueMusic 1.0
import 'qrc:/QueMusic/components'

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
        model: favoritesSong
        options: favouritePage.favSortOptions
        nameRole: "title"
        timeRole: "createdAt"
        timeDesc: true
    }

    QSortModel {
        id: listSort
        model: favoritesList
        options: favouritePage.favSortOptions
        nameRole: "title"
        timeRole: "createdAt"
        timeDesc: true
    }

    QMenu {
        id: sortMenu
        model: favSortOptions.map(o => o.label)
        blurSource: null
        masked: true
        current: favouriteChildPage.lastIndex === 0 ? songSort.menuIndex : listSort.menuIndex
        onClicked: (i) => {
            var target = favouriteChildPage.lastIndex === 0 ? songSort : listSort;
            target.selectMenu(i);
            favouritePage.setMode = 0;
            favouritePage.chooseIndex = [];
            songs.scrollTop();
            lists.scrollTop();
        }
    }

    function chooseTotal() {
        return favouriteChildPage.lastIndex === 0 ? favoritesSong.count : favoritesList.count;
    }

    function isAllChosen() {
        var total = favouritePage.chooseTotal();
        return total > 0 && favouritePage.chooseIndex.length >= total;
    }

    function toggleAllChoose() {
        var total = favouritePage.chooseTotal();
        if (total === 0) return;
        if (favouritePage.isAllChosen()) {
            favouritePage.chooseIndex = [];
            return;
        }
        var all = [];
        for (var i = 0; i < total; i++) all.push(i);
        favouritePage.chooseIndex = all;
    }

    QPages {
        id: favouriteChildPage
        x: 24
        y: 68
        width: parent.width - 48
        height: parent.height - 68
        pageList: [songs,lists,local,singer,history]

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
                color: Style.themes.fontColor
            }
        }

        QBlurTapBar {
            x: 0
            y: 12
            z: 5
            model: ["歌曲","歌单","本地","歌手","历史"]
            tabWidth: 75
            width: 381
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
                buttonColor: Style.themes.primaryColor
                onClicked: sortMenu.popup(sortBtn, 0, sortBtn.height + 6)
            }
            QButton {
                visible: favouriteChildPage.lastIndex === 4
                height: 38
                text: "清空历史"
                iconCharacter: "\uf08e"
                onClicked: globalDialog.openSimpleDialog("清空", "将清空全部播放历史，是否继续？",
                    function() {
                        Playback.clearHistory()
                        mainWarn.tiped("已清空播放历史", 1)
                    })
            }
            QButton {
                height: 38
                text: favouritePage.setMode === 1 ? "取消选择" : "选择"
                iconCharacter: "\uf09f"
                buttonColor: favouritePage.setMode === 1 ? Style.themes.containColor : Style.themes.primaryColor
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

        QListView {
            id: songs
            width: favouriteChildPage.width + 16
            height: favouriteChildPage.height
            model: songSort
            clip: true
            topMargin: 72
            selectedIndices: favouritePage.chooseIndex

            onClicked: (index) => {
                if (favouritePage.setMode === 1) {
                    var idx = favouritePage.chooseIndex.indexOf(index);
                    if (idx === -1) {
                        favouritePage.chooseIndex = favouritePage.chooseIndex.concat([index]);
                    } else {
                        favouritePage.chooseIndex = favouritePage.chooseIndex.filter(v => v !== index);
                    }
                } else {
                    var r = songSort.at(index);
                    MusicApi.getMusicInfo(r.id, 0, r.source);
                }
            }
            onToolClicked: (index,tool) => {
                var r = songSort.at(index);
                switch(tool) {
                case 0:
                    var listIndex = -1;
                    for(var i = 0;i < playListModel.count;i++) {
                        if(playListModel.get(i).path === r.id) {
                            listIndex = i;
                        }
                    }
                    if (listIndex == -1) {
                        playListModel.append({ name: r.title, path: r.id, songer: r.artist, source: r.source });
                        mainWarn.tiped("成功加入播放列表",1);
                    }
                    break;
                case 1:
                    favoritesSong.removeFavorite(r.id, "song");
                    mainWarn.tiped("取消收藏",0);
                }
            }
            onMenuClicked: (index,choice) => {
                var r = songSort.at(index);
                switch(choice) {
                case 0:
                    MusicApi.getMusicInfo(r.id,1,r.source);
                    break;
                }
            }
            Text {
                anchors.centerIn: parent
                visible: favoritesSong.count === 0
                text: "没有收藏的内容？快去收藏一些歌曲吧"
                color: Style.themes.textColor
                font.pixelSize: 14
            }
        }
        QListView {
            id: lists
            width: favouriteChildPage.width + 16
            height: favouriteChildPage.height
            model: listSort
            clip: true
            isList: true
            topMargin: 72
            visible: false
            selectedIndices: favouritePage.chooseIndex

            onClicked: (index) => {
                if (favouritePage.setMode === 1) {
                    var idx = favouritePage.chooseIndex.indexOf(index);
                    if (idx === -1) {
                        favouritePage.chooseIndex = favouritePage.chooseIndex.concat([index]);
                    } else {
                        favouritePage.chooseIndex = favouritePage.chooseIndex.filter(v => v !== index);
                    }
                } else {
                    var r = listSort.at(index);
                    MusicApi.playlistSong.clear();
                    MusicApi.globalid = r.id;
                    MusicApi.getPlaylistSongs(r.id,1,20,r.source);
                    playListSongsWindow.songSource = r.source;
                    playListSongsWindow.opened({ id: r.id, title: r.title, artist: r.artist, cover: r.cover, duration: r.duration });
                    window.exitIndex = 1;
                }
            }
            onToolClicked: (index,tool) => {
                switch(tool) {
                case 1:
                    favoritesList.removeFavorite(listSort.at(index).id, "playlist");
                    mainWarn.tiped("取消收藏",0);
                }
            }
            Text {
                anchors.centerIn: parent
                visible: favoritesList.count === 0
                text: "没有收藏的内容？快去收藏一些歌单吧"
                color: Style.themes.textColor
                font.pixelSize: 14
            }
        }
        Item {
            id: local
            visible: false
            width: favouriteChildPage.width
            height: favouriteChildPage.height
            Text {
                anchors.centerIn: parent
                text: "本地收藏"
                color: Style.themes.textColor
                font.pixelSize: 14
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
                color: Style.themes.textColor
                font.pixelSize: 14
            }
        }
        QListView {
            id: history
            visible: false
            width: favouriteChildPage.width + 16
            height: favouriteChildPage.height
            model: Playback.history
            clip: true
            topMargin: 72
            menuModel: ["加入播放列表", "收藏", "移除记录"]
            toolText0: "\uf095"
            toolText1: "\uf0c8"

            function addToQueue(e) {
                for (var i = 0; i < playListModel.count; i++)
                    if (playListModel.get(i).path === e.path) return
                playListModel.append({ name: e.title, path: e.path, songer: e.artist, source: e.source })
                mainWarn.tiped("成功加入播放列表", 1)
            }
            function toggleFav(e) {
                if (e.source === -1) { mainWarn.tiped("本地歌曲请使用本地收藏", 0); return }
                if (favoritesSong.isFavorite(e.path, "song")) {
                    favoritesSong.removeFavorite(e.path, "song")
                    mainWarn.tiped("取消收藏", 0)
                } else {
                    favoritesSong.addFavorite(e.path, e.title, e.artist, e.cover, e.source, e.duration, "song")
                    mainWarn.tiped("成功收藏", 1)
                }
            }

            onClicked: (index) => {
                var e = Playback.history.get(index)
                Playback.playItem({ name: e.title, path: e.path, songer: e.artist, source: e.source })
            }
            onToolClicked: (index, tool) => {
                var e = Playback.history.get(index)
                if (tool === 0) history.addToQueue(e)
                else history.toggleFav(e)
            }
            onMenuClicked: (index, choice) => {
                var e = Playback.history.get(index)
                if (choice === 0) history.addToQueue(e)
                else if (choice === 1) history.toggleFav(e)
                else Playback.history.remove(index, 1)
            }
            Text {
                anchors.centerIn: parent
                visible: Playback.history.count === 0
                text: "还没有播放记录，去听点什么吧"
                color: Style.themes.textColor
                font.pixelSize: 14
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
            //color: Style.themes.sideColor
            gradient: Gradient {
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 1.0; color: Style.themes.sideColor }
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
                buttonColor: favouritePage.isAllChosen() ? Style.themes.themeColor : Style.themes.fullColor
                textColor: favouritePage.isAllChosen() ? Style.themes.primaryColor : Style.themes.fontColor
                text: "全选"
                onClicked: favouritePage.toggleAllChoose()
            }
            Rectangle {
                x: 118
                y: 12
                width: 92
                height: 36
                radius: 20
                color: "transparent"//Style.themes.fullColor
                Text {
                    anchors.centerIn: parent
                    text: "已选择:" + favouritePage.chooseIndex.length + "项"
                    color: Style.themes.textColor
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
                            globalDialog.openSimpleDialog("取消收藏", "这将取消收藏这些歌曲",
                                function() {
                                    for(var i=0;i<favouritePage.chooseIndex.length;i++) {
                                        favoritesSong.removeFavorite(songSort.at(favouritePage.chooseIndex[i]).id, "song");
                                    }
                                    favouritePage.chooseIndex = [];
                                    Style.warned("成功取消" + favouritePage.chooseIndex.length + "个收藏歌曲",1);
                                }
                            );
                            break;
                        case 2:
                            globalDialog.openSimpleDialog("取消收藏", "这将取消收藏这些歌单",
                                function() {
                                    for(var i=0;i<favouritePage.chooseIndex.length;i++) {
                                        favoritesList.removeFavorite(listSort.at(favouritePage.chooseIndex[i]).id, "playlist");
                                    }
                                    favouritePage.chooseIndex = [];
                                    Style.warned("成功取消" + favouritePage.chooseIndex.length + "个收藏歌单",1);
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
                            var playlist = [];
                            for(var i = 0;i < playListModel.count;i++) {
                                playlist.push(playListModel.get(i).path);
                            }
                            for(var a = 0;a < favouritePage.chooseIndex.length;a++) {
                                var fav = songSort.at(favouritePage.chooseIndex[a]);
                                var listIndex = -1;
                                for(var b = 0;b < playlist.length;b++) {
                                    if(fav.id == playlist[b]) {
                                        listIndex = b;
                                        break;
                                    }
                                }
                                if (listIndex == -1) {
                                    playListModel.append({ name: fav.title, path: fav.id, songer: fav.artist, source: fav.source });
                                    mainWarn.tiped("成功加入播放列表",1);
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
                    buttonColor: Style.themes.themeColor
                    textColor: Style.themes.primaryColor
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
                //reuseItems: true
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
                onToolClicked: (index,tool) => {
                    switch(tool) {
                    case 0:
                        var listIndex = -1;
                        var indexHash = model.get(index).hash;
                        for(var i = 0;i < playListModel.count;i++) {
                            var forUrl = playListModel.get(i).path;
                            if(forUrl === indexHash) {
                               listIndex = i;
                            }
                        }
                        if (listIndex == -1) {
                            playListModel.append({ name: model.get(index).title, path: model.get(index).hash, songer: model.get(index).artist, source: playListSongsWindow.songSource });
                            mainWarn.tiped("成功加入播放列表",1);
                        }
                        break;
                    case 1:
                        if (favoritesSong.isFavorite(model.get(index).hash, "song")) {
                            favoritesSong.removeFavorite(model.get(index).hash, "song");
                            mainWarn.tiped("取消收藏",0);
                        } else {
                            favoritesSong.addFavorite(model.get(index).hash, model.get(index).title, model.get(index).artist, model.get(index).cover, playListSongsWindow.songSource, model.get(index).duration, "song");
                            mainWarn.tiped("成功收藏",1);
                        }
                        break;
                    }
                }

                onEnded: {
                    if(MusicApi.playlistSong.count % 20 === 0 && MusicApi.playlistSong.count !== 0) {
                        var tagid = playListSongsWindow.id;
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
