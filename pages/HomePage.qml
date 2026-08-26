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
                font.weight: Font.DemiBold
                color: Style.themes.fontColor
            }
            //QButton { x: parent.width - 120; y: 0; height: 40; width: 120; iconCharacter: "\uf10c"; text: "刷新" }
            QDrop {
                x: parent.width - 96
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
                    height: warnText.implicitHeight + 40
                    color: Style.themes.containColor
                    radius: Style.settings.cubeRadius
                    border.color: Style.themes.sideColor
                    border.width: 1
                    Text {
                        x: 20
                        anchors.verticalCenter: parent.verticalCenter
                        font.family: iconFont.name
                        height: warnText.implicitHeight
                        text: "\uf11a"
                        color: Style.themes.themeColor
                        font.pixelSize: Style.settings.texticon
                    }
                    Text {
                        id: warnText
                        x: 48
                        y: 20
                        width: parent.width - 108
                        text: "该版本属于开发中Beta版本，是未正式发布的开发中测试版本，部分功能仍未有效，并且稳定性欠佳，非最终质量"
                        wrapMode: Text.Wrap
                        color: Style.themes.fontColor
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
                    readonly property int leftWidth: homeView.standWidth * 0.6
                    readonly property int rightWidth: homeView.standWidth * 0.4

                    // 每日推荐大卡片
                    QFloatCard {
                        width: parent.leftWidth - 8
                        height: 256

                        // 大标题
                        Text {
                            x: 16
                            y: 50
                            text: "DAILY RECOMMEND"
                            width: parent.width
                            font.pixelSize: Style.settings.textmain
                            font.bold: true
                            elide: Text.ElideRight
                            color: Style.themes.themeColor
                        }
                        Text {
                            x: 16
                            y: 88
                            text: "每日推荐"
                            width: parent.width
                            elide: Text.ElideRight
                            font.pixelSize: 32
                            font.bold: true
                            color: Style.themes.fontColor
                        }
                        Text {
                            x: 16
                            y: 152
                            text: "那些你反复循环的节奏，长成了今天的模样。"
                            width: parent.width
                            wrapMode: Text.Wrap
                            font.pixelSize: Style.settings.textmain
                            color: Style.themes.textColor
                        }
                        Rectangle {
                            x: parent.width - 152
                            y: 64
                            width: 128
                            height: 128
                            radius: Style.settings.cubeRadius
                            color: Style.themes.secondaryColor
                            QPicture {
                                anchors.fill: parent
                                anchors.margins: 8
                                sourceSize: Qt.size(128,128)
                                source: "qrc:/QueMusic/resources/app/rainbowMusicIcon.png"
                                radius: Style.settings.cubeRadius - 2
                            }

                            RectangularShadow {
                                anchors.fill: parent
                                z: -1
                                offset.x: 5
                                offset.y: 5
                                radius: Style.settings.cubeRadius
                                blur: 24
                                color: Style.themes.shadowColor
                            }
                        }

                        onClicked: {
                            MusicApi.recommendSongs.clear();
                            MusicApi.getRecommendSongs(1, 20, MusicApi.songSource);
                            var image = "qrc:/QueMusic/resources/app/rainbowMusicIcon.png";
                            var title = "每日推荐";
                            dailyRecomWindow.opened(title,image);
                            window.exitIndex = 1;
                        }
                        controlItem: QButton {
                            x: 16
                            y: 192
                            height: 32
                            radius: 16
                            iconCharacter: "\uf0e7"
                            text: "前往查看"
                            shadowEnabled: false
                            buttonColor: Style.themes.themeColor
                            textColor: Style.themes.fullColor
                            iconColor: Style.themes.fullColor
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

                    QFloatCard {
                        x: parent.leftWidth + 8
                        width: parent.rightWidth - 8
                        height: 120

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
                        onClicked: {
                            if(Options.lastSongs.hash !== "") {
                                MusicApi.getMusicInfo(Options.lastSongs.hash, 0, Options.lastSongs.source);
                            }
                        }
                        controlItem: [
                            QPicture {
                                y: 56
                                x: 16
                                width: 52; height: 52
                                radius: 12
                                source: Options.lastSongs.cover || "qrc:/QueMusic/resources/app/musicpic.png"
                            },
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
                            },
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
                            },
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
                            },
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
                            },
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
                        ]
                    }

                    // 我的收藏歌单
                    QFloatCard {
                        x: parent.leftWidth + 8
                        y: 136
                        width: parent.rightWidth - 8
                        height: 120
                        color: Style.themes.containColor

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
                        onClicked: {
                            sidebar.indexed(3);
                            mainContent.contentIndexed(3);
                        }
                    }
                }

                QHead { text: "私人专属" }

                Item {
                    height: 180
                    width: homeView.standWidth
                    readonly property int leftWidth: homeView.standWidth * 0.5 - 8
                    readonly property int rightWidth: homeView.standWidth * 0.5 - 8
                    Rectangle {
                        x: 0
                        y: 0
                        width: parent.leftWidth
                        height: 180
                        color: Style.themes.primaryColor
                        radius: Style.settings.cubeRadius
                        Text {
                            x: 12
                            y: 16
                            height: 24
                            text: "歌单分类"
                            font.pixelSize: Style.settings.textH2
                            font.bold: true
                            color: Style.themes.fontColor
                            verticalAlignment: Text.AlignVCenter
                        }
                        RectangularShadow {
                            anchors.fill: parent
                            z: -1
                            offset.x: 3
                            offset.y: 5
                            radius: Style.settings.cubeRadius
                            blur: 10
                            spread: 0
                            color: Style.themes.shadowColor
                        }

                        ListView {
                            id: categoryList
                            y: 56
                            width: parent.width
                            height: 96
                            orientation: ListView.Horizontal
                            spacing: 20
                            leftMargin: 16
                            rightMargin: 16
                            clip: true
                            model: MusicApi.getHotlistMenu

                            // 隐藏系统滚动条，用惯性和鼠标拖拽
                            interactive: true
                            boundsBehavior: Flickable.DragOverBounds

                            delegate: Item {
                                id: catDel
                                width: 96
                                height: 96
                                property int radius: Style.settings.cubeRadius
                                //scale: catMouse.containsMouse ? 1.06 : 1.0
                                //Behavior on scale { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }

                                QPicture {
                                    id: card
                                    anchors.fill: parent
                                    radius: catDel.radius
                                    source: (model.cover || "").replace("{size}", "256") || "qrc:/QueMusic/resources/app/musicpic.png"
                                    sourceSize: Qt.size(256,256)

                                    // 底部渐变遮罩 —— 保证标题永远可读
                                    Rectangle {
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.bottom: parent.bottom
                                        height: 48
                                        radius: catDel.radius
                                        gradient: Gradient {
                                            GradientStop { position: 0.0; color: "transparent" }
                                            GradientStop { position: 1.0; color: Qt.rgba(0, 0, 0, 0.72) }
                                        }
                                    }

                                    // 分类标题
                                    Text {
                                        y: 60
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        text: model.title || ""
                                        font.pixelSize: 16
                                        font.weight: Font.Bold
                                        color: "#FFFFFF"
                                        elide: Text.ElideRight
                                        maximumLineCount: 1
                                    }

                                    // hover 时浮现的箭头
                                    Text {
                                        anchors.right: parent.right
                                        anchors.top: parent.top
                                        anchors.margins: 14
                                        text: "\uf0e7"
                                        font.family: iconFont.name
                                        font.pixelSize: 18
                                        color: "#FFFFFF"
                                        opacity: catMouse.containsMouse ? 1 : 0
                                        Behavior on opacity { NumberAnimation { duration: 200 } }
                                    }
                                }

                                MouseArea {
                                    id: catMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onPressed: catDel.scale = 0.96
                                    onReleased: catDel.scale = 1.0
                                    onCanceled: catDel.scale = 1.0

                                    onClicked: {
                                        MusicApi.musicPlaylists.clear();
                                        MusicApi.globaltagid = model.tagid;
                                        MusicApi.getMusicPlaylists(model.tagid, 1, 20);
                                        var image = (model.cover || "").replace("{size}", "256") || "qrc:/QueMusic/resources/app/musicpic.png";
                                        recommendWindow.opened(model.title, image);
                                        window.exitIndex = 1;
                                    }
                                }
                            }
                        }
                    }
                    QFloatCard {
                        x: parent.leftWidth + 16
                        y: 0
                        width: parent.rightWidth
                        height: 82
                        Text {
                            x: 20
                            y: 20
                            width: 42
                            height: 42
                            text: "\uf104"
                            font.family: iconFont.name
                            font.pixelSize: 32
                            color: Style.themes.themeColor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        Text {
                            x: 70
                            y: 20
                            height: 22
                            text: "私人漫游"
                            color: Style.themes.fontColor
                            font.pixelSize: Style.settings.textmain
                            verticalAlignment: Text.AlignVCenter
                        }
                        Text {
                            x: 70
                            y: 42
                            height: 20
                            text: "全网播放量最高的热门单曲合集"
                            color: Style.themes.textColor
                            font.pixelSize: Style.settings.text
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: {
                            MusicApi.personalFm.clear();
                            personalWindow.page = 1;
                            personalWindow.mode = "fm";
                            MusicApi.getPersonalFm(1, 20, MusicApi.songSource);
                            personalWindow.currentModel = MusicApi.personalFm;
                            personalWindow.opened("私人漫游", "qrc:/QueMusic/resources/app/rainbowMusicIcon.png");
                            window.exitIndex = 1;
                        }
                        controlItem: SButton {
                            x: parent.width - 50
                            y: 23
                            iconCharacter: "\uf0e7"
                            width: 36
                            height: 36
                            radius: 18
                            buttonColor: "transparent"
                            shadowEnabled: false
                            onClicked: parent.clicked()
                        }
                    }
                    QFloatCard {
                        x: parent.leftWidth + 16
                        y: 98
                        width: parent.rightWidth
                        height: 82
                        Text {
                            x: 20
                            y: 20
                            width: 42
                            height: 42
                            text: "\uf109"
                            font.family: iconFont.name
                            font.pixelSize: 32
                            color: Style.themes.themeColor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        Text {
                            x: 70
                            y: 20
                            height: 22
                            text: "私人雷达"
                            color: Style.themes.fontColor
                            font.pixelSize: Style.settings.textmain
                            verticalAlignment: Text.AlignVCenter
                        }
                        Text {
                            x: 70
                            y: 42
                            height: 20
                            text: "最新发行高赞潮流流行歌曲"
                            color: Style.themes.textColor
                            font.pixelSize: Style.settings.text
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: {
                            MusicApi.personalRadar.clear();
                            personalWindow.page = 1;
                            personalWindow.mode = "radar";
                            MusicApi.getPersonalRadar(1, 20, MusicApi.songSource);
                            personalWindow.currentModel = MusicApi.personalRadar;
                            personalWindow.opened("私人雷达", "qrc:/QueMusic/resources/app/rainbowMusicIcon.png");
                            window.exitIndex = 1;
                        }
                        controlItem: SButton {
                            x: parent.width - 50
                            y: 23
                            iconCharacter: "\uf0e7"
                            width: 36
                            height: 36
                            radius: 18
                            buttonColor: "transparent"
                            shadowEnabled: false
                            onClicked: parent.clicked()
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
                                sourceSize: Qt.size(128,128)
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
        haveControl: false
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

                onEnded: {
                    if(MusicApi.recommendSongs.count % 20 === 0 && MusicApi.recommendSongs.count !== 0) {
                        MusicApi.getRecommendSongs(MusicApi.recommendSongs.count / 20 + 1, 20, MusicApi.songSource);
                        isEnd = false;
                    } else {
                        if(MusicApi.recommendSongs.count !== 0) {
                            isEnd = true;
                        }
                    }
                }
            }
        }
    }

    // 私人漫游 / 私人雷达
    AnimatorWindow {
        id: personalWindow
        mainTarget: homeMain
        haveControl: false
        property var currentModel: MusicApi.personalFm
        property string mode: "fm"   // "fm" 私人漫游 / "radar" 私人雷达
        property int page: 1         // 当前页码
        property int pageSize: 20    // 每页数量
        content: Item {
            QListView {
                id: personalView
                x: 24
                y: 128
                width: personalWindow.width - 32
                height: personalWindow.height - 128
                model: personalWindow.currentModel
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

                onEnded: {
                    if(personalWindow.currentModel.count % personalWindow.pageSize === 0 && MusicApi.playlistSong.count !== 0) {
                        personalWindow.page += 1;
                        if(personalWindow.mode === "fm") {
                            MusicApi.getPersonalFm(personalWindow.page, personalWindow.pageSize, MusicApi.songSource);
                        } else {
                            MusicApi.getPersonalRadar(personalWindow.page, personalWindow.pageSize, MusicApi.songSource);
                        }
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

            onEnded: {
                if(MusicApi.musicPlaylists.count % 20 === 0 && MusicApi.musicPlaylists.count !== 0) {
                    MusicApi.getMusicPlaylists(MusicApi.globaltagid,MusicApi.musicPlaylists.count / 20 + 1,20);
                    isEnd = false;
                } else {
                    if(MusicApi.musicPlaylists.count !== 0) {
                        isEnd = true;
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

                onEnded: {
                    if(MusicApi.playlistSong.count % 20 === 0 && MusicApi.playlistSong.count !== 0) {
                        var tagid = hotlistsWindow.id;
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
