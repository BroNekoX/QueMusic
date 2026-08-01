// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024-2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QueMusic 1.0
import 'qrc:/QueMusic/components'

Item {
    id: favouritePage

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
                font.pixelSize: Style.settings.pageTitle
                color: Style.themes.fontColor
            }
        }

        QBlurTapBar {
            x: 0
            y: 10
            z: 5
            model: ["歌曲","歌单","关注歌手","历史记录"]
            tabWidth: 90
            width: 364
            rectXy: Qt.rect(0, 0, width, 40)
            blurSource: favouriteChildPage.pageList[favouriteChildPage.lastIndex]
            onTabChange: (index) => {
                favouriteChildPage.stack(index)
            }
        }

        QListView {
            id: songs
            width: favouriteChildPage.width + 16
            height: favouriteChildPage.height
            model: favoritesSong
            clip: true
            topMargin: 72

            onClicked: (index) => {
                MusicApi.getMusicInfo(model.get(index).id,0,model.get(index).source);
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
                        playListModel.append({ name: model.get(index).title, path: model.get(index).id, songer: model.get(index).artist, source: playListSongsWindow.songSource });
                        mainWarn.tiped("成功加入播放列表",1);
                    }
                }
            }
            onMenuClicked: (index,choice) => {
                switch(choice) {
                case 0:
                    MusicApi.getMusicInfo(model.get(index).id,1,model.get(index).source);
                    break;
                }
            }
        }
        QListView {
            id: lists
            width: favouriteChildPage.width + 16
            height: favouriteChildPage.height
            model: favoritesList
            clip: true
            topMargin: 72
            visible: false

            onClicked: (index) => {
                MusicApi.playlistSong.clear();
                MusicApi.globalid = model.get(index).id;
                MusicApi.getPlaylistSongs(model.get(index).id,1,20,model.get(index).source);
                playListSongsWindow.songSource = model.get(index).source;
                playListSongsWindow.opened(model.get(index));
                window.exitIndex = 1;
            }
            onToolClicked: (index,tool) => {
                switch(tool) {
                }
            }
        }
        Item {
            id: singer
            visible: false
            width: favouriteChildPage.width
            height: favouriteChildPage.height
            Text {
                anchors.fill: parent
                text: "喜欢的歌手"
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
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
                anchors.fill: parent
                text: "历史记录"
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                color: Style.themes.textColor
                font.pixelSize: 14
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
                                    MusicApi.getPlaylistSongs(tagid,Math.floor(MusicApi.playlistSong.count / 20) + 1,20,playListSongsWindow.songSource);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

}
