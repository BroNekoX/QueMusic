// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects
import QueMusic 1.0

//底部控制栏
Rectangle {
    id: musicControlMin
    y: parent.height - 78 + musicControlMax.hideHeight
    height: 78
    color: Style.settings.noOpacityControl ? Style.primaryColor : Style.primaryBlurColor
    clip: false
    property int musicInfoX: 100

    // 宿主注入：播放引擎不再靠上下文继承访问宿主的局部 id
    readonly property AudioEngine player: Playback.player

    readonly property string mediaTime: {
        const seconds = Math.floor(Playback.player.position / 1000) % 60;
        return Math.floor(Playback.player.position / 60000) + ':' + (seconds < 10 ? '0' + seconds : seconds);
      }

    Connections {
        target: Options.queue
        function onPlayListIndexChanged(): void {
            if(Options.queue.playListIndex < 0)
                return;
            likeButton.iconColor = FavoriteSongs.isFavorite(Options.queue.get(Options.queue.playListIndex).path, "song")
                                   ? Style.themeColor : Style.textColor;
        }
    }

    Rectangle {
        width: musicControlMin.width
        height: 1
        color: Style.sideColor
    }

    function parseArtists(raw: string): var {
        const parts = raw.split(/\s*[\/、,，&;&；]\s*/);
        const list = [];
        list.push("搜索")
        for(let i = 0; i < parts.length; i++) {
            const s = parts[i].trim();
            if(s && list.indexOf(s) === -1) {
                list.push(s);
            }
        }
        return list;
    }

    // 统一搜索入口
    function doSearchSongsMessage(name: string): void {
        MusicApi.searchSongsResults.clear();
        mainSearchInput.text = name;
        MusicApi.nowIndex = 0;
        mainContent.contentIndexed(6);
        MusicApi.searchSongs(name, MusicApi.nowIndex, 1, 20);
        Options.exitIndex = 1;
    }

    //控制条
    Item {
        id: sliderControl
        visible: Playback.player.onMedia
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
                GradientStop { position: 1.0; color: Style.textColor }
            }

        }

        Slider {
            z: 2
            id: progressSlider
            anchors.fill: parent
            width: musicControlMin.width
            from: 0
            to: Playback.player.duration > 0 ? Playback.player.duration : 1 // 避免除零错误
            value: pressed ? null : Playback.player.position
            live: true
            padding: 0


            // 关键：用户拖动时，跳转播放位置
            onMoved: {
                Playback.player.position = value
            }

            // 背景轨道
            background: Rectangle {
                y: progressSlider.hovered ? 8 : 10
                x: 0
                width: musicControlMin.width
                height: progressSlider.hovered ? 6 : 2
                color: Style.sideColor

                // 已完成部分
                Rectangle {
                    width: progressSlider.visualPosition * sliderControl.width
                    height: parent.height
                    color: Style.themeColor
                }
            }

            // 手柄
            handle: Rectangle {
                visible: progressSlider.hovered// ? 1 : 0
                x: progressSlider.leftPadding + progressSlider.visualPosition * (progressSlider.availableWidth-width)
                y: 2
                implicitWidth: 18
                implicitHeight: 18
                radius: 9
                color: "#ffffff"
                border.color: Style.themeColor
                border.width: 2.5
                ToolTip {
                    visible: parent.visible
                    text: musicControlMin.mediaTime
                    horizontalPadding: 10
                    background: Rectangle {
                        anchors.fill: parent
                        color: "#fcfdff"
                        border.width: 2
                        radius: height
                        border.color: "#cccdcf"
                    }
                }
            }
        }
    }

    //音乐信息
    Item {
        id: musicinfo
        x: 0//musicControlMin.musicInfoX
        y: 14
        z: 1
        clip: false
        width: 300
        height: 50
        Item {
            id: musicpic
            z: 5
            width: 50
            height: 50
            clip: false
            x: 24
            property int radius: 12
            Image {
                id: sourcepic
                anchors.fill: musicpic
                fillMode: Image.PreserveAspectCrop
                visible: false
                source: Playback.player.urlStr || "qrc:/QueMusic/resources/app/musicpic.png"
                sourceSize: Qt.size(64, 64)
            }
            Rectangle {
                id: maskpic
                anchors.fill: musicpic
                color: "#ff000000"
                radius: musicpic.radius
                layer.enabled: true
                visible: false
            }
            MultiEffect {
                z: 1
                anchors.fill: musicpic
                source: sourcepic
                maskEnabled: true
                maskSource: maskpic
                maskThresholdMin: 0.5
                maskSpreadAtMin: 1.0
            }
        }

        Text {
            id: titleDisplay
            y: 0
            x: 88
            width: 136
            elide: Text.ElideRight
            height: 25
            text: Playback.musicTitle
            font.bold: true
            font.pixelSize: 15
            verticalAlignment: Text.AlignVCenter
            color: titleDisplayMouse.containsMouse ? Style.themeColor : Style.textColor

            MouseArea {
                id: titleDisplayMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                onClicked: (mouse) => {
                    if (mouse.button === Qt.RightButton) {
                        // 右键保留原有搜索菜单
                        if(!Playback.musicTitle)  return;
                        titleMenu.popup();
                        return;
                    }
                    if(musicControlMax.visible) {
                        openMaxLyric.running = false;
                        closeMaxLyric.running = true;
                    } else {
                        closeMaxLyric.running = false;
                        openMaxLyric.running = true;
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
                onClicked: (index) => {
                    musicControlMin.doSearchSongsMessage(Playback.musicTitle);
                }
            }
        }
        Text {
            id: artistDisplay
            y: 25
            x: 88
            width: 136
            elide: Text.ElideRight
            height: 25
            text: Playback.musicArtist
            font.bold: false
            font.pixelSize: 13
            verticalAlignment: Text.AlignVCenter
            color: artistDisplayMouse.containsMouse ? Style.themeColor : Style.textColor

            MouseArea {
                id: artistDisplayMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                onClicked: (mouse) => {
                    if (mouse.button === Qt.RightButton) {
                        // 右键保留原有搜索菜单（多歌手选择）
                        if(!Playback.musicArtist)  return; //本地音乐没有歌手信息时，忽略
                        const artists = musicControlMin.parseArtists(Playback.musicArtist);
                        artistMenu.model = artists;// 多歌手,弹菜单
                        artistMenu.popup();
                        return;
                    }
                    if(musicControlMax.visible) {
                        openMaxLyric.running = false;
                        closeMaxLyric.running = true;
                    } else {
                        closeMaxLyric.running = false;
                        openMaxLyric.running = true;
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
                onClicked: (index) => {
                    if(index === 0) {
                        musicControlMin.doSearchSongsMessage(Playback.musicArtist);
                    } else {
                        musicControlMin.doSearchSongsMessage(model[index]);
                    }
                }
            }
        }
        SButton {
            id: likeButton
            x: 224
            y: 5
            iconCharacter: "\uf0c8"
            width: 40
            height: 40
            radius: 40
            buttonColor: "transparent"
            hoverColor: Style.hoverColor
            iconColor: Style.textColor
            shadowEnabled: false
            onClicked: {
                if(Options.queue.get(Options.queue.playListIndex).source !== -1) {
                    console.log("收藏的hash/id:",Options.queue.get(Options.queue.playListIndex).path);
                    if (FavoriteSongs.isFavorite(Options.queue.get(Options.queue.playListIndex).path, "song")) {
                        FavoriteSongs.removeFavorite(Options.queue.get(Options.queue.playListIndex).path, "song");
                        Options.warn.tiped("已取消收藏", 0);
                        iconColor = Style.textColor;
                    } else {
FavoriteSongs.addFavorite(Options.queue.get(Options.queue.playListIndex).path, Playback.musicTitle, Playback.musicArtist, Playback.player.urlStr, Options.queue.get(Options.queue.playListIndex).source, Math.floor(Playback.player.duration / 1000), "song");
                        Options.warn.tiped("已收藏", 1);
                        iconColor = Style.themeColor;
                    }
                }
            }
            tipText: "收藏"
        }
        SButton {
            x: 266
            y: 5
            iconCharacter: "\uf011"
            width: 40
            height: 40
            radius: 40
            buttonColor: "transparent"
            hoverColor: Style.hoverColor
            iconColor: Style.textColor
            shadowEnabled: false
            visible: Options.queue.count > 0 && Options.queue.playListIndex >= 0
                     && Options.queue.playListIndex < Options.queue.count
                     && Options.queue.get(Options.queue.playListIndex).source !== -1
            onClicked: {
                if(Options.queue.get(Options.queue.playListIndex).path) {
                    // 音质交给 MusicApi 按设置选 hash，这里无需再分支
                    MusicApi.getMusicInfo(Options.queue.get(Options.queue.playListIndex).path,1);
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
            iconCharacter: Playback.cycleIcon
            width: 46
            height: 46
            radius: 46
            buttonColor: "transparent"
            hoverColor: Style.hoverColor
            iconColor: Style.textColor
            shadowEnabled: false
            iconSize: Style.settings.texticon + 1
            onClicked: {
                if(Options.settings.cycleIndex < 3) {
                    Options.settings.cycleIndex += 1;
                } else {
                    Options.settings.cycleIndex = 0;
                }
            }
            tipText: Playback.cycleTip
        }
        SButton {
            iconCharacter: "\uf0dc"
            width: 46
            height: 46
            radius: 46
            buttonColor: "transparent"
            hoverColor: Style.hoverColor
            iconColor: Style.textColor
            iconSize: Style.settings.texticonH
            shadowEnabled: false
            onClicked: Playback.previous()
            tipText: "上一首"
        }
        SButton {
            iconCharacter: Playback.player.playing ? "\uf02f" : "\uf00e"
            width: 46
            height: 46
            radius: 46
            buttonColor: Style.secondaryBlurColor
            hoverColor: Style.hoverColor
            iconColor: Style.textColor
            iconSize: Style.settings.texticonH
            shadowEnabled: false
            onClicked: Playback.togglePlay()
            tipText: Playback.player.playing ? "暂停" : "播放"
        }
        SButton {
            iconCharacter: "\uf0d9"
            width: 46
            height: 46
            radius: 46
            buttonColor: "transparent"
            hoverColor: Style.hoverColor
            iconColor: Style.textColor
            iconSize: Style.settings.texticonH
            shadowEnabled: false
            onClicked: Playback.next(false)
            tipText: "下一首"
        }
        SButton {
            iconCharacter: "\uf0d0"
            width: 46
            height: 46
            radius: 46
            buttonColor: "transparent"
            hoverColor: Style.hoverColor
            iconColor: Style.textColor
            iconSize: Style.settings.texticon + 1
            shadowEnabled: false
            onClicked: musicControlMin.openPlayerOptions()
            tipText: "播放器控制"
        }

    }

    //右侧栏
    Row {
        id: playerRow
        anchors.right: parent.right
        anchors.rightMargin: 24
        spacing: 2
        y: 20
        z: 2
        height: 40
        clip: false

        // 扩展点：底栏（时间之前）
        PluginSlot { slotName: "player"; target: playerRow }

        Text {
            height: 40
            width: 80
            text: musicControlMin.mediaTime + " / " + Playback.fmt(Playback.player.duration)
            font.bold: false
            font.pixelSize: 14
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            color: Style.textColor
        }
        SButton {
            iconCharacter: "\uf0b6"
            width: 40
            height: 40
            radius: 40
            buttonColor: "transparent"
            hoverColor: Style.hoverColor
            iconColor: Style.textColor
            iconSize: Style.settings.texticon + 1
            shadowEnabled: false
            onClicked: {
                if(musicInfo.visible) {
                    musicInfo.close();
                } else {
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
            hoverColor: Style.hoverColor
            iconColor: Style.textColor
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
                    Playback.stepVolume(event.angleDelta.y > 0 ? Playback.volumeStep : -Playback.volumeStep)
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
            hoverColor: Style.hoverColor
            iconColor: Style.textColor
            shadowEnabled: false
            iconSize: Style.settings.texticon + 1
            onClicked: {
                if(Options.desktop.visible) {
                    Options.desktop.close()
                } else {
                    Options.desktop.open()
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
            hoverColor: Style.hoverColor
            iconColor: Style.textColor
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
            if(musicControlMax.visible) {
                openMaxLyric.running = false;
                closeMaxLyric.running = true;
            } else {
                closeMaxLyric.running = false;
                openMaxLyric.running = true;
            }
        }
    }

    // 收藏/取消收藏当前曲目
    function toggleFavorite(): void {
        const i = Options.queue.playListIndex;
        if(i < 0 || i >= Options.queue.count) return;
        const e = Options.queue.get(i);
        if(e.source === -1) {
            Options.warn.tiped("本地歌曲请使用本地收藏", 0);
            return;
        }
        if(FavoriteSongs.isFavorite(e.path, "song")) {
            FavoriteSongs.removeFavorite(e.path, "song");
            likeButton.iconColor = Style.textColor;
            Options.warn.tiped("已取消收藏", 0);
        } else {
            FavoriteSongs.addFavorite(e.path, Playback.musicTitle, Playback.musicArtist, Playback.player.urlStr,
                                      e.source, Math.floor(Playback.player.duration / 1000), "song");
            likeButton.iconColor = Style.themeColor;
            Options.warn.tiped("已收藏", 1);
        }
    }

    function openPlayerOptions(): void { playerOptions.open() }

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
        model: Options.queue
    }

    PlayerOptions {
        id: playerOptions
    }

    MusicInfo {
        id: musicInfo
    }

    QOptionDialog {
        id: optionsEQ
        title: "音频工作台"
        width: 640
        headerHeight: 60
        header: QTapBar {
            id: eqTab
            y: 36
            height: 32
            model: ["音频处理", "声道", "增益与动态"]
            onIndexChanged: optionsEQ.goTop();
        }

        options: EqualizerPanel {
            width: parent.width
            height: 870
            controlWidth: parent.width
            display: eqTab.index + 1
            engine: player
        }
    }
}
