// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
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
            window.completedStart.playlistLoaded = true;
        }
    }

    // 顶部标题
    Item {
        x: 24
        y: 24
        height: 40
        width: parent.width - 48
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
            }
        }
    }

    QBlurTapBar {
        x: 24
        y: 80
        z: 12
        model: ["歌曲","歌单","排行榜","歌手"]
        tabWidth: 80
        width: 324
        rectXy: Qt.rect(0, 12, width, 40)
        blurSource: playlistChildPage
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
        x: parent.width - 172
        y: 80
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


    QPages {
        id: playlistChildPage
        x: 24
        y: 68
        width: parent.width - 48
        height: parent.height - 68
        pageList: [musicsPage,musicMenuPage,cloud,album]
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
                        color: musicMenuPage.musicMenuIndex === index ? Style.themes.themeColor : Style.themes.fullColor
                        border.color: Style.themes.secondaryColor
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
                            // map 数组需用 modelData.title（旧版是字符串数组）
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

            ListView {
                id: musicMenuList
                anchors.fill: parent
                anchors.topMargin: musicMenuFlow.implicitHeight + 80
                model: MusicApi.musicPlaylists
                property int artistX: width / 2 - 50
                clip: true
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

                add: Transition {
                    ParallelAnimation {
                        NumberAnimation {
                            properties: "x"
                            from: 400
                            to: 0
                            duration: 350
                            easing.type: Easing.OutExpo
                        }
                        NumberAnimation {
                            properties: "opacity"
                            from: 0
                            to: 1
                            duration: 350
                            easing.type: Easing.OutExpo
                        }
                    }
                }
                displaced: Transition {
                    NumberAnimation {
                        properties: "y"
                        duration: 240
                        easing.type: Easing.Bezier; easing.bezierCurve: [ 0.23, 0.06, 0.00, 1.00, 1, 1 ]
                    }
                }
                delegate: Rectangle {
                    id: musicMenuMusic
                    height: 64
                    width: musicMenuList.width
                    radius: Style.settings.labelRadius
                    color: index % 2 === 0 ? Style.themes.blurOverlayColor : "transparent"

                    Rectangle {
                        anchors.fill: parent
                        radius: Style.settings.labelRadius
                        color: Style.themes.hoverColor
                        opacity: musicMenuArea.containsMouse ? 1 : 0
                        z: 1
                        Behavior on opacity { NumberAnimation { duration: 80 } }
                    }

                    Rectangle {
                        y: 8
                        x: 8
                        z: 4
                        width: 48
                        height: 48
                        color: Style.themes.containColor
                        radius: 10
                        Text {
                            anchors.fill: parent
                            text: "\uf044"
                            font.family: iconFont.name
                            font.pixelSize: Style.settings.texticon
                            color: Style.themes.fontColor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }


                    Text {
                        x: 80
                        y: 0
                        z: 3
                        width: musicMenuList.artistX - 96
                        height: 64
                        text: model.specialname
                        color: Style.themes.fontColor
                        font.bold: true
                        elide: Text.ElideRight
                        font.pixelSize: Style.settings.textmain
                        verticalAlignment: Text.AlignVCenter
                        visible: true
                        Behavior on color { ColorAnimation { duration: 120 } }
                    }
                    Text {
                        x: musicMenuList.artistX
                        y: 0
                        z: 3
                        width: musicMenuList.artistX - 80
                        height: 64
                        text: model.username
                        color: Style.themes.textColor
                        font.bold: true
                        elide: Text.ElideRight
                        font.pixelSize: Style.settings.text
                        verticalAlignment: Text.AlignVCenter
                        visible: true
                        Behavior on color { ColorAnimation { duration: 120 } }
                    }
                    Row {
                        anchors.right: parent.right
                        anchors.rightMargin: 20
                        spacing: 5
                        z: 2
                        y: 12
                        height: 40
                        SButton {
                            iconCharacter: "\uf095"
                            width: 40
                            height: 40
                            radius: 40
                            buttonColor: "transparent"
                            hoverColor: Qt.rgba(0.5,0.5,0.5,0.2)
                            shadowEnabled: false
                            onClicked: {
                            }
                        }
                        SButton {
                            iconCharacter: "\uf0c8"
                            width: 40
                            height: 40
                            radius: 40
                            buttonColor: "transparent"
                            hoverColor: Qt.rgba(0.5,0.5,0.5,0.2)
                            shadowEnabled: false
                            onClicked: {
                            }
                        }
                        SButton {
                            iconCharacter: "\uf00f"
                            width: 40
                            height: 40
                            radius: 40
                            buttonColor: "transparent"
                            hoverColor: Qt.rgba(1.0,0.5,0.5,0.8)
                            shadowEnabled: false
                            onClicked: {
                            }
                        }
                    }

                    MouseArea {
                        id: musicMenuArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            MusicApi.playlistSong.clear();
                            MusicApi.globalid = model.hash;
                            MusicApi.getPlaylistSongs(model.hash, 1, 20);
                            playListSongsWindow.opened(model);
                            window.exitIndex = 2;
                        }
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
                MusicApi.musicToplist.clear();
                MusicApi.getAllToplist(MusicApi.songSource);
            }
            QHead {
                text: "排行榜"
                y: 68
                width: parent.width - 24
            }
            ListView {
                id: toplistView
                y: 116
                width: parent.width
                height: parent.height - 116
                model: MusicApi.musicToplist
                clip: true
                bottomMargin: 24
                delegate: Rectangle {
                    id: toplistDel
                    height: 64
                    width: toplistView.width - 24
                    radius: Style.settings.labelRadius
                    color: index % 2 === 0 ? Style.themes.blurOverlayColor : "transparent"

                    Rectangle {
                        anchors.fill: parent
                        radius: Style.settings.labelRadius
                        color: Style.themes.hoverColor
                        opacity: toplistArea.containsMouse ? 1 : 0
                        z: 1
                        Behavior on opacity { NumberAnimation { duration: 80 } }
                    }
                    QPicture {
                        y: 8
                        x: 8
                        z: 4
                        width: 48
                        height: 48
                        radius: 10
                        source: model.cover.replace("{size}","128") || "qrc:/QueMusic/resources/app/musicpic.png"
                    }
                    Text {
                        x: 80
                        y: 0
                        z: 3
                        width: toplistView.width - 200
                        height: 64
                        text: model.title
                        color: Style.themes.fontColor
                        font.bold: true
                        elide: Text.ElideRight
                        font.pixelSize: Style.settings.textmain
                        verticalAlignment: Text.AlignVCenter
                    }
                    Text {
                        x: toplistView.width - 160
                        y: 0
                        z: 3
                        width: 120
                        height: 64
                        text: model.artist || ""
                        color: Style.themes.textColor
                        elide: Text.ElideRight
                        font.pixelSize: Style.settings.text
                        horizontalAlignment: Text.AlignRight
                        verticalAlignment: Text.AlignVCenter
                    }
                    MouseArea {
                        id: toplistArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            MusicApi.playlistSong.clear();
                            MusicApi.globalid = model.hash;
                            MusicApi.getMusicToplist(Number(model.hash), MusicApi.songSource);
                            playListSongsWindow.opened(model);
                            window.exitIndex = 2;
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
            Component.onCompleted: {
                MusicApi.singerList.clear();
                MusicApi.getHotSingers(1, 20, MusicApi.songSource);
            }
            QHead {
                text: "热门歌手"
                y: 68
                width: parent.width - 24
            }
            Flow {
                y: 116
                spacing: 20
                width: parent.width
                Repeater {
                    model: MusicApi.singerList
                    delegate: Item {
                        id: singerDel
                        width: 96
                        height: 132
                        scale: singerArea.containsMouse ? 1.06 : 1.0
                        Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutExpo } }
                        QPicture {
                            width: 96
                            height: 96
                            radius: 48
                            source: model.cover.replace("{size}","256") || "qrc:/QueMusic/resources/app/musicpic.png"
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
                                MusicApi.getSingerSongs(model.hash, 1, 20, MusicApi.songSource);
                                playListSongsWindow.opened(model);
                                window.exitIndex = 2;
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
                            if(!MusicApi.loadState && MusicApi.playlistSong.count % 20 === 0) {
                                var tagid = MusicApi.globalid;
                                MusicApi.getPlaylistSongs(tagid, MusicApi.playlistSong.count / 20 + 1, 20, MusicApi.songSource);
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