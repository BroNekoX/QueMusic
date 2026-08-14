// AnimatorWindow.qml
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects
import QueMusic 1.0

Item {
    id: root
    //color: Style.themes.primaryColor
    z: 20
    anchors.fill: parent
    //border.color: Style.themes.secondaryColor
    visible: false

    property string id: ""
    property string cover: "qrc:/QueMusic/resources/app/musicpic.png"
    property string title: "MusicFolder"
    property string artist: "Artist"
    property string descript: "Description"
    property int duration: 0
    property int playcount: 0
    property int standTopMargin: 184

    property int winIndex: 1
    property var mainTarget

    property int songSource: MusicApi.songSource
    default property alias content: loadWidget.sourceComponent



    function opened(info) {
        root.id = info.hash || info.id;
        root.title = info.title || "";
        root.artist = info.artist || "";
        root.cover = info.cover.replace("{size}", "128") || "qrc:/QueMusic/resources/app/musicpic.png";
        root.descript = info.album || "Not have Description";
        root.duration = info.duration || 0;//|| parseInt(info.duration);
        root.playcount = info.playcount || 0; //|| parseInt(info.playcount);
        loadWidget.active = true;
        if (favoritesList.isFavorite(root.id, "playlist")) {
            favoriteButton.iconColor = Style.themes.themeColor
        } else {
            favoriteButton.iconColor = Style.themes.textColor
        }
    }
    function closed() {
        windowOpenAnime.running = false;
        mainTarget.visible = true;
        windowCloseAnime.running = true;
        window.exitIndex -= 1;
    }
    Connections {
        target: window
        enabled: root.visible
        function onExit() {
            if(window.exitIndex <= root.winIndex) {
                windowOpenAnime.running = false;
                root.mainTarget.visible = true;
                windowCloseAnime.running = true;
            }
        }
    }

    SequentialAnimation {
        id: windowOpenAnime
    ParallelAnimation {
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
            target: mainTarget
            property: "scale"
            from: 1
            to: 1.1
            duration: 100
        }
        NumberAnimation {
            target: mainTarget
            property: "opacity"
            from: 1
            to: 0
            duration: 100
        }
    }
    ScriptAction {
        script: root.mainTarget.visible = false
    }
    }
    SequentialAnimation {
        id: windowCloseAnime
    ParallelAnimation {
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
            target: mainTarget
            property: "scale"
            from: 1.16
            to: 1
            easing.type: Easing.OutExpo
            duration: 240
        }
        NumberAnimation {
            target: mainTarget
            property: "opacity"
            from: 0
            to: 1
            easing.type: Easing.OutExpo
            duration: 240
        }
    }
    ScriptAction {
        script: {
            loadWidget.active = false
            root.visible = false
        }
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
            // title
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
            // 描述
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
            //verticalAlignment: Text.AlignVCenter
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
                    if (favoritesList.isFavorite(root.id, "playlist")) {
                        favoritesList.removeFavorite(root.id, "playlist");
                        mainWarn.tiped("取消收藏",0);
                        iconColor = Style.themes.textColor
                    } else {
                        favoritesList.addFavorite(root.id, root.title, root.artist, root.cover, root.songSource, root.duration, "playlist");
                        mainWarn.tiped("成功收藏",1);
                        iconColor = Style.themes.themeColor
                    }
                }
            }
        }
    }

    Loader {
        id: loadWidget
        active: false
        onLoaded: {
            root.visible = true
            windowOpenAnime.start()
        }
    }
}