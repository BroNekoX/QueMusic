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

    QPages {
        id: favouriteChildPage
        x: 24
        y: 68
        width: parent.width - 48
        height: parent.height - 68
        pageList: [songs,lists,singer,history]

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
            model: ["歌曲","歌单","关注歌手","历史记录"]
            tabWidth: 90
            width: 364
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
                height: 38
                text: favouritePage.setMode === 1 ? "取消选择" : "选择"
                iconCharacter: "\uf09f"
                buttonColor: favouritePage.setMode === 1 ? Style.themes.containColor : Style.themes.fullColor
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
            model: favoritesSong
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
                    MusicApi.getMusicInfo(model.get(index).id, 0, model.get(index).source);
                }
            }
            onToolClicked: (index,tool) => {
                switch(tool) {
                case 0:
                    var listIndex = -1;
                    var indexHash = model.get(index).id;
                    for(var i = 0;i < playListModel.count;i++) {
                        var forUrl = playListModel.get(i).path;
                        if(forUrl === indexHash) {
                            listIndex = i;
                        }
                    }
                    if (listIndex == -1) {
                        playListModel.append({ name: model.get(index).title, path: model.get(index).id, songer: model.get(index).artist, source: model.get(index).source });
                        mainWarn.tiped("成功加入播放列表",1);
                    }
                    break;
                case 1:
                    favoritesSong.removeFavorite(model.get(index).id, "song");
                    mainWarn.tiped("取消收藏",0);
                }
            }
            onMenuClicked: (index,choice) => {
                switch(choice) {
                case 0:
                    MusicApi.getMusicInfo(model.get(index).id,1,model.get(index).source);
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
            model: favoritesList
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
                    MusicApi.playlistSong.clear();
                    MusicApi.globalid = model.get(index).id;
                    MusicApi.getPlaylistSongs(model.get(index).id,1,20,model.get(index).source);
                    playListSongsWindow.songSource = model.get(index).source;
                    playListSongsWindow.opened(model.get(index));
                    window.exitIndex = 1;
                }
            }
            onToolClicked: (index,tool) => {
                switch(tool) {
                case 1:
                    favoritesList.removeFavorite(model.get(index).id, "playlist");
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
        Item {
            id: history
            visible: false
            width: favouriteChildPage.width
            height: favouriteChildPage.height
            Text {
                anchors.centerIn: parent
                text: "历史记录"
                color: Style.themes.textColor
                font.pixelSize: 14
            }
        }

        // 选择模式
        Rectangle {
            id: chooseArea
            x: -24
            y: visible ? favouriteChildPage.height - 60 : favouriteChildPage.height
            width: favouritePage.width
            height: 60
            visible: favouritePage.setMode !== 0
            color: Style.themes.sideColor
            Behavior on y { NumberAnimation { duration: 420; easing.type: Easing.OutExpo } }
            Rectangle {
                x: 16
                y: 12
                width: 92
                height: 36
                radius: 20
                color: Style.themes.fullColor
                Text {
                    anchors.centerIn: parent
                    text: "多选模式"
                    color: Style.themes.textColor
                    font.pixelSize: Style.settings.textmain
                }
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
                    buttonColor: "#fa4642"
                    text: "取消收藏"
                    onClicked: {
                        switch(favouritePage.setMode) {
                        case 1:
                            globalDialog.openSimpleDialog("取消收藏", "这将取消收藏这些歌曲",
                                function() {
                                    for(var i=0;i<favouritePage.chooseIndex.length;i++) {
                                        favoritesSong.removeFavorite(favoritesSong.get(favouritePage.chooseIndex[i]).id, "song");
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
                                        favoritesList.removeFavorite(favoritesList.get(favouritePage.chooseIndex[i]).id, "playlist");
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
                    text: "加入播放列表"
                    onClicked: {
                        switch(favouritePage.setMode) {
                        case 1:
                            var playlist = [];
                            for(var i = 0;i < playListModel.count;i++) {
                                playlist.push(playListModel.get(i).path);
                            }
                            for(var a = 0;a < favouritePage.chooseIndex.length;a++) {
                                var listIndex = -1;
                                for(var b = 0;b < playlist.length;b++) {
                                    if(favoritesSong.get(favouritePage.chooseIndex[a]).id == playlist[b]) {
                                        listIndex = b;
                                        break;
                                    }
                                }
                                if (listIndex == -1) {
                                    playListModel.append({ name: favoritesSong.get(favouritePage.chooseIndex[a]).title, path: favoritesSong.get(favouritePage.chooseIndex[a]).id, songer: favoritesSong.get(favouritePage.chooseIndex[a]).artist, source: playListSongsWindow.songSource });
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

                footer: Item {
                    height: 60
                    width: playListsView.width
                    QButton {
                        anchors.centerIn: parent
                        height: 40; width: 120
                        radius: 20
                        iconCharacter: "\uf0f8"
                        text: "更多"
                        onClicked: {
                            if(!MusicApi.loadState) {
                                var tagid = playListSongsWindow.id
                                if(MusicApi.songSource === 1) {
                                    MusicApi.getPlaylistSongs(tagid,Math.floor(MusicApi.playlistSong.count / 10) + 1,10,playListSongsWindow.songSource);
                                } else {
                                    if(MusicApi.playlistSong.count % 20 === 0) {
                                        MusicApi.getPlaylistSongs(tagid,MusicApi.playlistSong.count / 20 + 1,20,playListSongsWindow.songSource);
                                    } else {
                                        mainWarn.tiped("没有更多了",0);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

}
