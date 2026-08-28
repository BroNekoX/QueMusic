// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Window
import QueMusic 1.0
import QtCore
import QtMultimedia
import QWindowKit 1.0
import QtQuick.Effects
import QtQuick.Controls.Basic

//import 'qrc:/QueMusic/components'
//import 'qrc:/QueMusic/layout'
//import 'qrc:/QueMusic/cpp'

Window {
    id: window
    width: 1140
    height: 720
    minimumWidth: 810
    minimumHeight: 540
    //visible: true
    color: Style.themes.primaryColor
    title: "QueMusic"
    Component.onCompleted: {
        windowAgent.setup(window);
        windowAgent.setWindowAttribute("dark-mode", false);
        if(!Options.settings.noWindowKit) {
            //dwm-blur acrylic-material mica mica-alt extra-margins
        } else {
            windowAgent.setWindowAttribute("extra-margins", 3);
            windowAgent.setWindowAttribute("title-bar-height", 40);
        }
        MusicApi.songSource = Options.settings.mainMusicSource;
        MusicApi.downloadPath = Options.settings.downloadFolder;

        window.visible = true;
        MusicApi.songSource = Options.settings.mainMusicSource;
        // 更新设置项
        Style.changeUi();
        Style.changeTheme();
    }

    Connections {
        target: Options.settings
        function onDownloadFolderChanged() {
            MusicApi.downloadPath = Options.settings.downloadFolder;
        }
    }
    property string musicTitle: "QueMusic"
    property string musicArtist: "Artist"
    property int exitIndex: 0
    property string version: "Beta-0.4.1"
    property int versionCode: 41

    property string localLyricsRequestPath: ""

    Connections {
        target: MusicApi
        function onLocalLyricsReady(filePath, lyrics) {
            if (filePath !== window.localLyricsRequestPath)
                return;
            MusicApi.lyricsData = lyrics;
            MusicApi.lyricsTranslate = [];
        }
        function onLocalLyricsFailed(filePath) {
            if (filePath !== window.localLyricsRequestPath)
                return;
            MusicApi.setLocalLyrics();
        }
    }

    // 播放本地歌曲：同名 .lrc → 内嵌歌词 → 在线匹配 → 占位歌词
    function playLocalSong(path, name) {
        var meta = MusicApi.readLocalMetadata(path) || {};
        var hasMeta = Object.keys(meta).length > 0;
        var localLyrics = MusicApi.readLocalLyrics(path) || {};
        var hasLocalLyrics = localLyrics.found === true && localLyrics.lyrics && localLyrics.lyrics.length > 0;
        var title = meta.title || name;
        var artist = meta.artist || "";
        window.localLyricsRequestPath = path;
        if (hasMeta) {
            mainMedia.urlLocal = false;
            mainMedia.noTitle = title;
            mainMedia.urlStr = meta.cover || "qrc:/QueMusic/resources/app/musicpic.png";
            window.musicTitle = title;
            window.musicArtist = artist;
            MusicApi.lyricsData = hasLocalLyrics ? localLyrics.lyrics : (meta.lyrics || []);
            MusicApi.lyricsTranslate = hasLocalLyrics ? [] : (meta.translate || []);
            colorExtractor.extractColorsFromUrl(meta.cover);
        } else {
            mainMedia.urlLocal = true;
            mainMedia.noTitle = name;
            window.musicTitle = name;
            window.musicArtist = "";
            MusicApi.lyricsData = hasLocalLyrics ? localLyrics.lyrics : [];
            MusicApi.lyricsTranslate = [];
        }
        if (!hasLocalLyrics && (!hasMeta || !meta.lyrics || meta.lyrics.length === 0)) {
            MusicApi.setLocalLyrics();
            MusicApi.findLocalLyrics(path, title, artist, meta.duration || 0);
        }
        mainMedia.source = path;
        mainMedia.play();
    }

    // 首次加载内容临时存储，防止重新加载浪费内存
    property QtObject completedStart: QtObject {
        property bool homeLoaded: false
        property bool playlistLoaded: false
    }

    // 关闭前保存最后播放的歌曲
    function toClosing() {
        if(Options.settings.closeToManage) {
            window.showMinimized();
            return;
        }
        if(playListModel.count > 0 && playListModel.playListIndex >= 0) {
            var e = playListModel.get(playListModel.playListIndex);
            Options.lastSongs.name = window.musicTitle;
            Options.lastSongs.artist = window.musicArtist;
            Options.lastSongs.cover = mainMedia.urlStr || "qrc:/QueMusic/resources/app/musicpic.png";
            Options.lastSongs.hash = e.path;
            Options.lastSongs.source = e.source;
            console.log("保存当前音乐记录。");
        }
        // 清理桌面悬浮窗（灵动岛 / 小窗播放器）
        desktopSpot.active = false;
        desktopLyricsLoader.active = false;
        desktopPlayerLoader.active = false;
        window.close();
    }

    signal getKeys(var keys)
    signal exit() // 返回

    //type: 0.提示 1.警告 2.错误 3.正确
    signal message(string title,string text,int type)

    Shortcut {
        sequence: "Esc" // 返回
        context: Qt.ApplicationShortcut
        onActivated: {
            window.exit();
            console.log("Exit");
            if(window.exitIndex > 0) {
                window.exitIndex -= 1;
            }
            mainLayout.forceActiveFocus();
        }
    }
    Shortcut {
        sequence: Options.shortCuts.play // 暂停/播放
        context: Qt.ApplicationShortcut
        enabled: Options.settings.openShortCut
        onActivated: {
            console.log("shortcut--play")
            if (mainMedia.playing === false) {
                mainMedia.play();
            }
            else {
                mainMedia.pause();
            }
        }
    }
    Shortcut {
        sequence: Options.shortCuts.back // 上一首
        context: Qt.ApplicationShortcut
        enabled: Options.settings.openShortCut
        onActivated: {
            console.log("shortcut--back")
            musicControlMin.lastMedia();
        }
    }
    Shortcut {
        sequence: Options.shortCuts.forward // 下一首
        context: Qt.ApplicationShortcut
        enabled: Options.settings.openShortCut
        onActivated: {
            console.log("shortcut--forward")
            musicControlMin.enterMedia();
        }
    }
    Shortcut {
        sequence: Options.shortCuts.playList // 播放菜单
        context: Qt.ApplicationShortcut
        enabled: Options.settings.openShortCut
        onActivated: {
            console.log("shortcut--playList")
            if(playList.visible) {
                playList.close();
            } else {
                playList.open();
            }
        }
    }
    Shortcut {
        sequence: Options.shortCuts.musicControl // 播放模式切换
        context: Qt.ApplicationShortcut
        enabled: Options.settings.openShortCut
        onActivated: {
            if(mainLayout.state === "") {
                controlMaxLoader.active = true;
            } else {
                window.playermined();
                minedAnimation.start();
                mainLayout.state = "";
            }
        }
    }

    //加载icon库
    FontLoader {
        id: iconFont
        source: "qrc:/QueMusic/resources/fonts/feather.ttf"
    }
    FontLoader {
        id: textFont
        source: "qrc:/QueMusic/resources/fonts/poppins.ttf"
    }

    // QWindowKit窗口代理
    WindowAgent {
        id: windowAgent
    }

    function playermined() { barLeftWidgets.y = 12 }//{ barLeftWidgets.visible = true }
    function playermaxed() { barLeftWidgets.y = -48 }//{ barLeftWidgets.visible = false }

    // 顶部栏 - 与qwindowkit和window耦合，难抽为组件
    Rectangle {
        id: titleBar
        x: sidebar.width
        y: 0
        z: 10
        width: window.width - x
        height: 60
        color: "transparent"

        // 此组件创建时，将此组件与 qwindowkit 绑定，标题栏事件由此传入
        Component.onCompleted: windowAgent.setTitleBar(titleBar);

        Row {
            id: barLeftWidgets
            y: 12
            Behavior on y { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }
            anchors {
                left: parent.left // 靠左
                leftMargin: 16
                //verticalCenter: titleBar.verticalCenter // 内容居中
            }
            spacing: 5
            //width: 241

            QWKButton {
                id: returnButton
                source: Style.darkis ? "qrc:/QueMusic/resources/window-bar/returnd.svg" : "qrc:/QueMusic/resources/window-bar/return.svg"
                onClicked: {
                    window.exit()
                    console.log("Exit")
                    if(window.exitIndex > 0) {
                        window.exitIndex -= 1
                    }
                }
                Component.onCompleted: windowAgent.setHitTestVisible(returnButton, true);
            }   

            TextField {
                id: mainSearchInput
                x: 20
                y: 0
                height: 36
                width: 160
                //displayText: "搜索"
                leftPadding: 16
                placeholderText: "搜索"
                color: Style.themes.textColor
                font.pixelSize: Style.settings.textmain
                verticalAlignment: Text.AlignVCenter
                selectionColor: Style.themes.containColor
                focus: false
                onReleased: searchCard.open();
                //clip: true
                //onTextEdited: parent.border.color = Style.themes.themeColor
                //onEditingFinished: parent.border.color = "transparent"
                onAccepted: {
                    if(text.trim() == "") {
                        mainWarn.tiped("请输入文本>-<",0);
                        return;
                    }
                    MusicApi.searchSongsResults.clear();
                    mainContent.contentIndexed(6);
                    Options.settings.searchList = Options.settings.searchList.filter(value => value !== mainSearchInput.text);
                    Options.settings.searchList.splice(0, 0, mainSearchInput.text);
                    MusicApi.searchSongs(mainSearchInput.text,MusicApi.nowIndex,1,20);
                    window.exitIndex = 1;
                    searchCard.close();
                }
                Component.onCompleted: windowAgent.setHitTestVisible(mainSearchInput, true);
                background: Rectangle {
                    height: 36
                    width: 201
                    radius: 18
                    color: Style.themes.primaryColor//Style.themes.secondaryColor
                }
            }
            SButton {
                id: searchButton
                width: 36
                height: 36
                radius: 18
                iconCharacter: "\uf100"
                buttonColor: "transparent"
                onClicked: {
                    if(mainSearchInput.text.trim() == "") {
                        mainWarn.tiped("请输入文本>-<",0);
                        return;
                    }
                    MusicApi.searchSongsResults.clear();
                    mainContent.contentIndexed(6);
                    Options.settings.searchList = Options.settings.searchList.filter(value => value !== mainSearchInput.text);
                    Options.settings.searchList.splice(0, 0, mainSearchInput.text);
                    MusicApi.searchSongs(mainSearchInput.text,MusicApi.nowIndex,1,20);
                    window.exitIndex = 1;
                    searchCard.close();
                }
                Component.onCompleted: windowAgent.setHitTestVisible(searchButton, true);
            }
        }

        // 窗口按钮
        Row {
            anchors {
                right: parent.right // 靠右
                rightMargin: 16
            }
            spacing: 0
            y: 10 - controlMaxLoader.hideHeight
            height: 40

            //QWKButton {
                //id: accountButton
                //largeicon: true
                //source: ""
                //onClicked: window.account()
                //Component.onCompleted: windowAgent.setHitTestVisible(accountButton, true);
            //}

            QWKButton {
                id: fullDesktopButton
                largeicon: true
                source: Style.darkis || mainLayout.state !== "" ? "qrc:/QueMusic/resources/window-bar/airplayd.svg" : "qrc:/QueMusic/resources/window-bar/airplay.svg"
                onClicked: {
                    mainMessage.dialog("Error Dialog","本功能未开发完成，无法使用。","\uf11a");
                }
                Component.onCompleted: windowAgent.setHitTestVisible(fullDesktopButton, true);
            }

            QWKButton {
                id: settingButton
                largeicon: true
                source: Style.darkis || mainLayout.state !== "" ? "qrc:/QueMusic/resources/window-bar/settingd.svg" : "qrc:/QueMusic/resources/window-bar/setting.svg"
                onClicked: {
                    settingsView.active = true;
                }
                Component.onCompleted: windowAgent.setHitTestVisible(settingButton, true);
            }

            QWKButton {
                id: minButton
                source: Style.darkis || mainLayout.state !== "" ? "qrc:/QueMusic/resources/window-bar/minimized.svg" : "qrc:/QueMusic/resources/window-bar/minimize.svg"
                onClicked: window.showMinimized();
                Component.onCompleted: windowAgent.setSystemButton(WindowAgent.Minimize, minButton);
                //Component.onCompleted: windowAgent.setHitTestVisible(minButton, true);
            }

            QWKButton {
                readonly property string maximized: Style.darkis || mainLayout.state !== "" ? "qrc:/QueMusic/resources/window-bar/maximized.svg" : "qrc:/QueMusic/resources/window-bar/maximize.svg"
                readonly property string restored: Style.darkis || mainLayout.state !== "" ? "qrc:/QueMusic/resources/window-bar/restored.svg" : "qrc:/QueMusic/resources/window-bar/restore.svg"
                id: maxButton
                source: window.visibility === Window.Maximized ? restored : maximized
                onClicked: {
                    if (window.visibility === Window.Maximized) {
                        window.showNormal();
                    } else {
                        window.showMaximized();
                    }
                }
                Component.onCompleted: windowAgent.setSystemButton(WindowAgent.Maximize, maxButton);
                //Component.onCompleted: windowAgent.setHitTestVisible(maxButton, true);
            }

            QWKButton {
                readonly property string hover: Style.darkis ? "qrc:/QueMusic/resources/window-bar/close.svg" : "qrc:/QueMusic/resources/window-bar/closed.svg"
                readonly property string unhover: Style.darkis || mainLayout.state !== "" ? "qrc:/QueMusic/resources/window-bar/closed.svg" : "qrc:/QueMusic/resources/window-bar/close.svg"
                id: closeButton
                source: closeButton.hovered ? hover : unhover
                hoverColor: "#ee4848"
                onClicked: window.toClosing();
                Component.onCompleted: windowAgent.setSystemButton(WindowAgent.Close, closeButton);
                //Component.onCompleted: windowAgent.setHitTestVisible(closeButton, true);
            }
        }
    }

    //MainLayout
    Item {
        id: mainLayout
        anchors.fill: parent
        z: 5
        property int maxLyricType: 0
        readonly property int piclong: mainLayout.width < 1280 ? mainLayout.height / 3 + mainLayout.width / 8 - 100 : mainLayout.height / 3 + 60

        ParallelAnimation {
            id: maxedAnimation
            NumberAnimation { target: controlMaxLoader; property: "y"; duration: 320; from: mainLayout.height; to: 0; easing.type: Easing.Bezier; easing.bezierCurve: [ 0.23, 0.06, 0.00, 1.00, 1, 1 ] }
            NumberAnimation { target: musicControlMin; property: "musicInfoX"; duration: 320; to: 30; easing.type: Easing.Bezier; easing.bezierCurve: [ 0.23, 0.06, 0.00, 1.00, 1, 1 ] }
            ColorAnimation { target: musicControlMin; property:"color"; to: Style.themes.blurOverlayColor; duration: 320 }
        }
        SequentialAnimation {
            id: minedAnimation
            ParallelAnimation {
                NumberAnimation { target: controlMaxLoader; property: "y"; duration: 320; from: 0; to: mainLayout.height; easing.type: Easing.Bezier; easing.bezierCurve: [ 0.23, 0.06, 0.00, 1.00, 1, 1 ] }
                NumberAnimation { target: musicControlMin; property: "musicInfoX"; duration: 320; to: 100; easing.type: Easing.Bezier; easing.bezierCurve: [ 0.23, 0.06, 0.00, 1.00, 1, 1 ] }
                ColorAnimation { target: musicControlMin; property:"color"; to: Style.themes.primaryBlurColor; duration: 320 }
            }
            ScriptAction {
                script: {
                    controlMaxLoader.visible = false;
                    controlMaxLoader.active = false;
                    controlMaxLoader.hideHeight = 0;
                }
            }
        }

        states: [
            State {
                name: ""
                PropertyChanges { target: musicpic; x: 30; y: mainLayout.height - 64; radius: 12; height: 50; width: 50 }
                PropertyChanges { target: musicpicShadow; visible: false }
            },
            State {
                name: "MaxedCover"
                PropertyChanges { target: musicpic; x: mainLayout.width * 0.5 - (mainLayout.piclong / 2); y: mainLayout.height / 1.7 - mainLayout.piclong; radius: 24; height: mainLayout.piclong; width: mainLayout.piclong }
                PropertyChanges { target: controlMaxLoader; lyricsX: mainLayout.width; lyricsType: 1; infoX: mainLayout.width * 0.5 - (mainLayout.piclong / 2) }
                PropertyChanges { target: musicpicShadow; visible: true }
            },
            State {
                name: "MaxedNormal"
                PropertyChanges { target: musicpic; x: mainLayout.width * 0.23 - (mainLayout.piclong / 2); y: mainLayout.height / 1.7 - mainLayout.piclong; radius: 24; height: mainLayout.piclong; width: mainLayout.piclong }
                PropertyChanges { target: controlMaxLoader; lyricsX: mainLayout.width * 0.46; lyricsType: 0; infoX: mainLayout.width * 0.23 - (mainLayout.piclong / 2) }
                PropertyChanges { target: musicpicShadow; visible: true }
            },
            State {
                name: "MaxedLyric"
                PropertyChanges { target: musicpic; x: -400; y: mainLayout.height / 2; radius: 12; height: 50; width: 50 }
                PropertyChanges { target: controlMaxLoader; lyricsX: 48; lyricsType: 2; infoX: -400 }
                PropertyChanges { target: musicpicShadow; visible: false }
            }
            
        ]
        transitions: [
            Transition {
                from: ""; to: "*"
                ParallelAnimation {
                    NumberAnimation { target: musicpic; properties: "x,y,width,height"; duration: 350; easing.type: Easing.Bezier; easing.bezierCurve: [ 0.30, 0.06, 0.00, 1.12, 1, 1 ] }
                    NumberAnimation { target: musicpic; property: "radius"; duration: 350; easing.type: Easing.OutExpo }
                }
            },
            Transition {
                from: "*"; to: ""
                ParallelAnimation {
                    NumberAnimation { target: musicpic; properties: "x,y,width,height"; duration: 350; easing.type: Easing.Bezier; easing.bezierCurve: [ 0.24, 0.06, 0.00, 1.12, 1, 1 ] }//0.23, 0.04, 0.00, 1.20
                    NumberAnimation { target: musicpic; property: "radius"; duration: 350; easing.type: Easing.OutExpo }
                }
            },
            Transition {
                from: "*"; to: "*"
                ParallelAnimation {
                    NumberAnimation { target: musicpic; properties: "x,y,width,height"; duration: 350; easing.type: Easing.Bezier; easing.bezierCurve: [ 0.30, 0.06, 0.00, 1.00, 1, 1 ] }//0.23, 0.04, 0.00, 1.20
                    NumberAnimation { target: musicpic; property: "radius"; duration: 350; easing.type: Easing.OutExpo }
                    NumberAnimation { target: controlMaxLoader; property: "lyricsX"; duration: 350; easing.type: Easing.Bezier; easing.bezierCurve: [ 0.30, 0.06, 0.00, 1.00, 1, 1 ] }
                    NumberAnimation { target: controlMaxLoader; property: "infoX"; duration: 350; easing.type: Easing.Bezier; easing.bezierCurve: [ 0.30, 0.06, 0.00, 1.00, 1, 1 ] }
                    NumberAnimation { target: controlMaxLoader; property: "infoY"; duration: 350; easing.type: Easing.Bezier; easing.bezierCurve: [ 0.30, 0.06, 0.00, 1.00, 1, 1 ] }
                }
            }
        ]

        // Style变化信号统一
        Connections {
            target: Style
            function onChangeTheme() {
                windowAgent.setWindowAttribute("dwm-blur", false);
                if(Style.settings.backmode === 0) {
                    backGround.visible = false;
                    sidebar.baseColor = Style.themes.primaryColor;
                    window.color = Style.themes.primaryColor;
                    mainContent.color = Style.themes.secondaryColor;
                } else if(Style.settings.backmode === 1) {
                    backGround.visible = false;
                    sidebar.baseColor = Style.themes.primaryBlurColor;
                    window.color = Style.themes.containColor;
                    mainContent.color = Style.themes.blurOverlayColor;
                } else if(Style.settings.backmode === 2) {
                    backGround.visible = true;
                    sidebar.baseColor = Style.themes.blurOverlayColor;
                    window.color = Style.themes.primaryColor;
                    mainContent.color = Style.themes.blurOverlayColor;
                    backGround.source = "qrc:/QueMusic/resources/pic/cloudRainbow.png";
                } else if(Style.settings.backmode === 3) {
                    backGround.visible = true;
                    sidebar.baseColor = Style.themes.blurOverlayColor;
                    window.color = Style.themes.primaryColor;
                    mainContent.color = Style.themes.blurOverlayColor;
                    switch(Style.settings.backpic) {
                        case 0:
                            backGround.source = "qrc:/QueMusic/resources/pic/back1.jpg";
                            break;
                        case 1:
                            backGround.source = "qrc:/QueMusic/resources/pic/back3.jpg";
                            break;
                        case 2:
                            backGround.source = Style.settings.backgroundImage;
                            break;
                    }
                } else if(Style.settings.backmode === 4) {
                    backGround.visible = false;
                    sidebar.baseColor = Style.themes.primaryBlurColor;
                    window.color = "transparent";
                    mainContent.color = Style.themes.primaryBlurColor;
                    windowAgent.setWindowAttribute("dwm-blur", true);
                }
            }
        }
        Image {
            id: backGround
            z: 0
            x: 0
            y: 0
            width: parent.width
            height: parent.height
            visible: false
            asynchronous: true
            fillMode: Image.PreserveAspectCrop

        }

        LeftSideBar {
            z: 2
            id: sidebar
            x: 0
            y: 0
            height: parent.height - 78
            width: 210
        }

        // 主体内容区域
        MainContent {
            z: 1
            id: mainContent
            x: sidebar.width
            y: 0
            width: parent.width - x
            height: parent.height - 78
        }

        // 底部栏
        PlayerControl {
            id: musicControlMin
            x: 0
            //y: parent.height - 78
            width: parent.width
            height: 78
            z: 4
        }

        //单独分离音乐封面
        Item {
            id: musicpic
            z: 5
            width: 50
            height: 50
            clip: false
            x: 30
            opacity: controlMaxLoader.basicCd && controlMaxLoader.visible ? 0 : 1
            y: mainLayout.height - 64
            property int radius: 12
            scale: mainMedia.playing ? 1.0 : 0.84
            //layer.enabled: true
            Behavior on scale { NumberAnimation { duration: 320; easing.type: Easing.Bezier; easing.bezierCurve: [ 0.20, 0.04, 0.00, 1.64, 1, 1 ] } }
            Behavior on opacity { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }
            RectangularShadow {
                id: musicpicShadow
                anchors.fill: musicpic
                z: 0
                offset.x: 2
                offset.y: 12
                radius: 24
                blur: 32
                visible: false
                opacity: visible ? 1 : 0
                Behavior on opacity { NumberAnimation { duration: 320; easing.type: Easing.OutCubic } }
                color: "#66000000"
            }
            Image {
                id: sourcepic
                anchors.fill: musicpic
                fillMode: Image.PreserveAspectCrop
                visible: false
                source: mainMedia.urlStr || "qrc:/QueMusic/resources/app/musicpic.png"
                sourceSize: Qt.size(512, 512)
                cache: false
            }
            Rectangle {
                id: maskpic
                anchors.fill: musicpic
                color: "#ff000000"
                radius: musicpic.radius
                layer.enabled: true
                visible: false
            }
            MouseArea {
                anchors.fill: musicpic
                onClicked: picWatch.dialog(mainMedia.urlStr || "qrc:/QueMusic/resources/app/musicpic.png",window.musicTitle);
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

        // 全窗口沉浸歌词页
        Loader {
            id: controlMaxLoader
            x: 0
            z: 3
            visible: false
            active: false
            property int lyricsX: mainLayout.width * 0.46
            property int lyricsType: 0// 0. normal 1. Cover 2. Lyrics
            property int infoX: mainLayout.width * 0.23 - (mainLayout.piclong / 2)
            property bool isHideGui: false
            property int hideHeight: 0
            property bool basicCd: false
            Behavior on hideHeight { enabled: controlMaxLoader.visible; NumberAnimation { duration: 480; easing.type: Easing.OutExpo } }
            onLoaded: {
                window.playermaxed()
                minedAnimation.stop()
                visible = true
                maxedAnimation.start()
                switch(mainLayout.maxLyricType) {
                case 0:
                    mainLayout.state = "MaxedNormal"
                    break;
                case 1:
                    mainLayout.state = "MaxedCover"
                    break;
                case 2:
                    mainLayout.state = "MaxedLyric"
                    break;
                }
            }
            width: mainLayout.width
            height: mainLayout.height
            source: "qrc:/QueMusic/layout/PlayerMaxCenter.qml"
        }
        //提取颜色部分
        Item {
            id: coverColor
            width: 800
            height: 600
            property color color1: "#00ee66"
            property color color2: "#00b1ee"
            property color color3: "#9d4edd"
            property bool thirdColors: true

            // 颜色提取器
            ColorExtractor {
                id: colorExtractor
                signal colorExtractFinished()
                onColorsExtracted: {
                    //rectcolorAnime.running = false;
                    console.log("提取到颜色:", colors);
                    if (colors.length >= 3) {
                        // 更新渐变颜色
                        console.log("三种颜色");
                        coverColor.color1 = colors[0];
                        coverColor.color2 = colors[1];
                        coverColor.color3 = colors[2];
                        coverColor.thirdColors = true;
                    } else if(colors.length == 2) {
                        console.log("2种颜色");
                        coverColor.color1 = colors[0];
                        coverColor.color2 = colors[1];
                        coverColor.color3 = colors[0];
                        coverColor.thirdColors = false;
                    } else {
                        console.log("默认颜色");
                        coverColor.color1 = "#00b1ee";
                        coverColor.color2 = "#9d4edd";
                        coverColor.color3 = "#00ea64";
                        coverColor.thirdColors = true;
                    }
                    //rectcolorAnime.running = true;
                    colorExtractFinished();
                }

                onColorsExtractedAsString: {
                    console.log("颜色字符串:", colors);
                }
            }
        }
    }

    Loader {
        id: settingsView
        anchors.fill: parent
        active: false
        visible: false
        z: 6
        source: "qrc:/QueMusic/SettingsView.qml"//"qrc:/QueMusic/SettingsView.qml"
        opacity: visible ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 120 } }
        onLoaded: {
            visible = true;
            settingAnime.running = true;
        }
    }

    Item {
        id: fpsCounter
        visible: Options.settings.displayFps
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 68
        anchors.rightMargin: 16
        z: 99
        property int frames: 0
        property real fps: 0
        width: 78
        height: 24
        Rectangle {
            anchors.fill: parent
            radius: 12
            color: Style.themes.shadowColor
            opacity: 0.75
        }
        Text {
            anchors.centerIn: parent
            text: fpsCounter.fps.toFixed(0) + " FPS"
            color: Style.themes.fontColor
            font.pixelSize: 11
            font.bold: true
        }
        Timer {
            interval: 500
            repeat: true
            running: fpsCounter.visible
            onTriggered: {
                fpsCounter.fps = fpsCounter.frames * 2;
                fpsCounter.frames = 0;
            }
        }
        Connections {
            target: window
            function onAfterRendering() { fpsCounter.frames++ }
        }
    }

    SequentialAnimation {
        id: settingAnime
        NumberAnimation {
            target: settingsView
            property: "opacity"
            duration: 240
            from: 0
            to: 1
            easing.type: Easing.OutCubic
        }
        ScriptAction {
            script: {
                mainLayout.state = "";
                mainLayout.visible = false;
                window.playermined();
                //barLeftWidgets.visible = true;
                minedAnimation.start();
            }
        }
    }
    SequentialAnimation {
        id: settingOutAnime
        ScriptAction {
            script: mainLayout.visible = true;
        }
        NumberAnimation {
            target: settingsView
            property: "opacity"
            duration: 240
            from: 1
            to: 0
            easing.type: Easing.OutCubic
        }
        ScriptAction {
            script: {
                settingsView.visible = false;
                settingsView.active = false;
            }
        }
    }

    CoverHelper {
        id: coverHelper
    }

    Connections {
        target: MusicApi
        function onUrlplay(playurl,title,artist,cover,solve,hash,source) {
            mainMedia.urlLocal = false;
            mainMedia.source = playurl;
            mainMedia.noTitle = title;
            window.musicTitle = title;
            window.musicArtist = artist;
            console.log("url:", playurl);
            mainMedia.urlStr = cover;
            mainMedia.play();
            colorExtractor.extractColorsFromUrl(solve);
            console.log("---正在提取封面颜色");
            var listIndex = -1;
            for(var i = 0;i < playListModel.count;i++) {
                var forUrl = playListModel.get(i).path;
                if(forUrl === hash) {
                    listIndex = i;
                }
            }
            //var listIndex = listfile.findIndexByValue(playListModel, "path", playurl);
            if (listIndex == -1) {
                playListModel.append({ name: title, path: hash, songer: artist, source: source });
                playListModel.playListIndex = playListModel.count - 1;
            } else {
                playListModel.playListIndex = listIndex;
            }

        }
        // C++ 下载/提示信号
        function onWarned(text,type) {
            mainWarn.tiped(text,type);
        }
    }


    AudioOutput { id: volumeValue; volume: Options.settings.musicVolume; device: Options.settings.useDefaultDevice ? musicDevices.defaultAudioOutput : musicDevices.audioOutputs[Options.settings.audioDevice] }
    MediaDevices { id: musicDevices }
    // 主媒体
    GetWave {
        id: getWave
        mediaPlayer: mainMedia
        enabled: Style.settings.waveDisplay && mainMedia.playing
        bands: 128
        //audioBufferOutput: mainMedia.audioBufferOutput
    }
    MediaPlayer {
        property string noTitle
        property string urlStr: "qrc:/QueMusic/resources/app/musicpic.png"
        property string album
        property string date
        property string type
        property bool urlLocal
        property bool onMedia: mediaStatus !== MediaPlayer.NoMedia
        id: mainMedia
        audioOutput: volumeValue
        //audioBufferOutput: getWave.audioBufferOutput

        source: ""
        autoPlay: Options.settings.autoPlay
        onMetaDataChanged: {
            console.log("QML: MediaPlayer created, audioBufferOutput =",audioBufferOutput)
            if(urlLocal) {
                // 尝试不同的键名
                var title = mainMedia.metaData.stringValue(MediaMetaData.Title)
                var artist = mainMedia.metaData.stringValue(MediaMetaData.AlbumArtist) || mainMedia.metaData.value(MediaMetaData.Author)
                var album = mainMedia.metaData.stringValue(MediaMetaData.AlbumTitle)
                var cover = mainMedia.metaData.value(MediaMetaData.CoverArtImage) || mainMedia.metaData.value(MediaMetaData.ThumbnailImage)
                var date = mainMedia.metaData.value(MediaMetaData.Date)
                var type = mainMedia.metaData.value(MediaMetaData.MediaType)
                var keys = mainMedia.metaData.keys()

                var lyrics = mainMedia.metaData.value(MediaMetaData.AudioCodec)
                console.debug("metadatalyric:", lyrics)

                if (keys) {
                    console.log("获取keys:" + keys)
                }

                if (title) {
                    console.log("标题 (Title): " + title);
                    window.musicTitle = title
                } else {
                    console.log("使用文件名标题");
                    window.musicTitle = noTitle
                }

                if (artist) {
                    console.log("艺术家 (Artist): " + artist);
                    window.musicArtist = artist
                }

                if (album) {
                    console.log("专辑 (Album): " + album)
                    mainMedia.album = album
                }
                
                if (date) {
                    mainMedia.date = date.toString()
                }
                
                if (date) {
                    mainMedia.type = type.toString()
                }

                if (cover) {
                    console.log("找到封面艺术: " + cover);
                    urlStr = coverHelper.convertVariantToUrl(cover);
                    console.log("封面艺术url: " + urlStr);
                    colorExtractor.extractColorsFromUrl(urlStr);
                } else {
                    // 元数据未内嵌封面：本地文件尝试同目录同名/常见命名封面图兜底
                    // （如 a.mp3 旁的 a.jpg / cover.png）。未命中则保持空，与原行为一致。
                    var localCover = coverHelper.findLocalCover(mainMedia.source)
                    urlStr = localCover ? localCover : null
                }


                // 打印所有可用的元数据键
                console.log("所有可用元数据键:", Object.keys(mainMedia.metaData));
            }
        }

        onMediaStatusChanged: {
            if(mainMedia.mediaStatus === MediaPlayer.EndOfMedia) {
                switch(musicControlMin.cycleIndex) {
                    case 0:
                        musicControlMin.enterMedia()
                        break;
                    case 1:
                        mainMedia.position = 0
                        mainMedia.play()
                        break;
                    case 2:
                        musicControlMin.randomMedia()
                        break;
                    case 3:
                        mainMedia.stop()
                        break;
                }
            }
        }

        onUrlStrChanged: {
            if (windowsSmtc.available)
                smtcUpdateMediaInfo()
        }
    }
    // 依据播放列表上下文动态启用/禁用 SMTC 的上一首/下一首按钮
    // （无上一首/下一首时禁用对应按钮，避免一律恒启用）
    function updateSmtcControls() {
        if (!windowsSmtc.available)
            return;
        var count = playListModel.count;
        var idx = playListModel.playListIndex;
        windowsSmtc.setControlsEnabled(true, true, idx < count - 1, idx > 0);
    }

    // 统一将当前曲目信息推给 SMTC。
    // AppMediaId 取播放列表当前项的稳定标识 path（在线歌曲为歌曲 hash、本地文件为文件路径），供系统按曲目分组元信息；
    // 列表未就绪/无当前项时传空串，C++ 侧会清除旧的 id。
    function smtcUpdateMediaInfo() {
        if (!windowsSmtc.available)
            return
        var mediaId = ""
        if (playListModel.count > 0 && playListModel.playListIndex >= 0) {
            var item = playListModel.get(playListModel.playListIndex)
            if (item)
                mediaId = item.path
        }
        windowsSmtc.updateMediaInfo(window.musicTitle, window.musicArtist,
                                    mainMedia.album, mainMedia.urlStr, mediaId)
    }

    // Windows SMTC
    WindowsSmtcManager {
        id: windowsSmtc

        Component.onCompleted: {
            windowsSmtc.initialize(window);
            updateSmtcControls()
        }
    }

    Connections {
        target: windowsSmtc

        function onPlayPressed() { mainMedia.play() }
        function onPausePressed() { mainMedia.pause() }
        function onNextPressed() { musicControlMin.enterMedia() }
        function onPreviousPressed() { musicControlMin.lastMedia() }
        function onSeekRequested(pos) { mainMedia.position = pos }
    }

    Connections {
        target: mainMedia

        function onSourceChanged() {
            // 切歌/新曲目开始播放的瞬间：主动推送 position=0 并刷新时间线；
            // duration 尚未就绪（<=0）时由 C++ 侧走全零重置分支清空上一首的残留进度。
            if (windowsSmtc.available)
                windowsSmtc.updateTimeline(0, mainMedia.duration)
            updateSmtcControls()
        }
        function onDurationChanged() {
            // 新歌时长加载完成：以 position=0 主动推送一条完整时间线，
            // 随后 onPositionChanged 会用实时位置持续刷新。
            if (windowsSmtc.available && mainMedia.duration > 0)
                windowsSmtc.updateTimeline(0, mainMedia.duration)
        }
        function onPlaybackStateChanged() {
            updateSmtcControls()
            if (!windowsSmtc.available)
                return
            switch (mainMedia.playbackState) {
            case MediaPlayer.PlayingState:
                windowsSmtc.setPlaybackStatus(WindowsSmtcManager.Playing)
                break
            case MediaPlayer.PausedState:
                windowsSmtc.setPlaybackStatus(WindowsSmtcManager.Paused)
                break
            case MediaPlayer.StoppedState:
                windowsSmtc.setPlaybackStatus(WindowsSmtcManager.Stopped)
                break
            case MediaPlayer.NoMediaState:
                windowsSmtc.setPlaybackStatus(WindowsSmtcManager.Closed)
                break
            default:
                windowsSmtc.setPlaybackStatus(WindowsSmtcManager.Closed)
                break
            }
        }
        function onPositionChanged() {
            if (windowsSmtc.available)
                windowsSmtc.updateTimeline(mainMedia.position, mainMedia.duration)
        }
    }

    Connections {
        target: window

        function onMusicTitleChanged() {
            if (windowsSmtc.available)
                smtcUpdateMediaInfo()
        }
        function onMusicArtistChanged() {
            if (windowsSmtc.available)
                smtcUpdateMediaInfo()
        }
    }

    // 播放列表
    ListModel {
        id: playListModel
        property int playListIndex: -1
        // 列表增删后同步 SMTC 上一首/下一首按钮可用性
        onCountChanged: updateSmtcControls()
    }
    SearchCard {
        id: searchCard
        onSearchIndex: (index) => {
            MusicApi.searchSongsResults.clear();
            mainContent.contentIndexed(6);
            var name = Options.settings.searchList[index];
            mainSearchInput.text = name;
            Options.settings.searchList = Options.settings.searchList.filter(value => value !== name);
            Options.settings.searchList.splice(0, 0, name);
            MusicApi.searchSongs(name,MusicApi.nowIndex,1,20);
            window.exitIndex = 1;
            searchCard.close();
        }
    }

    DesktopPlayer {
        id: desktopPlayer
    }

    // 桌面灵动岛（弃用？）
    Loader {
        id: desktopSpot
        anchors.fill: parent
        active: false
        asynchronous: true
        visible: status == Loader.Ready
        source: "qrc:/QueMusic/components/DesktopSpot.qml"
    }
    // 桌面小窗播放器
    Loader {
        id: desktopPlayerLoader
        active: false
        asynchronous: true
        visible: status == Loader.Ready
        source: "qrc:/QueMusic/components/DesktopPlayerWindow.qml"
    }
    // 桌面歌词
    Loader {
        id: desktopLyricsLoader
        active: false
        asynchronous: true
        visible: status == Loader.Ready
        source: "qrc:/QueMusic/components/DesktopLyrics.qml"
        property int lyricSize: 20
    }
    QAlertDialog {
        id: globalDialog
        title: "Dialog"
        message: "呃呃呃呃呃呃呃？(>-<)"
        isInput: false
        blurSource: mainLayout.visible ? mainLayout : settingsView
        //standardButtons: Dialog.Ok | Dialog.Cancel

        // 简单确认对话框的回调存储（由 openSimpleDialog 使用）
        property var dialogCallback: null

        // 通用简单确认对话框：点击"确定"后执行 callBack 回调
        function openSimpleDialog(title, text, callBack) {
            globalDialog.title = title;
            globalDialog.message = text;
            globalDialog.isInput = false;
            globalDialog.dialogCallback = callBack || null;
            globalDialog.open();
        }

        onConfirm: {
            // 若有回调则执行回调，否则保持原有默认行为（关闭窗口）
            if (globalDialog.dialogCallback) {
                var cb = globalDialog.dialogCallback;
                globalDialog.dialogCallback = null;
                cb();
            }
        }
    }
    QWarn {
        id: mainWarn
        Connections {
            target: Style
            function onWarned(text,type) {
                mainWarn.tiped(text,type);
            }
        }
    }
    QMessage {
        id: mainMessage
        function openSimpleDialog(title, text, callBack) {
            //mainMessage.dialogCallback = callBack || null;
            mainMessage.dialog(title,text,"\uf11a");
        }
    }
    QOptionDialog {
        id: picWatch
        property string source: "qrc:/QueMusic/resources/app/musicpic.png"
        property string fileName: "Picture.png"
        title: "查看图片"
        dialogContentHeight: 320
        cancelText: "保存"
        cancelIcon: "\uf00f"
        onCancel: {
            var sysPicPath = StandardPaths.writableLocation(StandardPaths.PicturesLocation)
            if(imageWatch.status === Image.Ready) {
                imageWatch.grabToImage(function(result) {
                    result.saveToFile(sysPicPath + "/" + picWatch.fileName);
                    console.log("图片已保存！");
                    mainWarn.tiped("已保存至系统图片文件夹",1);
                },Qt.size(512,512))
            } else {
                mainWarn.tiped("图片正在快速加载",0);
            }
        }
        function dialog(_source,_title) {
            source = _source;
            fileName = _title + ".png";
            picWatch.open();
        }

        options: Item {
            anchors.fill: parent
            Image {
                id: imageWatch
                source: picWatch.source
                x: parent.width / 2 - 128
                width: 256
                height: 256
                cache: false
                sourceSize.width: 512
                sourceSize.height: 512
                fillMode: Image.PreserveAspectCrop
            }
            Item {
                id: dragImage
                Drag.active: dragImageArea.drag.active
                Drag.dragType: Drag.Automatic
                Drag.supportedActions: Qt.CopyAction
                Drag.imageSource: imageWatch.source
                Drag.imageSourceSize: Qt.size(64, 64)
                Drag.mimeData: {
                    "text/uri-list": imageWatch.source

                    //picWatch.source
                }
            }

            MouseArea {
                id: dragImageArea
                anchors.fill: imageWatch
                drag.target: dragImage
            }

            Text {
                width: parent.width
                height: 40
                y: 260
                text: picWatch.fileName
                font.pixelSize: Style.settings.textH2
                color: Style.themes.textColor
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
    Loader {
        id: textWatch
        anchors.fill: parent
        active: false
        asynchronous: true
        visible: status == Loader.Ready
        source: "qrc:/QueMusic/components/QTextWindow.qml"
    }
}
