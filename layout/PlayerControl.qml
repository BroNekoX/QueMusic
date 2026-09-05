// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Controls.Basic
import QueMusic 1.0
import 'qrc:/QueMusic/components'

//底部控制栏
Rectangle {
    id: musicControlMin
    y: parent.height - 78 + controlMaxLoader.hideHeight
    height: 78
    color: Style.themes.primaryBlurColor
    clip: false
    property int musicInfoX: 100

    readonly property string mediaTime: Playback.fmt(mainMedia.position)
    Connections {
        target: playListModel
        function onPlayListIndexChanged() {
            // 空队列时 playListIndex 为 -1，get 会返回 undefined
            if(playListModel.playListIndex < 0)
                return;
            likeButton.iconColor = favoritesSong.isFavorite(playListModel.get(playListModel.playListIndex).path, "song")
                                   ? Style.themes.themeColor : Style.themes.textColor;
        }
    }

    Rectangle {
        width: musicControlMin.width
        height: 1
        color: Style.themes.sideColor
    }

    // 拆分多歌手，覆盖常见分隔符：/ 、 ， , & ; ；(不含空格，避免拆坏英文歌手名)
    function parseArtists(raw) {
        var parts = raw.split(/\s*[\/、,，&;&；]\s*/);
        var list = [];
        list.push("搜索")
        for(var i = 0; i < parts.length; i++) {
            var s = parts[i].trim();
            if(s && list.indexOf(s) === -1) {
                list.push(s);
            }
        }
        return list;
    }

    // 统一搜索入口
    function doSearchSongsMessage(name) {
        MusicApi.searchSongsResults.clear();
        mainSearchInput.text = name;
        MusicApi.nowIndex = 0;
        mainContent.contentIndexed(6);
        MusicApi.searchSongs(name, MusicApi.nowIndex, 1, 20);
        window.exitIndex = 1;
    }

    //控制条
    Item {
        id: sliderControl
        visible: mainMedia.onMedia
        x: 0
        y: -10
        z: 6
        width: musicControlMin.width
        height: 23
        clip: false
        Rectangle {
            z: 1
            x: 0
            y: -80
            height: 92
            width: musicControlMin.width
            opacity: progressSlider.hovered ? 0.2 : 0
            Behavior on opacity { NumberAnimation { duration: 100 } }

            gradient: Gradient {
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 1.0; color: Style.themes.textColor }
            }

        }

        Slider {
            z: 2
            id: progressSlider
            anchors.fill: parent
            width: musicControlMin.width
            from: 0
            to: mainMedia.duration > 0 ? mainMedia.duration : 1 // 避免除零错误
            value: pressed ? null : mainMedia.position
            live: true
            padding: 0


            // 关键：用户拖动时，跳转播放位置
            onMoved: {
                mainMedia.position = value
            }

            // 可选：在滑块手柄上显示预览时间
            ToolTip {
                parent: progressSlider.handle
                visible: progressSlider.pressed
                text: musicControlMin.mediaTime
                horizontalPadding: 8
                background: Rectangle {
                    anchors.fill: parent
                    color: Style.themes.primaryColor
                    border.width: 2
                    radius: height
                    border.color: Style.themes.secondaryColor
                }
            }
            // 背景轨道
            background: Rectangle {
                y: progressSlider.hovered ? 8 : 10
                x: 0
                width: musicControlMin.width
                height: progressSlider.hovered ? 6 : 2
                radius: 0
                color: Style.themes.sideColor

                // 已完成部分
                Rectangle {
                    width: progressSlider.visualPosition * sliderControl.width
                    height: parent.height
                    color: Style.themes.themeColor
                    radius: 0
                }
            }

            // 手柄
            handle: Rectangle {
                visible: progressSlider.hovered// ? 1 : 0
                x: progressSlider.leftPadding + progressSlider.visualPosition * (progressSlider.availableWidth-width)
                y: 2
                implicitWidth: 18
                implicitHeight: 18
                radius: 18
                color: Style.themes.primaryColor
                border.color: Style.themes.themeColor
                border.width: 3
            }
        }
    }

    //音乐信息
    Item {
        id: musicinfo
        x: musicControlMin.musicInfoX
        y: 14
        z: 1
        clip: false
        width: 200
        height: 50

        Text {
            id: titleDisplay
            y: 0
            x: 0
            width: 128
            elide: Text.ElideRight
            height: 25
            text: window.musicTitle
            font.bold: true
            font.pixelSize: 15
            verticalAlignment: Text.AlignVCenter
            color: titleDisplayMouse.containsMouse ? Style.themes.themeColor : Style.themes.textColor

            MouseArea {
                id: titleDisplayMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                onClicked: (mouse) => {
                    if (mouse.button === Qt.RightButton) {
                        // 右键保留原有搜索菜单
                        if(!window.musicTitle)  return;
                        titleMenu.popup();
                        return;
                    }
                    // 左键与控制栏整体点击行为一致——底部栏打开播放页，播放页点播放栏关闭
                    if(mainLayout.state === "") {
                        controlMaxLoader.active = true;
                    } else {
                        window.playermined();
                        minedAnimation.start();
                        mainLayout.state = "";
                    }
                }
                QTip {
                    visible: titleDisplayMouse.containsMouse
                    text: "右键搜索"
                }
            }
            // 右键弹出菜单再搜索，左键直接打开全屏播放器
            QMenu {
                id: titleMenu
                model: ["搜索歌曲名"]
                masked: true
                blurSource: null // 位置特殊，关闭模糊效果
                onClicked: (index) => {
                    musicControlMin.doSearchSongsMessage(window.musicTitle);
                }
            }
        }
        Text {
            id: artistDisplay
            y: 25
            x: 0
            width: 128
            elide: Text.ElideRight
            height: 25
            text: window.musicArtist
            font.bold: false
            font.pixelSize: 13
            verticalAlignment: Text.AlignVCenter
            color: artistDisplayMouse.containsMouse ? Style.themes.themeColor : Style.themes.textColor

            MouseArea {
                id: artistDisplayMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                onClicked: (mouse) => {
                    if (mouse.button === Qt.RightButton) {
                        // 右键保留原有搜索菜单（多歌手选择）
                        if(!window.musicArtist)  return; //本地音乐没有歌手信息时，忽略
                        var artists = musicControlMin.parseArtists(window.musicArtist);
                        artistMenu.model = artists;// 多歌手,弹菜单
                        artistMenu.popup();
                        return;
                    }
                    // 左键与控制栏整体点击行为一致：底部栏态打开播放页，播放页点同一位置关闭
                    if(mainLayout.state === "") {
                        controlMaxLoader.active = true;
                    } else {
                        window.playermined();
                        minedAnimation.start();
                        mainLayout.state = "";
                    }
                }
                QTip {
                    visible: artistDisplayMouse.containsMouse
                    text: "右键搜索"
                }
            }
            //多位歌手时，显示菜单
            QMenu {
                id: artistMenu
                model: []
                masked: true
                blurSource: null // 位置特殊，关闭模糊效果
                onClicked: (index) => {
                    if(index === 0) {
                        musicControlMin.doSearchSongsMessage(window.musicArtist);
                    } else {
                        musicControlMin.doSearchSongsMessage(model[index]);
                    }
                }
            }
        }
        SButton {
            id: likeButton
            x: 132
            y: 5
            iconCharacter: "\uf0c8"
            width: 40
            height: 40
            radius: 40
            buttonColor: "transparent"
            hoverColor: Style.themes.hoverColor
            iconColor: Style.themes.textColor
            shadowEnabled: false
            onClicked: {
                if(playListModel.get(playListModel.playListIndex).source !== -1) {
                    console.log("收藏的hash/id:",playListModel.get(playListModel.playListIndex).path);
                    if (favoritesSong.isFavorite(playListModel.get(playListModel.playListIndex).path, "song")) {
                        favoritesSong.removeFavorite(playListModel.get(playListModel.playListIndex).path, "song");
                        mainWarn.tiped("取消收藏",0);
                        iconColor = Style.themes.textColor;
                    } else {
                        favoritesSong.addFavorite(playListModel.get(playListModel.playListIndex).path, window.musicTitle, window.musicArtist, mainMedia.urlStr, playListModel.get(playListModel.playListIndex).source, Math.floor(mainMedia.duration / 1000), "song");
                        mainWarn.tiped("成功收藏",1);
                        iconColor = Style.themes.themeColor;
                    }
                }
            }
            tipText: "收藏"
        }
        SButton {
            x: 174
            y: 5
            //iconSize:
            iconCharacter: "\uf011"
            width: 40
            height: 40
            radius: 40
            buttonColor: "transparent"
            hoverColor: Style.themes.hoverColor
            iconColor: Style.themes.textColor
            shadowEnabled: false
            visible: playListModel.count > 0 && playListModel.playListIndex >= 0
                     && playListModel.playListIndex < playListModel.count
                     && playListModel.get(playListModel.playListIndex).source !== -1
            onClicked: {
                if(playListModel.get(playListModel.playListIndex).path) {
                    if(Options.settings.soundQuality === 0) {
                        MusicApi.getMusicInfo(playListModel.get(playListModel.playListIndex).path,1);
                    } else if(Options.settings.soundQuality === 1) {
                        MusicApi.getMusicInfo(playListModel.get(playListModel.playListIndex).path,1);
                    } else {
                        MusicApi.getMusicInfo(playListModel.get(playListModel.playListIndex).path,1);
                    }
                }
            }
            tipText: "下载"
        }

    }

    //中间控制
    Row {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 16
        z: 3
        height: 46
        spacing: 4
        SButton {
            iconCharacter: ["\uf118","\uf115","\uf0e2","\uf03b"][Options.settings.cycleIndex]
            width: 46
            height: 46
            radius: 46
            buttonColor: "transparent"
            hoverColor: Style.themes.hoverColor
            iconColor: Style.themes.textColor
            shadowEnabled: false
            iconSize: Style.settings.texticon + 1
            onClicked: {
                if(Options.settings.cycleIndex < 3) {
                    Options.settings.cycleIndex += 1;
                } else {
                    Options.settings.cycleIndex = 0;
                }
            }
            tipText: "播放顺序"
        }
        SButton {
            iconCharacter: "\uf0dc"
            width: 46
            height: 46
            radius: 46
            buttonColor: "transparent"
            hoverColor: Style.themes.hoverColor
            iconColor: Style.themes.textColor
            iconSize: Style.settings.texticonH
            shadowEnabled: false
            onClicked: musicControlMin.lastMedia()
            tipText: "上一首"
        }
        SButton {
            iconCharacter: mainMedia.playing ? "\uf02f" : "\uf00e"
            width: 46
            height: 46
            radius: 46
            buttonColor: Style.themes.secondaryBlurColor
            hoverColor: Style.themes.hoverColor
            iconColor: Style.themes.textColor
            iconSize: Style.settings.texticonH
            shadowEnabled: false
            onClicked: Playback.togglePlay()
            tipText: mainMedia.playing ? "暂停" : "播放"
        }
        SButton {
            iconCharacter: "\uf0d9"
            width: 46
            height: 46
            radius: 46
            buttonColor: "transparent"
            hoverColor: Style.themes.hoverColor
            iconColor: Style.themes.textColor
            iconSize: Style.settings.texticonH
            shadowEnabled: false
            onClicked: musicControlMin.enterMedia()
            tipText: "下一首"
        }
        SButton {
            iconCharacter: "\uf0d0"
            width: 46
            height: 46
            radius: 46
            buttonColor: "transparent"
            hoverColor: Style.themes.hoverColor
            iconColor: Style.themes.textColor
            iconSize: Style.settings.texticon + 1
            shadowEnabled: false
            onClicked: musicControlMin.openPlayerOptions()
            tipText: "播放器控制"
        }

    }

    //右侧栏
    Row {
        anchors.right: parent.right
        anchors.rightMargin: 24
        spacing: 2
        y: 20
        z: 2
        height: 40
        clip: false

        Label {
            height: 40
            width: 80
            text: musicControlMin.mediaTime + " / " + Playback.fmt(mainMedia.duration)
            font.bold: false
            font.pixelSize: 14
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            color: Style.themes.textColor
        }
        SButton {
            iconCharacter: "\uf0b6"
            width: 40
            height: 40
            radius: 40
            buttonColor: "transparent"
            hoverColor: Style.themes.hoverColor
            iconColor: Style.themes.textColor
            iconSize: Style.settings.texticon + 1
            shadowEnabled: false
            onClicked: {
                if(musicInfo.visible) {
                    //playerInfoDialog.close();
                    musicInfo.close();
                } else {
                    //playerInfoDialog.open();
                    musicInfo.open();
                }
            }
            tipText: "音乐详情"
        }
        SButton {
            iconCharacter: "\uf043"
            width: 40
            height: 40
            radius: 40
            buttonColor: "transparent"
            hoverColor: Style.themes.hoverColor
            iconColor: Style.themes.textColor
            iconSize: Style.settings.texticon + 2
            shadowEnabled: false
            onHoveredChanged: {
                if(hovered) {
                    volumeControl.delay = 360
                    volumeControl.open();
                } else {
                    if(!volumeControl.visible) {
                        volumeControl.close();
                    }
                }
            }
            onClicked: {
                if(volumeControl.visible) {
                    volumeControl.close();
                } else {
                    volumeControl.delay = 0
                    volumeControl.open();
                }
            }
            WheelHandler {
                onWheel: (event) => {
                    Playback.stepVolume(event.angleDelta.y > 0 ? 0.05 : -0.05)
                    event.accepted = true
                }
            }
        }
        SButton {
            iconCharacter: "\uf0b2"
            width: 40
            height: 40
            radius: 40
            buttonColor: "transparent"
            hoverColor: Style.themes.hoverColor
            iconColor: Style.themes.textColor
            shadowEnabled: false
            iconSize: Style.settings.texticon + 1
            onClicked: {
                if(desktopPlayer.visible) {
                    desktopPlayer.close()
                } else {
                    desktopPlayer.open()
                }
            }
            tipText: "桌面部件"
        }
        SButton {
            iconCharacter: "\uf098"
            width: 40
            height: 40
            radius: 40
            buttonColor: "transparent"
            hoverColor: Style.themes.hoverColor
            iconColor: Style.themes.textColor
            iconSize: Style.settings.texticon + 2
            shadowEnabled: false
            onClicked: {
                if(playList.visible) {
                    playList.close();
                } else {
                    playList.open();
                }
            }
            tipText: "播放列表"
        }
    }

    MouseArea {
        z: 0
        anchors.fill: parent
        onClicked: {
            if(mainLayout.state === "") {
                controlMaxLoader.active = true;
            } else {
                window.playermined();
                minedAnimation.start();
                mainLayout.state = "";
            }
        }
    }

    // 收藏/取消收藏当前曲目
    function toggleFavorite() {
        var i = playListModel.playListIndex;
        if(i < 0 || i >= playListModel.count) return;
        var e = playListModel.get(i);
        if(e.source === -1) {
            mainWarn.tiped("本地歌曲请使用本地收藏", 0);
            return;
        }
        if(favoritesSong.isFavorite(e.path, "song")) {
            favoritesSong.removeFavorite(e.path, "song");
            likeButton.iconColor = Style.themes.textColor;
            mainWarn.tiped("取消收藏", 0);
        } else {
            favoritesSong.addFavorite(e.path, window.musicTitle, window.musicArtist, mainMedia.urlStr,
                                      e.source, Math.floor(mainMedia.duration / 1000), "song");
            likeButton.iconColor = Style.themes.themeColor;
            mainWarn.tiped("成功收藏", 1);
        }
    }

    function openPlayerOptions() { playerOptions.open() }

    // 上一首
    function lastMedia() { Playback.previous() }
    // 下一首
    function enterMedia() { Playback.next(false) }
    // 随机播放（洗牌牌堆，避免最近播放）
    function randomMedia() { Playback.next(true) }
    // 切换播放列表显示
    function togglePlayList() {
        if(playList.visible) {
            playList.close();
        } else {
            playList.open();
        }
    }

    ToolTip {
        id: volumeControl
        margins: 0
        parent: Overlay.overlay
        width: 180
        height: 40
        verticalPadding: 5
        leftPadding: 10
        rightPadding: 40
        delay: 360
        closePolicy: Popup.CloseOnPressOutside
        x: parent.width - 230
        y: parent.height - 110
        background: QBlurCard {
            anchors.fill: parent
            clip: false
            blurMax: 48
            borderRadius: 23
            blurSource: mainLayout
            shadowEffect: true
            rectXy: Qt.rect(volumeControl.x, volumeControl.y, 180, 40)
            //color: Style.themes.primaryBlurColor
        }
        contentItem: QSlider {
            z: 1
            to: 100
            implicitWidth: 130
            implicitHeight: 36
            valueText: Math.floor(value)
            value: Options.settings.musicVolume * 100
            onMoved: {
                Options.settings.musicVolume = value / 100
            }
        }
        enter: Transition {
            NumberAnimation { property: "y"; duration: 320; from: volumeControl.parent.height - 88; to: volumeControl.parent.height - 110; easing.type: Easing.OutExpo }
            NumberAnimation { property: "opacity"; duration: 320; from: 0; to: 1; easing.type: Easing.OutExpo }
        }
        exit: Transition {
            NumberAnimation { property: "y"; duration: 160; to: volumeControl.parent.height - 88; easing.type: Easing.InCubic }
            NumberAnimation { property: "opacity"; duration: 160; to: 0; easing.type: Easing.InCubic }
        }
    }

    PlayList {
        id: playList
        model: playListModel
    }

    PlayerOptions {
        id: playerOptions
    }

    MusicInfo {
        id: musicInfo
    }
}