// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Effects
import QtQuick.Controls.Basic
import QueMusic 1.0
import 'qrc:/QueMusic/components'

Item {
    id: playlistPage
    //property alias animatedWindow: animationWrapper
    property real toolsWindow: 0
    //property bool displaytop: flickable.contentY > 60 ? true : false
    Component.onCompleted: {
        if(!window.completedStart.playlistLoaded) {
            MusicApi.getPlaylistMenu(3);
            MusicApi.getNewSongs(1, 1, 20);
            MusicApi.getAllToplist();
            window.completedStart.playlistLoaded = true;
        }
    }

    QPages {
        id: playlistChildPage
        x: 24
        y: 68
        width: parent.width - 48
        height: parent.height - 68
        pageList: [musicsPage,musicMenuPage,cloud,album]

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
                text: "分类"
                font.weight: Font.DemiBold
                font.pixelSize: Style.settings.pageTitle
                color: Style.themes.fontColor
            }
            QDrop {
                x: parent.width - 120
                y: 0
                height: 36; width: 120
                //radius: 18
                anchors.right: parent.right
                choice: MusicApi.songSource
                textColor: MusicApi.songSource == 0 ? "#0F3975" : MusicApi.songSource == 1 ? "#750F0F" : MusicApi.songSource == 2 ? "#16750F" : "#756F0F"
                color: MusicApi.songSource == 0 ? "#CDE8FF" : MusicApi.songSource == 1 ? "#FFCDCD" : MusicApi.songSource == 2 ? "#CDFFCD" : "#FFFFCD"
                border.color: MusicApi.songSource == 0 ? "#4384F5" : MusicApi.songSource == 1 ? "#F54343" : MusicApi.songSource == 2 ? "#4DF543" : "#F5F543"
                radius: 18
                cardRadius: Style.settings.labelRadius
                model: ["酷狗音乐","网易云音乐","QQ音乐(x)","自定义源(x)"]
                onTransformed: (choiced) => {
                    MusicApi.songSource = choiced;
                    MusicApi.newSongs.clear();
                    MusicApi.getPlaylistMenu(3);
                    MusicApi.getNewSongs(1, 1, 20);
                    MusicApi.getAllToplist();
                }
            }
        }

        QBlurTapBar {
            x: 0
            y: 12
            z: 12
            model: ["歌曲","歌单","排行榜","歌手"]
            tabWidth: 80
            width: 324
            rectXy: Qt.rect(0, 12, width, 40)
            blurSource: playlistChildPage.pageList[playlistChildPage.lastIndex]
            onTabChange: (index) => {
                playlistChildPage.stack(index)
                switch(index) {
                case 0:
                    break;
                case 1:
                    MusicApi.getMenuInfo(MusicApi.allPlaylistMenu[0].id)
                    break;
                case 2:
                    break;
                case 3:
                    break;
                }
            }
        }

        QCard {
            x: parent.width - 148
            y: 12
            z: 11
            padding: 2
            width: 148
            height: 40
            cardColor: Style.themes.primaryBlurColor
            radius: 20

            Row {
                anchors.fill: parent
                //  刷新
                SButton {
                    width: 36
                    height: 36
                    radius: 18
                    iconCharacter: "\uf11b"
                    buttonColor: "transparent"
                    hoverColor: Style.themes.hoverColor
                    onClicked: {
                        MusicApi.recommendSongs.clear()
                        MusicApi.getRecommendSongs(1,24)
                    }
                }
                //  布局
                SButton {
                    width: 36
                    height: 36
                    radius: 18
                    iconCharacter: "\uf0d4"
                    buttonColor: "transparent"
                    hoverColor: Style.themes.hoverColor
                    onClicked: {

                    }
                }
                //  排序
                SButton {
                    width: 36
                    height: 36
                    radius: 18
                    iconCharacter: "\uf10b"
                    buttonColor: "transparent"
                    hoverColor: Style.themes.hoverColor
                    onClicked: {

                    }
                }
                // 筛选
                SButton {
                    width: 36
                    height: 36
                    radius: 18
                    iconCharacter: "\uf101"
                    buttonColor: "transparent"
                    hoverColor: Style.themes.hoverColor
                    onClicked: {

                    }
                }
            }
        }

        Item {
            id: musicsPage
            width: playlistChildPage.width
            height: playlistChildPage.height
            property int musicMenuIndex: 0
            Row {
                spacing: 6
                y: 72
                Repeater {
                    model: ["华语","欧美","日韩","韩语","日语"]
                    delegate: Rectangle {
                        width: 64
                        height: 32
                        radius: 16
                        color: musicsPage.musicMenuIndex === index ? Style.themes.themeColor : Style.themes.primaryColor
                        border.color: Style.themes.sideColor
                        border.width: 1
                        Rectangle {
                            anchors.fill: parent
                            radius: 16
                            color: Style.themes.hoverColor
                            opacity: musicsMenuArea.containsMouse ? 1 : 0
                            z: 1
                            Behavior on opacity { NumberAnimation { duration: 80 } }
                        }

                        Text {
                            anchors.fill: parent
                            text: modelData
                            elide: Text.ElideRight
                            z: 2
                            font.pixelSize: Style.settings.text
                            color: musicsPage.musicMenuIndex === index ? Style.themes.fullColor : Style.themes.textColor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        MouseArea {
                            id: musicsMenuArea
                            hoverEnabled: true
                            anchors.fill: parent
                            onClicked: {
                                MusicApi.newSongs.clear();
                                MusicApi.globalid = index + 1;
                                musicsPage.musicMenuIndex = index;
                                MusicApi.getNewSongs(index + 1, 1, 20);
                            }
                        }
                    }
                }
            }
            QListView {
                id: searchSong
                width: parent.width + 16
                y: 104
                height: parent.height - 104
                model: MusicApi.newSongs
                clip: true
                //topMargin: 72

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
                            if(!MusicApi.loadState) {
                                var tagid = MusicApi.globalid;
                                if(MusicApi.newSongs.count % 20 === 0) {
                                    MusicApi.getNewSongs(tagid, MusicApi.newSongs.count / 20 + 1, 20);
                                }
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
                        var listIndex = -1
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
        Item {
            id: musicMenuPage
            visible: false
            width: playlistChildPage.width
            height: playlistChildPage.height
            property int musicMenuIndex: 0
            Flow {
                id: musicMenuFlow
                spacing: 6
                y: 72
                width: parent.width
                Repeater {
                    model: MusicApi.allPlaylistMenu
                    delegate: Rectangle {
                        width: 64
                        height: 32
                        radius: 16
                        color: musicMenuPage.musicMenuIndex === index ? Style.themes.themeColor : Style.themes.primaryColor
                        border.color: Style.themes.sideColor
                        border.width: 1
                        Rectangle {
                            anchors.fill: parent
                            radius: 16
                            color: Style.themes.hoverColor
                            opacity: musiclistMenuArea.containsMouse ? 1 : 0
                            z: 1
                            Behavior on opacity { NumberAnimation { duration: 80 } }
                        }

                        Text {
                            anchors.fill: parent
                            text: modelData.title
                            elide: Text.ElideRight
                            z: 2
                            font.pixelSize: Style.settings.text
                            color: musicMenuPage.musicMenuIndex === index ? Style.themes.fullColor : Style.themes.textColor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        MouseArea {
                            id: musiclistMenuArea
                            hoverEnabled: true
                            anchors.fill: parent
                            onClicked: {
                                musicMenuPage.musicMenuIndex = index
                                MusicApi.globaltagid = MusicApi.allPlaylistMenu[index].id
                                MusicApi.musicPlaylists.clear()
                                MusicApi.getMenuInfo(MusicApi.allPlaylistMenu[index].id)
                            }
                        }
                    }
                }
            }

            QListView {
                id: musicMenuList
                height: parent.height - y
                clip: true
                y: musicMenuFlow.implicitHeight + 80
                width: parent.width + 16
                model: MusicApi.musicPlaylists
                property int artistX: width / 2 - 50
                //topMargin: 72
                bottomMargin: 24

                footer: Item {
                    height: 60
                    width: musicMenuList.width
                    QButton {
                        anchors.centerIn: parent
                        height: 40; width: 120
                        radius: 20
                        iconCharacter: "\uf0f8"
                        text: "更多"
                        onClicked: {
                            if(!MusicApi.loadState && MusicApi.musicPlaylists.count % 20 === 0) {
                                MusicApi.getMusicPlaylists(MusicApi.globaltagid, MusicApi.musicPlaylists.count / 20 + 1, 20);
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
        }
        Item {
            id: cloud
            visible: false
            width: playlistChildPage.width
            height: playlistChildPage.height
            Component.onCompleted: {
                if(MusicApi.toplistList.count === 0)
                    MusicApi.getAllToplist();
            }

            QScrollView {
                id: toplistFlick
                width: parent.width + 24
                height: parent.height
                contentChildren: Flow {
                    id: toplistFlow
                    width: parent.width
                    padding: 12
                    topPadding: 68
                    bottomPadding: 24
                    //height: implicitHeight + 640
                    spacing: 20
                    Repeater {
                        model: MusicApi.toplistList
                        delegate: Rectangle {
                            width: 156
                            height: 216
                            radius: Style.settings.labelRadius
                            color: Style.themes.primaryColor
                            scale: toplistCardArea.containsMouse ? 1.04 : 1.0
                            Behavior on scale { NumberAnimation { duration: 200; easing.type: Easing.OutExpo } }
                            RectangularShadow {
                                anchors.fill: parent
                                z: -1
                                offset.x: 2
                                offset.y: 2
                                radius: Style.settings.labelRadius
                                blur: toplistCardArea.containsMouse ? 24 : 8
                                spread: 0
                                color: Style.themes.shadowColor
                                Behavior on blur { NumberAnimation { duration: 200 } }
                            }
                            QPicture {
                                width: 156
                                height: 156
                                source: model.cover.replace("{size}","128") || "qrc:/QueMusic/resources/app/musicpic.png"
                                radius: Style.settings.labelRadius
                                radius3: 0
                                radius4: 0
                                sourceSize: Qt.size(128,128)
                            }
                            // 平台徽标
                            Rectangle {
                                x: 8
                                y: 8
                                width: 50
                                height: 20
                                radius: 10
                                color: model.source === 0 ? "#CDE8FF" : "#FFCDCD"
                                Text {
                                    anchors.centerIn: parent
                                    text: model.source === 0 ? "酷狗" : "网易云"
                                    font.pixelSize: 11
                                    font.weight: Font.DemiBold
                                    color: model.source === 0 ? "#0F3975" : "#750F0F"
                                }
                            }
                            Text {
                                x: 12
                                y: 166
                                width: 132
                                text: model.title
                                font.bold: true
                                color: Style.themes.fontColor
                                font.pixelSize: Style.settings.textmain
                                elide: Text.ElideRight
                            }
                            Text {
                                x: 12
                                y: 190
                                width: 132
                                text: model.artist || ""
                                color: Style.themes.textColor
                                font.pixelSize: Style.settings.text
                                elide: Text.ElideRight
                            }
                            MouseArea {
                                id: toplistCardArea
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: {
                                    MusicApi.playlistSong.clear();
                                    MusicApi.globalid = model.hash;
                                    playListSongsWindow.listType = "toplist";
                                    MusicApi.getMusicToplist(1, 20, Number(model.hash), model.source);
                                    playListSongsWindow.opened(model);
                                    window.exitIndex = 2;
                                }
                            }
                        }
                    }
                }
            }
        }
        Item {
            id: album
            visible: false
            width: playlistChildPage.width
            height: playlistChildPage.height
            property int singerTypeIndex: 0
            property int singerPage: 1
            // 歌手类型入口：area 按平台映射（酷狗 / 网易云）
            property var singerTypes: [
                { title: "华语", kg: 1, ne: 7 },
                { title: "欧美", kg: 2, ne: 96 },
                { title: "日本", kg: 5, ne: 8 },
                { title: "韩国", kg: 4, ne: 16 },
                { title: "热门歌手", kg: 3, ne: 0 }
            ]
            function loadSingers(typeIndex) {
                singerTypeIndex = typeIndex;
                singerPage = 1;
                MusicApi.singerList.clear();
                var area = MusicApi.songSource === 0 ? singerTypes[typeIndex].kg : singerTypes[typeIndex].ne;
                if(area === 0) {
                    MusicApi.getHotSingers(1, 30, MusicApi.songSource);
                } else {
                    MusicApi.getSingerCategory(area, 1, 30, MusicApi.songSource);
                }
            }
            Component.onCompleted: {
                loadSingers(0);
            }
            // 歌手类型标签
            Flow {
                id: singerTypeFlow
                y: 72
                spacing: 8
                width: parent.width
                Repeater {
                    model: album.singerTypes
                    delegate: Rectangle {
                        width: 76
                        height: 32
                        radius: 16
                        color: album.singerTypeIndex === index ? Style.themes.themeColor : Style.themes.primaryColor
                        border.color: Style.themes.sideColor
                        border.width: 1
                        Rectangle {
                            anchors.fill: parent
                            radius: 16
                            color: Style.themes.hoverColor
                            opacity: singerTypeArea.containsMouse ? 1 : 0
                            z: 1
                            Behavior on opacity { NumberAnimation { duration: 80 } }
                        }
                        Text {
                            anchors.fill: parent
                            text: modelData.title
                            elide: Text.ElideRight
                            z: 2
                            font.pixelSize: Style.settings.text
                            color: album.singerTypeIndex === index ? Style.themes.fullColor : Style.themes.textColor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        MouseArea {
                            id: singerTypeArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: album.loadSingers(index)
                        }
                    }
                }
            }
            // 歌手网格（可滚动 + 分页）
            Flickable {
                id: singerFlick
                y: 104
                width: parent.width
                height: parent.height - 152
                clip: true
                contentWidth: width
                contentHeight: singerColumn.implicitHeight + 24
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar {}
                Column {
                    id: singerColumn
                    width: parent.width
                    Flow {
                        width: parent.width
                        spacing: 20
                        Repeater {
                            model: MusicApi.singerList
                            delegate: Item {
                                width: 96
                                height: 132
                                scale: singerArea.containsMouse ? 1.06 : 1.0
                                Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutExpo } }
                                QPicture {
                                    width: 96
                                    height: 96
                                    radius: 48
                                    source: model.cover.replace("{size}","128") || "qrc:/QueMusic/resources/app/musicpic.png"
                                }
                                Text {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    y: 100
                                    width: parent.width
                                    text: model.title
                                    elide: Text.ElideRight
                                    horizontalAlignment: Text.AlignHCenter
                                    font.pixelSize: Style.settings.text
                                    color: Style.themes.textColor
                                }
                                MouseArea {
                                    id: singerArea
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    onClicked: {
                                        MusicApi.playlistSong.clear();
                                        MusicApi.globalid = model.hash;
                                        playListSongsWindow.listType = "singer";
                                        MusicApi.getSingerSongs(model.hash, 1, 20, MusicApi.songSource);
                                        playListSongsWindow.opened(model);
                                        window.exitIndex = 2;
                                    }
                                }
                            }
                        }
                    }
                    Item {
                        width: parent.width
                        height: 60
                        QButton {
                            anchors.centerIn: parent
                            height: 40
                            width: 120
                            radius: 20
                            iconCharacter: "\uf0f8"
                            text: "更多"
                            onClicked: {
                                if(MusicApi.loadState) return;
                                if(MusicApi.singerList.count % 30 !== 0) {
                                    mainWarn.tiped("没有更多了",0);
                                    return;
                                }
                                album.singerPage += 1;
                                var area = MusicApi.songSource === 0
                                    ? album.singerTypes[album.singerTypeIndex].kg
                                    : album.singerTypes[album.singerTypeIndex].ne;
                                if(area === 0) {
                                    MusicApi.getHotSingers(album.singerPage, 30, MusicApi.songSource);
                                } else {
                                    MusicApi.getSingerCategory(area, album.singerPage, 30, MusicApi.songSource);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // 歌单/榜单/歌手歌曲共用窗口
    PlayListWindow {
        id: playListSongsWindow
        mainTarget: playlistChildPage
        winIndex: 2
        property string listType: "playlist"   // playlist 歌单 / singer 歌手 / toplist 榜单
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
                            if(MusicApi.loadState) return;
                            var id = MusicApi.globalid;
                            var page = MusicApi.playlistSong.count / 20 + 1;
                            if(playListSongsWindow.listType === "singer") {
                                if(MusicApi.playlistSong.count % 20 === 0)
                                    MusicApi.getSingerSongs(id, page, 20, MusicApi.songSource);
                                else
                                    mainWarn.tiped("没有更多了",0);
                            } else if(playListSongsWindow.listType === "toplist") {
                                if(MusicApi.playlistSong.count % 20 === 0)
                                    MusicApi.getMusicToplist(page, 20, id, MusicApi.songSource);
                                else
                                    mainWarn.tiped("没有更多了",0);
                            } else {
                                if(MusicApi.playlistSong.count % 20 === 0)
                                    MusicApi.getPlaylistSongs(id, page, 20, MusicApi.songSource);
                                else
                                    mainWarn.tiped("没有更多了",0);
                            }
                        }
                    }
                }
            }
        }
    }
}