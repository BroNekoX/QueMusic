// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Effects
import QueMusic 1.0

Item {
    id: root
    z: 20
    anchors.fill: parent
    visible: false

    property string id: ""
    property string cover: "qrc:/QueMusic/resources/app/musicpic.png"
    property string title: "MusicFolder"
    property string artist: "Artist"
    property string descript: "Description"
    property int duration: 0
    property int playcount: 0
    property int standTopMargin: 184
    property real transX: 0
    property int originX: 0
    property list<ParallelAnimation> animeOnList: [openAnime0,openAnime1]
    property list<ParallelAnimation> animeOutList: [closeAnime0,closeAnime1]
    property int animeType: Style.settings.animeType
    transform: Translate { x: root.transX }
    Component.onCompleted: originX = mainTarget.x;

    property int winIndex: 1
    property Item mainTarget

    property int songSource: MusicApi.songSource
    default property alias content: loadWidget.sourceComponent



    function opened(info: var): void {
        root.id = info.hash || info.id;
        root.title = info.title || "";
        root.artist = info.artist || "";
        root.cover = info.cover.replace("{size}", "128") || "qrc:/QueMusic/resources/app/musicpic.png";
        root.descript = info.album || "Not have Description";
        root.duration = info.duration || 0;
        root.playcount = info.playcount || 0;
        loadWidget.active = true;
        if (FavoritePlaylists.isFavorite(root.id, "playlist")) {
            favoriteButton.iconColor = Style.themes.themeColor
        } else {
            favoriteButton.iconColor = Style.themes.textColor
        }
    }
    function closed(): void {
        animeOnList[animeType].running = false;
        mainTarget.visible = true;
        animeOutList[animeType].running = true;
        window.exitIndex -= 1;
    }
    Connections {
        target: window
        enabled: root.visible
        function onExit(): void {
            if(window.exitIndex <= root.winIndex) {
                root.animeOnList[root.animeType].running = false;
                root.mainTarget.visible = true;
                root.animeOutList[root.animeType].running = true;
            }
        }
    }

    ParallelAnimation {
        id: openAnime0
        onStarted: {
            root.transX = 0;
            root.mainTarget.x = root.originX;
        }

        NumberAnimation {
            target: root
            property: "scale"
            from: 0.8
            to: 1
            easing.type: Easing.OutExpo
            duration: 360
        }
        NumberAnimation {
            target: root
            property: "opacity"
            from: 0
            to: 1
            easing.type: Easing.OutExpo
            duration: 360
        }
        NumberAnimation {
            target: root.mainTarget
            property: "scale"
            from: 1
            to: 1.1
            duration: 100
        }
        NumberAnimation {
            target: root.mainTarget
            property: "opacity"
            from: 1
            to: 0
            duration: 100
        }
        onFinished: root.mainTarget.visible = false
    }
    ParallelAnimation {
        id: closeAnime0
        onStarted: {
            root.transX = 0;
            root.mainTarget.x = root.originX;
        }
        NumberAnimation {
            target: root
            property: "scale"
            from: 1
            to: 0.9
            duration: 100
        }
        NumberAnimation {
            target: root
            property: "opacity"
            from: 1
            to: 0
            duration: 100
        }
        NumberAnimation {
            target: root.mainTarget
            property: "scale"
            from: 1.2
            to: 1
            easing.type: Easing.OutExpo
            duration: 280
        }
        NumberAnimation {
            target: root.mainTarget
            property: "opacity"
            from: 0
            to: 1
            easing.type: Easing.OutExpo
            duration: 280
        }
        onFinished: {
            loadWidget.active = false
            root.visible = false
        }
    }

    ParallelAnimation {
        id: openAnime1
        onStarted: {
            root.scale = 1;
            root.mainTarget.scale = 1;
        }
        NumberAnimation {
            target: root
            property: "transX"
            from: 180
            to: 0
            easing.type: Easing.OutExpo
            duration: 420
        }
        NumberAnimation {
            target: root
            property: "opacity"
            from: 0
            to: 1
            easing.type: Easing.OutExpo
            duration: 420
        }
        NumberAnimation {
            target: root.mainTarget
            property: "x"
            from: root.originX
            to: root.originX - 180
            easing.type: Easing.InCubic
            duration: 120
        }
        NumberAnimation {
            target: root.mainTarget
            property: "opacity"
            from: 1
            to: 0
            easing.type: Easing.InCubic
            duration: 120
        }
        onFinished: root.mainTarget.visible = false
    }
    ParallelAnimation {
        id: closeAnime1
        onStarted: {
            root.scale = 1;
            root.mainTarget.scale = 1;
        }
        NumberAnimation {
            target: root
            property: "transX"
            from: 0
            to: 180
            duration: 100
        }
        NumberAnimation {
            target: root
            property: "opacity"
            from: 1
            to: 0
            duration: 100
        }
        NumberAnimation {
            target: root.mainTarget
            property: "x"
            from: root.originX - 180
            to: root.originX
            easing.type: Easing.OutExpo
            duration: 320
        }
        NumberAnimation {
            target: root.mainTarget
            property: "opacity"
            from: 0
            to: 1
            easing.type: Easing.OutExpo
            duration: 320
        }
        onFinished: {
            loadWidget.active = false
            root.visible = false
        }
    }

    Rectangle {
        x: 24
        y: 24
        width: root.width - 48
        height: 160
        z: 10
        radius: Style.settings.cubeRadius
        color: Style.themes.primaryColor
        RectangularShadow {
            anchors.fill: parent
            z: -1
            offset.x: 3
            offset.y: 3
            radius: Style.settings.cubeRadius
            blur: 24
            spread: 0
            visible: true
            color: Style.themes.shadowColor
        }
        QPicture {
            y: 16
            x: 16
            width: 128
            height: 128
            radius: Style.settings.cubeRadius
            source: root.cover
            MouseArea {
                anchors.fill: parent
                onClicked: picWatch.dialog(root.cover,root.title);
            }
        }
        Text {
            x: 160
            y: 24
            width: 300
            height: 32
            elide: Text.ElideRight
            color: Style.themes.fontColor
            font.bold: true
            text: root.title
            font.pixelSize: Style.settings.textH1
            verticalAlignment: Text.AlignVCenter
        }
        Text {
            id: makerText
            x: root.width - width - 64
            y: 24
            height: 32
            color: Style.themes.textColor
            text: "创建者：" + root.artist
            font.pixelSize: Style.settings.textmain
            verticalAlignment: Text.AlignVCenter
        }
        Text {
            x: 160
            y: 56
            width: root.width - 220
            height: 44
            maximumLineCount: 3
            elide: Text.ElideRight
            color: Style.themes.textColor
            text: root.descript
            wrapMode: Text.Wrap
            font.pixelSize: Style.settings.textmain
        }
        Rectangle {
            id: playInfoRectangle
            height: 36
            width: playCountRow.implicitWidth + 20
            x: root.width - playCountRow.implicitWidth - 83
            y: 100
            radius: Style.settings.labelRadius
            color: Style.themes.sideColor
            Row {
                id: playCountRow
                x: 10
                width: parent.width
                height: 36
                spacing: 8
                Text {
                    height: 36
                    text: "\uf00e"
                    font.pixelSize: Style.settings.textmain
                    font.family: iconFont.name
                    color: Style.themes.textColor
                    verticalAlignment: Text.AlignVCenter
                }
                Text {
                    height: 36
                    text: Math.floor(root.playcount / 10000) + "万  "
                    font.pixelSize: Style.settings.textmain
                    color: Style.themes.textColor
                    verticalAlignment: Text.AlignVCenter
                }
                Text {
                    height: 36
                    text: String(root.duration) + "首"
                    font.pixelSize: Style.settings.textmain
                    color: Style.themes.textColor
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
        Row {
            id: controlRow
            x: 160
            y: 100
            height: 36
            spacing: 8
            QButton {
                height: 36; width: 96
                radius: Style.settings.labelRadius
                iconCharacter: "\uf00e"
                text: "播放"
                shadowEnabled: false
                buttonColor: Style.themes.themeColor
                textColor: Style.themes.fullColor
                iconColor: Style.themes.fullColor
                onClicked: {
                    if(Options.settings.soundQuality === 0) {
                        MusicApi.getMusicInfo(MusicApi.playlistSong.get(0).hash);
                    } else if(Options.settings.soundQuality === 1) {
                        MusicApi.getMusicInfo(MusicApi.playlistSong.get(0).hashhq);
                    } else {
                        MusicApi.getMusicInfo(MusicApi.playlistSong.get(0).hashsq);
                    }
                }
            }
            SButton {
                width: 36
                height: 36
                radius: Style.settings.labelRadius
                iconCharacter: "\uf095"
                shadowEnabled: false
                buttonColor: Style.themes.sideColor
                onClicked: {
                }
            }
            SButton {
                id: favoriteButton
                width: 36
                height: 36
                radius: Style.settings.labelRadius
                iconCharacter: "\uf0c8"
                iconColor: Style.themes.textColor
                shadowEnabled: false
                buttonColor: Style.themes.sideColor
                onClicked: {
                    if (FavoritePlaylists.isFavorite(root.id, "playlist")) {
                        FavoritePlaylists.removeFavorite(root.id, "playlist");
                        mainWarn.tiped("已取消收藏", 0);
                        iconColor = Style.themes.textColor
                    } else {
                        FavoritePlaylists.addFavorite(root.id, root.title, root.artist, root.cover, root.songSource, root.duration, "playlist");
                        mainWarn.tiped("已收藏", 1);
                        iconColor = Style.themes.themeColor
                    }
                }
            }
        }
    }

    Loader {
        id: loadWidget
        active: false
        asynchronous: true
        onLoaded: {
            root.visible = true;
            root.animeOnList[root.animeType].start();
        }
    }
}