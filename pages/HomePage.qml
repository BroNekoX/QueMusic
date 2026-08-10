// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Effects
import QueMusic 1.0
import 'qrc:/QueMusic/components'

Item {
    id: homePage
    //property alias animatedWindow: animationWrapper
    property real toolsWindow: 0
    //property bool displaytop: flickable.contentY > 60 ? true : false


    Component.onCompleted: {
        if(!window.completedStart.homeLoaded) {
            MusicApi.getHotlistMenu.clear();
            MusicApi.getHotPlaylistMenu(3);
            MusicApi.getHotPlaylists(1);
            var date = new Date();
            var timeHour = date.getHours();
            if(timeHour > 3 && timeHour < 9) {
                homeText.text = "早上好"
            } else if(timeHour > 8 && timeHour < 13) {
                homeText.text = "上午好"
            } else if(timeHour > 12 && timeHour < 19) {
                homeText.text = "下午好"
            } else if(timeHour > 18 && timeHour < 23) {
                homeText.text = "晚上好"
            } else {
                //homeText.text = "晚安"
            }
            window.completedStart.homeLoaded = true;
        }
    }
    
    Item {
        id: homeMain
        anchors.fill: parent
        // 顶部标题
        Item {
            x: 24
            y: 24
            height: 40
            width: parent.width - 48
            z: 10
            Text {
                id: homeText
                x: 0
                y: 0
                height: 40
                verticalAlignment: Text.AlignVCenter
                text: "推荐"
                font.pixelSize: Style.settings.pageTitle
                color: Style.themes.fontColor
            }
            //QButton { x: parent.width - 120; y: 0; height: 40; width: 120; iconCharacter: "\uf10c"; text: "刷新" }
            QDrop {
                x: parent.width - 128
                y: 0
                height: 36; width: 128
                radius: 18
                anchors.right: parent.right
                choice: MusicApi.songSource
                model: ["酷狗音乐","网易云音乐","QQ音乐","自定义源"]
                onTransformed: (choiced) => {
                                   MusicApi.songSource = choiced
                               }
            }
        }

        QScrollView {
            id: homeView
            x: 0
            y: 68
            width: homePage.width
            height: homePage.height - 68
            property int standWidth: homePage.width - 52

            contentChildren: Column {
                id: homeContent
                spacing: 16
                padding: 24
                Rectangle {
                    width: homeView.standWidth
                    height: warnText.implicitHeight + 48
                    color: Style.darkis ? "#888826" : "#fafaaa"
                    radius: Style.settings.cubeRadius
                    border.color: "#bbbb38"
                    border.width: 2
                    Text {
                        x: 24
                        y: 24
                        font.family: iconFont.name
                        height: warnText.implicitHeight
                        text: "\uf11a"
                        color: Style.themes.textColor
                        font.pixelSize: Style.settings.texticon
                    }
                    Text {
                        id: warnText
                        x: 48
                        y: 24
                        text: "该版本属于开发中Beta版本，是未正式发布的开发中测试版本，部分功能仍未有效，并且稳定性欠佳，非最终质量"
                        elide: Text.ElideRight
                        color: Style.themes.textColor
                        font.bold: false
                        font.pixelSize: Style.settings.textmain
                    }
                    SButton {
                        iconCharacter: "\uf025"
                        x: parent.width - 52
                        anchors.verticalCenter: parent.verticalCenter
                        width: 36
                        height: 36
                        radius: 18
                        iconSize: Style.settings.texticon
                        buttonColor: "transparent"
                        shadowEnabled: false
                        onClicked: {
                            parent.visible = false;
                        }
                    }
                }
                // 首页头部部分
                Item {
                    height: 256
                    width: homeView.standWidth
                    readonly property int halfWidth: homeView.standWidth / 2

                    // 每日推荐大卡片
                    QPicture {
                        x: 8
                        width: parent.halfWidth - 16
                        height: 256
                        source: "qrc:/QueMusic/resources/app/rainbowMusicIcon.png"
                        //color: "blue"
                        sourceSize: Qt.size(256,256)
                        radius: Style.settings.cubeRadius
                        Behavior on y { NumberAnimation { duration: 320; easing.type: Easing.OutCubic } }
                        RectangularShadow {
                            anchors.fill: parent
                            opacity: parent.y / -8
                            z: -1
                            offset.x: 3
                            offset.y: -parent.y
                            radius: Style.settings.cubeRadius
                            blur: 24
                            spread: 0
                            color: Style.themes.shadowColor
                        }

                        // 底部渐变遮罩
                        Rectangle {
                            anchors.bottom: parent.bottom
                            width: parent.width
                            height: 116
                            radius: Style.settings.cubeRadius

                            gradient: Gradient {
                                GradientStop { position: 0.0; color: "transparent" }
                                GradientStop { position: 1.0; color: Qt.rgba(0,0,0,0.55) }
                            }
                        }
                        // 大标题
                        Text {
                            anchors.left: parent.left
                            anchors.bottom: parent.bottom
                            anchors.leftMargin: 20
                            anchors.bottomMargin: 14
                            text: "每日推荐"
                            font.pixelSize: 28
                            font.bold: true
                            color: "#ffffff"
                        }
                        Text {
                            anchors.left: parent.left
                            anchors.bottom: parent.bottom
                            anchors.leftMargin: 20
                            anchors.bottomMargin: 54
                            text: "根据你的口味每天更新"
                            font.pixelSize: Style.settings.text
                            color: "#d9ffffff"
                        }
                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onEntered: parent.y = -8;
                            onExited: parent.y = 0;

                            onClicked: {
                                MusicApi.recommendSongs.clear();
                                MusicApi.getRecommendSongs(1, 20, MusicApi.songSource);
                                var image = "qrc:/QueMusic/resources/app/rainbowMusicIcon.png";
                                var title = "每日推荐";
                                dailyRecomWindow.opened(title,image);
                                window.exitIndex = 1;
                            }
                        }
                    }

                    Rectangle {
                        x: parent.halfWidth + 8
                        width: parent.halfWidth - 16
                        height: 120
                        radius: Style.settings.cubeRadius
                        color: Style.themes.secondaryColor
                        Behavior on y { NumberAnimation { duration: 320; easing.type: Easing.OutCubic } }
                        RectangularShadow {
                            anchors.fill: parent
                            opacity: parent.y / -8
                            z: -1
                            offset.x: 3
                            offset.y: -parent.y
                            radius: Style.settings.cubeRadius
                            blur: 24
                            spread: 0
                            color: Style.themes.shadowColor
                        }

                        Text {
                            x: 16; y: 16
                            height: 20
                            text: "上次听到"
                            font.bold: true
                            font.pixelSize: Style.settings.textH2
                            color: Style.themes.fontColor
                            verticalAlignment: Text.AlignVCenter
                        }
                        Text {
                            anchors.centerIn: parent
                            visible: Options.lastSongs.name === ""
                            text: "还没有播放记录"
                            font.pixelSize: Style.settings.text
                            color: Style.themes.textColor
                        }
                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onEntered: parent.y = -8
                            onExited: parent.y = 0
                            onClicked: {
                                if(Options.lastSongs.hash !== "") {
                                    MusicApi.getMusicInfo(Options.lastSongs.hash, 0, Options.lastSongs.source);
                                }
                            }
                            QPicture {
                                y: 56
                                x: 16
                                width: 52; height: 52
                                radius: 12
                                source: Options.lastSongs.cover || "qrc:/QueMusic/resources/app/musicpic.png"
                            }
                            Text {
                                x: 78
                                y: 60
                                width: parent.width - 78
                                height: 23
                                text: Options.lastSongs.name
                                font.bold: true
                                font.pixelSize: Style.settings.textmain
                                color: Style.themes.fontColor
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }
                            Text {
                                x: 78
                                y: 83
                                width: parent.width - 78
                                height: 21
                                text: Options.lastSongs.artist
                                font.pixelSize: Style.settings.text
                                color: Style.themes.textColor
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }
                            QButton {
                                x: parent.width - 84
                                y: 10
                                height: 32; width: 68
                                radius: 18
                                iconCharacter: "\uf00e"
                                text: "播放"
                                shadowEnabled: false
                                buttonColor: Style.themes.themeColor
                                textColor: Style.themes.fullColor
                                iconColor: Style.themes.fullColor
                                onClicked: {
                                    if(Options.lastSongs.hash !== "") {
                                        MusicApi.getMusicInfo(Options.lastSongs.hash, 0, Options.lastSongs.source);
                                    }
                                }
                            }
                            SButton {
                                x: parent.width - 120
                                y: 10
                                width: 32
                                height: 32
                                radius: 16
                                iconCharacter: "\uf075"
                                shadowEnabled: false
                                buttonColor: Style.themes.sideColor
                                onClicked: {
                                }
                            }
                            //  加入播放列表
                            SButton {
                                x: parent.width - 52
                                y: 64
                                iconCharacter: "\uf095"
                                width: 36
                                height: 36
                                radius: 36
                                buttonColor: "transparent"
                                hoverColor: Style.themes.hoverColor
                                shadowEnabled: false
                                onClicked: {
                                    var listIndex = -1;
                                    var indexHash = Options.lastSongs.hash;
                                    for(var i = 0;i < playListModel.count;i++) {
                                        var forUrl = playListModel.get(i).path;
                                        if(forUrl === indexHash) {
                                            listIndex = i;
                                        }
                                    }
                                    if (listIndex == -1) {
                                        playListModel.append({ name: Options.lastSongs.name, path: Options.lastSongs.hash, songer: Options.lastSongs.artist, source: Options.lastSongs.source });
                                        mainWarn.tiped("成功加入播放列表",1);
                                    }
                                }
                            }
                        }
                    }

                    // 我的收藏歌单
                    Rectangle {
                        x: parent.halfWidth + 8
                        y: 136
                        width: parent.halfWidth - 16
                        height: 120
                        radius: Style.settings.cubeRadius
                        color: Style.themes.containColor
                        Behavior on y { NumberAnimation { duration: 320; easing.type: Easing.OutCubic } }
                        RectangularShadow {
                            id: favorCardShadow
                            anchors.fill: parent
                            opacity: 0
                            z: -1
                            offset.x: 3
                            offset.y: -parent.y + 136
                            radius: Style.settings.cubeRadius
                            blur: 24
                            spread: 0
                            color: Style.themes.shadowColor
                            Behavior on opacity { NumberAnimation { duration: 320; easing.type: Easing.OutCubic } }
                        }

                        Row {
                            anchors.centerIn: parent
                            spacing: 12
                            Text {
                                text: "\uf0c1"
                                font.family: iconFont.name
                                font.pixelSize: 32
                                color: Style.themes.themeColor
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Column {
                                spacing: 2
                                Text {
                                    text: "我的收藏歌单"
                                    font.bold: true
                                    font.pixelSize: Style.settings.textH1
                                    color: Style.themes.fontColor
                                }
                                Text {
                                    text: favoritesList.count + " 个歌单"
                                    font.pixelSize: Style.settings.textmain
                                    color: Style.themes.textColor
                                }
                            }
                        }
                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onEntered: {
                                parent.y = 128;
                                favorCardShadow.opacity = 1.0;
                            }
                            onExited: {
                                parent.y = 136;
                                favorCardShadow.opacity = 0.0;
                            }
                            onClicked: {
                                sidebar.indexed(3);
                                mainContent.contentIndexed(3);
                            }
                        }
                    }
                }

                QHead { text: "推荐分类" }
                Row {
                    height: 128
                    spacing: 24
                    width: homeView.standWidth
                    Repeater {
                        model: MusicApi.getHotlistMenu
                        delegate: Item {
                            id: hotlistDel
                            height: 128
                            width: 128
                            property int radius: Style.settings.labelRadius

                            Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutExpo } }

                            MouseArea {
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: {
                                    MusicApi.musicPlaylists.clear();
                                    MusicApi.globaltagid = model.tagid;
                                    MusicApi.getMusicPlaylists(model.tagid,1,20);
                                    var image = model.cover.replace("{size}", "256") || "qrc:/QueMusic/resources/app/musicpic.png";
                                    var title = model.title;
                                    recommendWindow.opened(title,image);
                                    window.exitIndex = 1;
                                }
                                onPressed: hotlistDel.scale = 0.92
                                onReleased: hotlistDel.scale = 1.0
                                onCanceled: hotlistDel.scale = 1.0
                            }
                            Item {
                                width: hotlistDel.width
                                height: hotlistDel.width

                                // 原始图像，隐藏
                                Image {
                                    id: sourceItem
                                    source: model.cover.replace("{size}", "128") || "qrc:/QueMusic/resources/app/musicpic.png"
                                    anchors.fill: parent
                                    sourceSize.width: parent.width
                                    sourceSize.height: parent.height
                                    fillMode: Image.PreserveAspectCrop
                                    visible: false
                                }

                                MultiEffect {
                                    id: multiEffect
                                    source: sourceItem
                                    anchors.fill: parent
                                    maskEnabled: true
                                    maskSource: mask
                                    // 下面两个属性抗锯齿
                                    maskThresholdMin: 0.5
                                    maskSpreadAtMin: 1.0

                                }

                                // 圆形黑色矩形（用于遮罩）
                                Item {
                                    id: mask
                                    width: sourceItem.width
                                    height: sourceItem.height
                                    layer.enabled: true
                                    visible: false


                                    Rectangle {
                                        anchors.fill: parent
                                        radius: hotlistDel.radius
                                        color: "black" // 黑色用于掩码：纯黑表示完全不透明
                                    }
                                }
                            }

                            Text {
                                id: text
                                width: hotlistDel.width - 32
                                height: 32
                                x: 12
                                y: hotlistDel.height - height
                                text: model.title
                                font.pixelSize: 15
                                font.bold: true
                                opacity: 0.8
                                color: Style.themes.primaryColor
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }
                }

                QHead { text: "热门歌单" }

                Flow {
                    spacing: 24
                    width: homeView.standWidth

                    Repeater {
                        model: MusicApi.hotPlayLists
                        delegate: Rectangle {
                            width: 148
                            height: 256
                            radius: Style.settings.labelRadius
                            color: Style.themes.primaryColor
                            scale: hotPlayListsArea.containsMouse ? 1.05 : 1.0
                            Behavior on scale { NumberAnimation { duration: 240; easing.type: Easing.OutExpo } }
                            RectangularShadow {
                                anchors.fill: parent
                                z: -1
                                offset.x: 2
                                offset.y: 2
                                radius: Style.settings.labelRadius
                                blur: hotPlayListsArea.containsMouse ? 24 : 8
                                spread: 0
                                visible: true
                                color: Style.themes.shadowColor
                                Behavior on blur { NumberAnimation { duration: 240; easing.type: Easing.OutExpo } }
                            }
                            MouseArea {
                                id: hotPlayListsArea
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: {
                                    hotlistsWindow.mainTarget = homeMain;
                                    MusicApi.playlistSong.clear();
                                    MusicApi.globalid = model.hash;
                                    MusicApi.getPlaylistSongs(model.hash,1,20);
                                    hotlistsWindow.opened(model);
                                    window.exitIndex = 1;
                                }
                            }
                            QPicture {
                                width: 148
                                height: 148
                                source: model.cover.replace("{size}", "128")
                                radius: Style.settings.labelRadius
                                //cache: true
                                radius3: 0
                                radius4: 0
                            }
                            Text {
                                x: 16
                                y: 160
                                width: 116
                                text: model.title
                                font.bold: true
                                color: Style.themes.fontColor
                                font.pixelSize: Style.settings.textmain
                                wrapMode: Text.Wrap
                                maximumLineCount: 2
                                clip: true
                                elide: Text.ElideRight
                            }
                            Text {
                                x: 16
                                y: 200
                                width: 116
                                height: 18
                                text: model.album
                                color: Style.themes.textColor
                                font.pixelSize: Style.settings.text
                                //wrapMode: Text.Wrap
                                //maximumLineCount: 2
                                clip: true
                                elide: Text.ElideRight
                            }
                            Rectangle {
                                x: 0
                                y: 224
                                height: 32
                                width: 148
                                color: Style.themes.sideColor
                                bottomLeftRadius: Style.settings.labelRadius
                                bottomRightRadius: Style.settings.labelRadius
                                Text {
                                    id: playIcon
                                    x: 16
                                    height: 32
                                    text: "\uf00e"
                                    font.pixelSize: Style.settings.text
                                    font.family: iconFont.name
                                    color: Style.themes.textColor
                                    verticalAlignment: Text.AlignVCenter
                                }
                                Text {
                                    x: 20 + playIcon.width
                                    height: 32
                                    text: Math.floor(model.playcount / 10000) + "万"
                                    font.pixelSize: Style.settings.textTip
                                    color: Style.themes.textColor
                                    verticalAlignment: Text.AlignVCenter
                                }
                                Text {
                                    x: parent.width - width - 16
                                    height: 32
                                    text: model.duration + "首"
                                    font.pixelSize: Style.settings.textTip
                                    color: Style.themes.textColor
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }
                        }
                    }
                }

                Item {
                    height: 60
                    width: homeView.standWidth
                    QButton {
                        anchors.centerIn: parent
                        height: 40; width: 120
                        radius: 20
                        iconCharacter: "\uf0f8"
                        text: "更多"
                        onClicked: {
                            if(MusicApi.songSource === 0) {
                                MusicApi.hotPlayLists.clear();
                            }
                            if(MusicApi.hotPlayLists.count % 20 === 0) {
                                MusicApi.getHotPlaylists(MusicApi.hotPlayLists.count / 20 + 1);
                            } else {
                                MusicApi.hotPlayLists.clear();
                                MusicApi.getHotPlaylists(MusicApi.hotPlayLists.count / 20 + 1);
                            }
                        }
                    }
                }
            }
        }
    }
    AnimatorWindow {
        id: dailyRecomWindow
        mainTarget: homeMain
        content: Item {

            QListView {
                id: dailyRecomView
                x: 24
                y: 128
                width: hotlistsWindow.width - 32
                height: hotlistsWindow.height - 128
                model: MusicApi.recommendSongs
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

                footer: Item {
                    height: 60
                    width: dailyRecomView.width
                    QButton {
                        anchors.centerIn: parent
                        height: 40; width: 120
                        radius: 20
                        iconCharacter: "\uf0f8"
                        text: "更多"
                        onClicked: {
                            if(!MusicApi.loadState && MusicApi.recommendSongs.count % 20 === 0) {
                                MusicApi.getRecommendSongs(MusicApi.recommendSongs.count / 20 + 1, 20, MusicApi.songSource);
                            } else {
                                mainWarn.tiped("没有更多了",0);
                            }
                        }
                    }
                }
            }
        }
    }

    AnimatorWindow {
        id: recommendWindow
        mainTarget: homeMain
        haveControl: false
        content: QListView {
            id: recomView
            x: 24
            y: 128
            width: recommendWindow.width - 32
            height: recommendWindow.height - 128
            model: MusicApi.musicPlaylists
            clip: true
            //reuseItems: true
            topMargin: 8
            bottomMargin: 24
            isList: true

            footer: Item {
                height: 60
                width: recomView.width
                QButton {
                    anchors.centerIn: parent
                    height: 40; width: 120
                    radius: 20
                    iconCharacter: "\uf0f8"
                    text: "更多"
                    onClicked: {
                        if(!MusicApi.loadState && MusicApi.musicPlaylists.count % 20 === 0) {
                            MusicApi.getMusicPlaylists(MusicApi.globaltagid,MusicApi.musicPlaylists.count / 20 + 1,20);
                        } else {
                            mainWarn.tiped("没有更多了",0);
                        }
                    }
                }
            }

            onClicked: (index) => {
                hotlistsWindow.mainTarget = recommendWindow;
                MusicApi.playlistSong.clear();
                MusicApi.globalid = model.get(index).hash;
                MusicApi.getPlaylistSongs(model.get(index).hash,1,20);
                hotlistsWindow.opened(model.get(index));
                window.exitIndex = 2;
            }
            onToolClicked: (index,tool) => {
                switch(tool) {
                }
            }
        }
    }
    PlayListWindow {
        id: hotlistsWindow
        mainTarget: homeMain
        winIndex: 2
        content: Item {

            QListView {
                id: hotlistsView
                x: 24
                y: 184
                width: hotlistsWindow.width - 32
                height: hotlistsWindow.height - 184
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

                footer: Item {
                    height: 60
                    width: hotlistsView.width
                    QButton {
                        anchors.centerIn: parent
                        height: 40; width: 120
                        radius: 20
                        iconCharacter: "\uf0f8"
                        text: "更多"
                        onClicked: {
                            if(!MusicApi.loadState && MusicApi.playlistSong.count % 20 === 0) {
                                var tagid = hotlistsWindow.id;
                                MusicApi.getPlaylistSongs(tagid,MusicApi.playlistSong.count / 20 + 1,20);
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
