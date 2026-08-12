// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
import QtQuick
import QueMusic 1.0
import 'qrc:/QueMusic/components'

Item {
    id: searchPage

    property int searchTab: 0


    QPages {
        x: 24
        y: 68
        width: parent.width - 48
        height: parent.height - 68
        id: searchChildPage
        pageList: [searchSong,searchLists,searchAlbum,searchLyrics]

        // 顶部常显示栏
        // 顶部标题
        Item {
            x: 0
            y: -44
            height: 40
            width: parent.width
            z: 10
            Text {
                x: 0
                y: 0
                height: 40
                verticalAlignment: Text.AlignVCenter
                text: "搜索结果"
                font.pixelSize: Style.settings.pageTitle
                color: Style.themes.fontColor
                QLoadSign {
                    id: searchLoad
                    x: parent.width
                    y: 2
                }
            }

            QDrop {
                x: parent.width - 128
                y: 2
                height: 36; width: 128
                //radius: Style.settings.noControlRadius ? Style.settings.labelRadius : 18
                anchors.right: parent.right
                choice: MusicApi.songSource
                model: ["酷狗音乐","网易云音乐","QQ音乐","自定义源"]
                onTransformed: (choiced) => {
                                   MusicApi.songSource = choiced
                               }
            }
        }

        QBlurTapBar {
            x: 0
            y: 12
            z: 5
            model: ["歌曲","歌单","专辑","歌词"]
            tabWidth: 80
            width: 324
            rectXy: Qt.rect(0, 12, width, 40)
            blurSource: searchChildPage.pageList[searchChildPage.lastIndex]
            onTabChange: (index) => {
                MusicApi.searchSongsResults.clear()
                searchChildPage.stack(index)
                MusicApi.nowIndex = index
                MusicApi.searchSongs(mainSearchInput.text,index,1,20)
            }
        }

        QListView {
            id: searchSong
            width: searchChildPage.width + 16
            height: searchChildPage.height
            model: MusicApi.searchSongsResults
            clip: true
            topMargin: 72

            footer: Item {
                height: 60
                width: searchSong.width
                QButton {
                    anchors.centerIn: parent
                    height: 40; width: 120
                    radius: 20
                    iconCharacter: "\uf0f8"
                    text: "更多"
                    onClicked: {
                        if(!MusicApi.loadState && MusicApi.searchSongsResults.count % 20 === 0) {
                            MusicApi.searchSongs(mainSearchInput.text,0,MusicApi.searchSongsResults.count / 20 + 1,20)
                        } else {
                            mainWarn.tiped("没有更多了",0);
                        }
                    }
                }
            }
            onClicked: (index) => {
                if(Options.settings.soundQuality === 0) {
                    MusicApi.getMusicInfo(model.get(index).hash);
                } else if(Options.settings.soundQuality === 1) {
                    MusicApi.getMusicInfo(model.get(index).hashhq);
                } else {
                    MusicApi.getMusicInfo(model.get(index).hashsq);
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
                        playListModel.append({ name: model.get(index).title, path: model.get(index).hash, songer: model.get(index).artist, source: MusicApi.songSource });
                        mainWarn.tiped("成功加入播放列表",1);
                    }
                    break;
                case 1:
                    if (favoritesSong.isFavorite(model.get(index).hash, "song")) {
                        favoritesSong.removeFavorite(model.get(index).hash, "song");
                        mainWarn.tiped("取消收藏",0);
                    } else {
                        favoritesSong.addFavorite(model.get(index).hash, model.get(index).title, model.get(index).artist, model.get(index).cover, MusicApi.songSource, model.get(index).duration, "song");
                        mainWarn.tiped("成功收藏",1);
                    }
                    break;
                }
            }
            onMenuClicked: (index,choice) => {
                switch(choice) {
                case 0:
                    if(Options.settings.soundQuality === 0) {
                        MusicApi.getMusicInfo(model.get(index).hash,1);
                    } else if(Options.settings.soundQuality === 1) {
                        MusicApi.getMusicInfo(model.get(index).hashhq,1);
                    } else {
                        MusicApi.getMusicInfo(model.get(index).hashsq,1);
                    }
                    break;
                }
            }
        }
        QListView {
            id: searchLists
            width: searchChildPage.width + 16
            height: searchChildPage.height
            model: MusicApi.searchSongsResults
            clip: true
            visible: false
            topMargin: 72
            bottomMargin: 24
            isList: true
            
            footer: Item {
                height: 60
                width: searchLists.width
                QButton {
                    anchors.centerIn: parent
                    height: 40; width: 120
                    radius: 20
                    iconCharacter: "\uf0f8"
                    text: "更多"
                    onClicked: {
                        if(!MusicApi.loadState && MusicApi.searchSongsResults.count % 10 === 0) {
                            MusicApi.searchSongs(mainSearchInput.text,1,Math.floor(MusicApi.searchSongsResults.count / 20) + 1,20)
                        } else {
                            mainWarn.tiped("没有更多了",0);
                        }
                    }
                }
            }

            onClicked: (index) => {
                MusicApi.playlistSong.clear();
                MusicApi.globalid = model.get(index).hash;
                MusicApi.getPlaylistSongs(model.get(index).hash,1,20);
                //var image = model.get(index).cover.replace("{size}", "256") || "qrc:/QueMusic/resources/app/musicpic.png";
                //var title = model.get(index).title;
                playListSongsWindow.opened(model.get(index));
                window.exitIndex = 2;
            }
            onToolClicked: (index,tool) => {
                switch(tool) {
                case 1:
                    if (favoritesList.isFavorite(model.get(index).hash, "playlist")) {
                        favoritesList.removeFavorite(model.get(index).hash, "playlist");
                        mainWarn.tiped("取消收藏",0);
                    } else {
                        favoritesList.addFavorite(model.get(index).hash, model.get(index).title, model.get(index).artist, model.get(index).cover, MusicApi.songSource, model.get(index).duration, "playlist");
                        mainWarn.tiped("成功收藏",1);
                    }
                    break;
                }
            }
        }
        QListView {
            id: searchAlbum
            width: searchChildPage.width + 16
            height: searchChildPage.height
            model: MusicApi.searchSongsResults
            clip: true
            visible: false
            topMargin: 72
            bottomMargin: 24
            isList: true

            footer: Item {
                height: 60
                width: searchAlbum.width
                QButton {
                    anchors.centerIn: parent
                    height: 40; width: 120
                    radius: 20
                    iconCharacter: "\uf0f8"
                    text: "更多"
                    onClicked: {
                        if(!MusicApi.loadState && MusicApi.searchSongsResults.count % 20 === 0) {
                            MusicApi.searchSongs(mainSearchInput.text,2,Math.floor(MusicApi.searchSongsResults.count / 20) + 1,20)
                        } else {
                            mainWarn.tiped("没有更多了",0);
                        }
                    }
                }
            }
            onClicked: (index) => {
                MusicApi.getMusicInfo(model.get(index).hash);
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
                        playListModel.append({ name: model.get(index).title, path: model.get(index).hash, songer: model.get(index).artist, source: MusicApi.songSource });
                        mainWarn.tiped("成功加入播放列表",1);
                    }
                    break;
                }
            }
        }
        QListView {
            id: searchLyrics
            width: searchChildPage.width + 16
            height: searchChildPage.height
            model: MusicApi.searchSongsResults
            clip: true
            visible: false
            topMargin: 72
            bottomMargin: 24

            footer: Item {
                height: 60
                width: searchLyrics.width
                QButton {
                    anchors.centerIn: parent
                    height: 40; width: 120
                    radius: 20
                    iconCharacter: "\uf0f8"
                    text: "更多"
                    onClicked: {
                        if(!MusicApi.loadState && MusicApi.searchSongsResults.count % 20 === 0) {
                            MusicApi.searchSongs(mainSearchInput.text,3,Math.floor(MusicApi.searchSongsResults.count / 20) + 1,20)
                        } else {
                            mainWarn.tiped("没有更多了",0);
                        }
                    }
                }
            }
            onClicked: (index) => {
                if(Options.settings.soundQuality === 0) {
                    MusicApi.getMusicInfo(model.get(index).hash);
                } else if(Options.settings.soundQuality === 1) {
                    MusicApi.getMusicInfo(model.get(index).hashhq);
                } else {
                    MusicApi.getMusicInfo(model.get(index).hashsq);
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
                        playListModel.append({ name: model.get(index).title, path: model.get(index).hash, songer: model.get(index).artist, source: MusicApi.songSource });
                        mainWarn.tiped("成功加入播放列表",1);
                    }
                    break;
                case 1:
                    if (favoritesSong.isFavorite(model.get(index).hash, "song")) {
                        favoritesSong.removeFavorite(model.get(index).hash, "song");
                        mainWarn.tiped("取消收藏",0);
                    } else {
                        favoritesSong.addFavorite(model.get(index).hash, model.get(index).title, model.get(index).artist, model.get(index).cover, MusicApi.songSource, model.get(index).duration, "song");
                        mainWarn.tiped("成功收藏",1);
                    }
                    break;
                }
            }
            onMenuClicked: (index,choice) => {
                switch(choice) {
                case 0:
                    if(Options.settings.soundQuality === 0) {
                        MusicApi.getMusicInfo(model.get(index).hash,1);
                    } else if(Options.settings.soundQuality === 1) {
                        MusicApi.getMusicInfo(model.get(index).hashhq,1);
                    } else {
                        MusicApi.getMusicInfo(model.get(index).hashsq,1);
                    }
                    break;
                }
            }
        }
    }

    PlayListWindow {
        id: playListSongsWindow
        mainTarget: searchChildPage
        winIndex: 2
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
                        MusicApi.getMusicInfo(model.get(index).hash);
                    } else if(Options.settings.soundQuality === 1) {
                        MusicApi.getMusicInfo(model.get(index).hashhq);
                    } else {
                        MusicApi.getMusicInfo(model.get(index).hashsq);
                    }
                }
                onToolClicked: (index,tool) => {
                    switch(tool) {
                    case 0:
                        playListModel.append({ name: model.get(index).title, path: model.get(index).hash, songer: model.get(index).artist, source: MusicApi.songSource });
                        mainWarn.tiped("成功加入播放列表",1);
                        break;
                    case 1:
                        if (favoritesSong.isFavorite(model.get(index).hash, "song")) {
                            favoritesSong.removeFavorite(model.get(index).hash, "song");
                            mainWarn.tiped("取消收藏",0);
                        } else {
                            favoritesSong.addFavorite(model.get(index).hash, model.get(index).title, model.get(index).artist, model.get(index).cover, MusicApi.songSource, model.get(index).duration, "song");
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
                            if(!MusicApi.loadState && MusicApi.playlistSong.count % 10 === 0) {
                                var tagid = playListSongsWindow.id
                                if(MusicApi.songSource === 1) {
                                    MusicApi.getPlaylistSongs(tagid,MusicApi.playlistSong.count / 10 + 1,10);
                                } else {
                                    MusicApi.getPlaylistSongs(tagid,MusicApi.playlistSong.count / 20 + 1,20);
                                }
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
