// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QueMusic 1.0
import QtCore
import QtMultimedia
import QWindowKit 1.0
import QtQuick.Controls.Basic

Window {
    id: window
    width: 1140
    height: 720
    minimumWidth: 810
    minimumHeight: 540
    color: Style.primaryColor
    title: "QueMusic"
    Component.onCompleted: {
        Options.queue = playListModel
        Options.warn = mainWarn
        Options.dialog = globalDialog
        Options.agent = windowAgent
        Options.desktop = desktopPlayer
        Options.smtc = smtc
        windowAgent.setup(window);
        // dark-mode / extra-margins / title-bar-height 是 Windows 专有属性
        if(!window.isMacOS) {
            // dwm-blur acrylic-material mica mica-alt
            windowAgent.setWindowAttribute("dark-mode", false);
            if(Options.settings.noWindowKit) {
                windowAgent.setWindowAttribute("extra-margins", 3);
                windowAgent.setWindowAttribute("title-bar-height", 40);
            }
        }
        MusicApi.songSource = Options.settings.mainMusicSource;
        MusicApi.downloadPath = Options.settings.downloadFolder;
        MusicApi.soundQuality = Options.settings.soundQuality;

        if(Options.settings.rememberWindow && Options.settings.winW > 0) {
            window.x = Options.settings.winX;
            window.y = Options.settings.winY;
            window.width = Options.settings.winW;
            window.height = Options.settings.winH;
        }

        window.visible = true;

        Playback.player = mainMedia;
        Playback.queue = playListModel;

        Style.changeUi();
        Style.changeTheme();

        // 会话恢复 / 历史加载 / 封面缓存维护都是磁盘与网络 I/O，全部移出首帧
        Qt.callLater(function() {
            Playback.loadHistory();
            Playback.restoreSession();

            if(Options.settings.cacheUrl)
                coverHelper.setCacheDir(Options.settings.cacheUrl);
            coverHelper.pruneCache(Options.settings.cacheSize);

            if(Options.settings.autoUpdate)
                autoUpdateTimer.start();
        });
    }

    Binding {
        target: MusicApi
        property: "soundQuality"
        value: Options.settings.soundQuality
    }
    Binding {
        target: MusicApi
        property: "downloadPath"
        value: Options.settings.downloadFolder
    }


    function silentUpdateCheck(): void {
        const xhr = new XMLHttpRequest();
        xhr.onreadystatechange = function() {
            if (xhr.readyState === XMLHttpRequest.DONE && xhr.status === 200) {
                const remote = parseInt(xhr.responseText.trim().substring(0, 3));
                if (remote > Options.versionCode)
                    mainWarn.tiped("发现新版本 v" + remote + "，请在设置-关于中查看", 1);
            }
        }
        xhr.open("GET", "https://raw.githubusercontent.com/BroNekoX/QueMusic/main/doc/updater.txt");
        xhr.send();
    }

    Timer {
        id: autoUpdateTimer
        interval: 3000
        onTriggered: window.silentUpdateCheck()
    }



    readonly property bool isMacOS: Qt.platform.os === "osx"

    // 标题栏是窗口拖拽区：插件挂进去的对象要标记可命中，否则点击会被拖拽吞掉
    function syncTitleBarHitTest(): void {
        const kids = barRightWidgets.children;
        for (let i = 0; i < kids.length; ++i)
            windowAgent.setHitTestVisible(kids[i], true);
    }

    function doSearch(text: string): void {
        MusicApi.searchSongsResults.clear();
        mainContent.contentIndexed(6);
        Options.settings.searchList = Options.settings.searchList.filter(value => value !== text);
        Options.settings.searchList.splice(0, 0, text);
        MusicApi.searchSongs(text, MusicApi.nowIndex, 1, 20);
        Options.exitIndex = 1;
    }

    // 首次加载内容临时存储，防止重新加载浪费内存
    property QtObject completedStart: QtObject {
        property bool homeLoaded: false
        property bool playlistLoaded: false
    }

    // 系统级关闭（Alt+F4、任务栏右键"关闭窗口"）与关闭按钮行为保持一致
    onClosing: function(close) {
        if(win.closeToTray()) {
            close.accepted = false;
            win.hideToTray();
            return;
        }
        // 未开启托盘时也要走保存与会话收尾，不能直接放行
        close.accepted = false;
        win.toClosing();
    }

    WindowControls {
        id: win
        targetWindow: window
        tray: systemTray
        onSessionClosing: {
            desktopLyricsLoader.active = false
            desktopPlayerLoader.active = false
        }
    }
    signal getKeys(var keys)
    signal exit()

    signal message(string title,string text,int type)

    Shortcut {
        sequence: "Esc"
        context: Qt.ApplicationShortcut
        enabled: !Options.recordingShortCut
        onActivated: {
            window.exit();
            console.log("Exit");
            if(Options.exitIndex > 0) {
                Options.exitIndex -= 1;
            }
            mainLayout.forceActiveFocus();
        }
    }
    // 键位与开关都走设置页（Options.shortCuts / globalShortcut*）
    Instantiator {
        model: [
            { k: "volumeUp",      a: function() { Playback.stepVolume(Playback.volumeStep); mainWarn.tiped("音量 " + Math.round(Options.settings.musicVolume * 100) + "%", 0) } },
            { k: "volumeDown",    a: function() { Playback.stepVolume(-Playback.volumeStep); mainWarn.tiped("音量 " + Math.round(Options.settings.musicVolume * 100) + "%", 0) } },
            { k: "seekBack",      a: function() { Playback.seekBack() } },
            { k: "seekForward",   a: function() { Playback.seekForward() } },
            { k: "mute",          a: function() { Playback.toggleMute(); mainWarn.tiped(Playback.muted ? "已静音" : "取消静音", 0) } },
            { k: "abLoop",        a: function() {
                Playback.setAbPoint(Playback.abA < 0 || Playback.abArmed ? 0 : 1);
                mainWarn.tiped(Playback.abArmed ? "A-B 循环已启用" : "已设置 A-B 起点", 1);
            } },
            { k: "favorite",      a: function() { musicControlMin.toggleFavorite() } },
            { k: "playerOptions", a: function() { musicControlMin.openPlayerOptions() } },
            { k: "play",          a: function() { Playback.togglePlay() } },
            { k: "back",          a: function() { Playback.previous() } },
            { k: "forward",       a: function() { Playback.next(false) } },
            { k: "playList",      a: function() { playList.visible ? playList.close() : playList.open() } },
            { k: "musicControl",  a: function() {
                if (mainLayout.state === "") {
                    controlMaxLoader.active = true;
                } else {
                    window.playermined();
                    minedAnimation.start();
                    mainLayout.state = "";
                }
            } }
        ]
        delegate: Shortcut {
            readonly property string flag: "globalShortcut" + modelData.k.charAt(0).toUpperCase() + modelData.k.slice(1)
            sequence: Options.shortCuts[modelData.k]
            context: Qt.ApplicationShortcut
            enabled: Options.settings.openShortCut && Options.shortCuts[flag]
            onActivated: modelData.a()
        }
    }

    FontLoader {
        id: iconFont
        source: "qrc:/QueMusic/resources/fonts/feather.ttf"
    }
    FontLoader {
        id: textFont
        source: "qrc:/QueMusic/resources/fonts/poppins.ttf"
    }

    WindowAgent {
        id: windowAgent
    }

    // 顶部栏 - 与qwindowkit和window耦合，难抽为组件
    Rectangle {
        id: titleBar
        x: 0
        y: 0
        z: 10
        width: window.width
        height: 60
        color: "transparent"
        property bool toBarLyric: musicControlMax.y === 0

        // 此组件创建时，将此组件与 qwindowkit 绑定，标题栏事件由此传入
        Component.onCompleted: windowAgent.setTitleBar(titleBar);

        Item {
            id: appTitleBlock
            x: 10
            y: 10
            width: 180
            height: 40
            Component.onCompleted: windowAgent.setHitTestVisible(appTitleBlock, true)
        }

        Row {
            id: barLeftWidgets
            y: 12
            Behavior on y { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }
            anchors {
                left: parent.left
                leftMargin: sidebar.width + 16
            }
            spacing: 5

            QWKButton {
                id: returnButton
                source: Style.darkis ? "qrc:/QueMusic/resources/window-bar/returnd.svg" : "qrc:/QueMusic/resources/window-bar/return.svg"
                onClicked: {
                    window.exit()
                    console.log("Exit")
                    if(Options.exitIndex > 0) {
                        Options.exitIndex -= 1
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
                leftPadding: 16
                placeholderText: "搜索"
                color: Style.textColor
                font.pixelSize: Style.settings.textmain
                verticalAlignment: Text.AlignVCenter
                selectionColor: Style.containColor
                focus: false
                onReleased: searchCard.open();
                onAccepted: {
                    if(text.trim() == "") {
                        mainWarn.tiped("请输入搜索内容", 0);
                        return;
                    }
                    window.doSearch(text);
                    searchCard.close();
                }
                Component.onCompleted: windowAgent.setHitTestVisible(mainSearchInput, true);
                background: Rectangle {
                    height: 36
                    width: 201
                    radius: 18
                    color: Style.primaryColor
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
                        mainWarn.tiped("请输入搜索内容", 0);
                        return;
                    }
                    window.doSearch(mainSearchInput.text);
                    searchCard.close();
                }
                Component.onCompleted: windowAgent.setHitTestVisible(searchButton, true);
            }
        }

        Row {
            id: barRightWidgets
            anchors {
                right: parent.right
                rightMargin: 16
            }
            spacing: 0
            y: 10 - musicControlMax.hideHeight
            height: 40

            // 扩展点：标题栏（窗口按钮之前）
            PluginSlot { slotName: "titlebar"; target: barRightWidgets }

            // 全屏开关
            QWKButton {
                id: fullScreenButton
                largeicon: true
                property int prevVisibility: Window.Maximized
                buttonColor: window.visibility === Window.FullScreen ? Style.containColor : "transparent"
                source: Style.darkis || titleBar.toBarLyric ? "qrc:/QueMusic/resources/window-bar/airplayd.svg" : "qrc:/QueMusic/resources/window-bar/airplay.svg"
                onClicked: {
                    if (window.visibility === Window.FullScreen) {
                        if (prevVisibility === Window.Maximized)
                            window.showMaximized();
                        else
                            window.showNormal();
                    } else {
                        prevVisibility = window.visibility;
                        window.showFullScreen();
                    }
                }
                Component.onCompleted: windowAgent.setHitTestVisible(fullScreenButton, true);
            }

            QWKButton {
                id: settingButton
                largeicon: true
                source: Style.darkis || titleBar.toBarLyric ? "qrc:/QueMusic/resources/window-bar/settingd.svg" : "qrc:/QueMusic/resources/window-bar/setting.svg"
                onClicked: {
                    settingsView.active = true;
                }
                Component.onCompleted: windowAgent.setHitTestVisible(settingButton, true);
            }

            QWKButton {
                id: minButton
                source: Style.darkis || titleBar.toBarLyric ? "qrc:/QueMusic/resources/window-bar/minimized.svg" : "qrc:/QueMusic/resources/window-bar/minimize.svg"
                onClicked: window.showMinimized();
                Component.onCompleted: {
                    if(window.isMacOS) {
                        visible = false;
                    } else {
                        windowAgent.setSystemButton(WindowAgent.Minimize, minButton);
                    }
                }
            }

            QWKButton {
                readonly property string maximized: Style.darkis || titleBar.toBarLyric ? "qrc:/QueMusic/resources/window-bar/maximized.svg" : "qrc:/QueMusic/resources/window-bar/maximize.svg"
                readonly property string restored: Style.darkis || titleBar.toBarLyric ? "qrc:/QueMusic/resources/window-bar/restored.svg" : "qrc:/QueMusic/resources/window-bar/restore.svg"
                id: maxButton
                source: window.visibility === Window.Maximized ? restored : maximized
                onClicked: {
                    if (window.visibility === Window.Maximized) {
                        window.showNormal();
                    } else {
                        window.showMaximized();
                    }
                }
                Component.onCompleted: {
                    if(window.isMacOS) {
                        visible = false;
                    } else {
                        windowAgent.setSystemButton(WindowAgent.Maximize, maxButton);
                    }
                }
            }

            QWKButton {
                readonly property string hover: Style.darkis ? "qrc:/QueMusic/resources/window-bar/close.svg" : "qrc:/QueMusic/resources/window-bar/closed.svg"
                readonly property string unhover: Style.darkis || titleBar.toBarLyric ? "qrc:/QueMusic/resources/window-bar/closed.svg" : "qrc:/QueMusic/resources/window-bar/close.svg"
                id: closeButton
                source: closeButton.hovered ? hover : unhover
                hoverColor: "#ee4848"
                onClicked: win.toClosing();
                Component.onCompleted: {
                    if(window.isMacOS) {
                        visible = false;
                    } else {
                        windowAgent.setSystemButton(WindowAgent.Close, closeButton);
                    }
                }
            }
        }
    }

    Item {
        id: mainLayout
        anchors.fill: parent
        z: 5
        property int maxLyricType: 0

        Connections {
            target: Style
            function onChangeTheme(): void {
                windowAgent.setWindowAttribute("dwm-blur", false);
                if(Style.settings.backmode === 0) {
                    backGround.visible = false;
                    sidebar.baseColor = Style.primaryColor;
                    window.color = Style.primaryColor;
                    mainContent.color = Style.secondaryColor;
                } else if(Style.settings.backmode === 1) {
                    backGround.visible = false;
                    sidebar.baseColor = Style.primaryBlurColor;
                    window.color = Style.containColor;
                    mainContent.color = Style.blurOverlayColor;
                } else if(Style.settings.backmode === 2) {
                    backGround.visible = true;
                    sidebar.baseColor = Style.blurOverlayColor;
                    window.color = Style.primaryColor;
                    mainContent.color = Style.blurOverlayColor;
                    backGround.source = "qrc:/QueMusic/resources/pic/cloudRainbow.png";
                } else if(Style.settings.backmode === 3) {
                    backGround.visible = true;
                    sidebar.baseColor = Style.blurOverlayColor;
                    window.color = Style.primaryColor;
                    mainContent.color = Style.blurOverlayColor;
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
                    sidebar.baseColor = Style.primaryBlurColor;
                    window.color = "transparent";
                    mainContent.color = Style.primaryBlurColor;
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

        MainContent {
            z: 1
            id: mainContent
            x: sidebar.width
            y: 0
            width: parent.width - x
            height: parent.height - 78
        }

        PlayerControl {
            id: musicControlMin
            x: 0
            width: parent.width
            height: 78
            z: 4
        }

        PlayerMaxCenter {
            id: musicControlMax
            width: mainLayout.width
            height: mainLayout.height
            y: 100//mainLayout.height
            visible: false
            z: 3
        }

        NumberAnimation {
            id: openMaxLyric
            duration: 420
            target: musicControlMax
            property: "y"
            from: mainLayout.height
            to: 0
            easing.type: Easing.BezierSpline
            easing.bezierCurve: [ 0.30, 0.08, 0.00, 1.00, 1, 1 ]
            onStarted: {
                musicControlMax.visible = true;
                barLeftWidgets.y = -48;
            }
        }
        NumberAnimation {
            id: closeMaxLyric
            duration: 420
            target: musicControlMax
            property: "y"
            to: mainLayout.height
            easing.type: Easing.BezierSpline
            easing.bezierCurve: [ 0.50, 0.08, 0.00, 1.00, 1, 1 ]
            onStarted: barLeftWidgets.y = 12;
            onFinished: {
                musicControlMax.visible = false;
                musicControlMax.hideHeight = 0;
            }
        }

        Item {
            id: coverColor
            width: 800
            height: 600
            property color color1: "#00ee66"
            property color color2: "#00b1ee"
            property color color3: "#9d4edd"
            property bool thirdColors: true

            // 取色在工作线程完成，结果回填
            ColorExtractor {
                id: colorExtractor
                signal colorExtractFinished()
                onColorsExtracted: (colors) => {
                    coverColor.color1 = colors.length > 0 ? colors[0] : "#00b1ee";
                    coverColor.color2 = colors.length > 1 ? colors[1] : "#9d4edd";
                    coverColor.color3 = colors.length > 2 ? colors[2]
                                      : (colors.length === 2 ? colors[0] : "#00ea64");
                    coverColor.thirdColors = colors.length >= 3 || colors.length < 2;
                    colorExtractFinished();
                }

                onColorsExtractedAsString: (colors) => {
                    console.log("颜色字符串:", colors);
                }
            }
        }
    }

    Loader {
        id: settingsView
        anchors.fill: parent
        active: false
        asynchronous: true
        visible: false
        z: 6
        sourceComponent: SettingsView {}
        opacity: visible ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 120 } }
        onLoaded: {
            visible = true;
            settingAnime.running = true;
        }
    }

    Loader {
        active: Options.settings.displayFps
        visible: active
        anchors.top: mainLayout.top
        anchors.right: mainLayout.right
        anchors.topMargin: 68
        anchors.rightMargin: 16
        width: 78
        height: 24
        sourceComponent: Item {
            id: fpsCounter
            z: 99
            visible: true
            property int frames: 0
            property real fps: 0
            Rectangle {
                anchors.fill: parent
                radius: 12
                color: Style.shadowColor
                opacity: 0.75
            }
            Text {
                anchors.centerIn: parent
                text: fpsCounter.fps.toFixed(0) + " FPS"
                color: Style.fontColor
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
                // 关闭帧率显示时不挂每帧回调，避免白耗 JS 调用
                enabled: Options.settings.displayFps
                target: window
                function onAfterRendering(): void { fpsCounter.frames++ }
            }
        }
    }

    Loader {
        active: Options.settings.debug
        visible: active
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: 68
        anchors.leftMargin: 220
        width: 260
        height: 92
        sourceComponent: Item {
            id: debugHud
            z: 99
            property string info: ""
            Rectangle {
                anchors.fill: parent
                radius: 12
                color: Style.shadowColor
                opacity: 0.75
            }
            Text {
                anchors.fill: parent
                anchors.margins: 10
                text: debugHud.info
                color: Style.fontColor
                font.pixelSize: 11
                font.family: TextFont.name
            }
            Timer {
                interval: 500
                repeat: true
                running: debugHud.visible
                onTriggered: {
                    debugHud.info = "调试模式"
                            + "\n媒体状态: " + mainMedia.mediaStatus
                            + "\n进度: " + Math.floor(mainMedia.position / 1000) + "s / " + Math.floor(mainMedia.duration / 1000) + "s"
                            + "\n队列: " + playListModel.count + " 首"
                            + "\n音量: " + Math.round(Options.settings.musicVolume * 100) + "%"
                }
            }
        }
    }

    NumberAnimation {
        id: settingAnime
        target: settingsView
        property: "opacity"
        duration: 240
        from: 0
        to: 1
        easing.type: Easing.OutCubic
        onFinished: {
            closeMaxLyric.running = true;
            mainLayout.visible = false;
        }
    }
    NumberAnimation {
        id: settingOutAnime
        target: settingsView
        property: "opacity"
        duration: 240
        from: 1
        to: 0
        easing.type: Easing.OutCubic
        onStarted: mainLayout.visible = true;
        onFinished: {
            settingsView.visible = false;
            settingsView.active = false;
        }
    }

    CoverHelper {
        id: coverHelper
        onLocalCoverReady: (path, coverUrl) => {
            if (path !== Playback.localLyricsRequestPath)
                return;
            const localCover = coverUrl || coverHelper.findLocalCover(path);
            mainMedia.urlStr = localCover || "qrc:/QueMusic/resources/app/musicpic.png";
            if (localCover)
                colorExtractor.extractColorsFromUrl(localCover);
        }
    }

    // WebDAV 缓存落地：按本地文件那套回填（标签/同名歌词/同目录封面）
    Connections {
        target: WebDavCache
        function onCached(url: string, localPath: string): void {
            if (mainMedia.source.toString() !== url)
                return;
            Playback.localLyricsRequestPath = localPath;
            MusicApi.readLocalMetadataAsync(localPath);
        }
    }

    Connections {
        target: MusicApi
        function onUrlplay(playurl: string, title: string, artist: string, cover: string,
                           solve: string, hash: string, source: int): void {
            mainMedia.noTitle = title;
            Playback.musicTitle = title;
            Playback.musicArtist = artist;
            Playback.musicHash = hash;
            Playback.musicSource = source;
            mainMedia.urlStr = cover;
            colorExtractor.extractColorsFromUrl(solve);
            // 淡出静音排空设备缓冲后再换源：上一首仍在播放，直接换 source 会截断波形产生爆音
            Playback.swap(function() {
                mainMedia.source = playurl;
                mainMedia.play();
            });

            const listIndex = Playback.indexOfPath(hash);
            if (listIndex < 0) {
                playListModel.append({ name: title, path: hash, songer: artist, source: source });
                playListModel.playListIndex = playListModel.count - 1;
            } else {
                playListModel.playListIndex = listIndex;
            }
        }
        function onWarned(text: string, type: int): void {
            mainWarn.tiped(text,type);
        }
        function onLocalLyricsReady(filePath: string, lyrics: var, translate: var): void {
            if (filePath !== Playback.localLyricsRequestPath)
                return;
            MusicApi.lyricsData = lyrics;
            MusicApi.lyricsTranslate = translate || [];
        }
        function onLocalLyricsFailed(filePath: string): void {
            if (filePath !== Playback.localLyricsRequestPath)
                return;
            if ((MusicApi.lyricsData || []).length > 1) // 已有歌词（如内嵌 SYLT）就保留，别退回占位
                return;
            MusicApi.setLocalLyrics();
        }
        function onLocalMetadataReady(filePath: string, meta: var): void {
            if (filePath !== Playback.localLyricsRequestPath)
                return;
            if (meta.title)
                Playback.musicTitle = meta.title;
            if (meta.artist)
                Playback.musicArtist = meta.artist;
            if (meta.album)
                mainMedia.album = meta.album;
            const hasMetaLyrics = meta.lyrics && meta.lyrics.length > 0;
            MusicApi.lyricsData = meta.lyrics || [];
            MusicApi.lyricsTranslate = meta.translate || [];
            if (!hasMetaLyrics)
                MusicApi.setLocalLyrics();
            MusicApi.readLocalLyricsAsync(filePath, meta.title || mainMedia.noTitle,
                                          meta.artist || "", meta.duration || 0, !hasMetaLyrics);
            if (meta.cover) {
                mainMedia.urlStr = meta.cover;
                colorExtractor.extractColorsFromUrl(meta.cover);
            } else {
                coverHelper.findEmbeddedCoverAsync(filePath);
            }
        }
    }


    MediaDevices { id: musicDevices }
    GetWave {
        id: getWave
        engine: mainMedia
        renderWindow: window
        enabled: Style.settings.waveDisplay && musicControlMax.visible
        bands: 128
    }

    AudioEngine {
        id: mainMedia
        property string noTitle
        property string urlStr: "qrc:/QueMusic/resources/app/musicpic.png"
        property string album
        property string date
        property string type
        property bool onMedia: mediaStatus !== AudioEngine.NoMedia
        property int audioBit: bitRate

        source: ""
        volume: Playback.outVolume
        deviceId: Options.settings.useDefaultDevice || musicDevices.audioOutputs.length <= Options.settings.audioDevice
                  ? "" : String(musicDevices.audioOutputs[Options.settings.audioDevice].id)

        onMetaDataChanged: {
            if (mainMedia.source.toString().startsWith("http"))
                return
            if (tagTitle)
                Playback.musicTitle = tagTitle
            if (artist)
                Playback.musicArtist = artist
            if (albumTitle)
                mainMedia.album = albumTitle
            mainMedia.date = mediaDate
            mainMedia.type = mediaType
        }

        onMediaStatusChanged: {
            if (mediaStatus === AudioEngine.LoadedMedia)
                Playback.autoSkipCount = 0;
            if(mediaStatus === AudioEngine.EndOfMedia) {
                if(Playback.sleepMode === 2) {
                    Playback.sleepEnd();
                    return;
                }
                switch(Options.settings.cycleIndex) {
                    case 0:
                        Playback.next(false);
                        break;
                    case 1:
                        // 重播同样会刷新缓冲，走淡出/淡入
                        Playback.swap(function() {
                            mainMedia.position = 0;
                            mainMedia.play();
                        })
                        break;
                    case 2:
                        Playback.next(true);
                        break;
                    case 3:
                        // 停止不再淡回，直接复位音量乘数
                        Playback.swap(function() { mainMedia.stop() }, false);
                        break;
                }
            }
        }

        onErrorOccurred: {
            if(playListModel.count < 2) return;
            if (Playback.autoSkipCount >= 2) {
                Playback.autoSkipCount = 0;
                Style.warned("连续多首无法播放，已停止自动跳过", 0);
                return;
            }
            Playback.autoSkipCount += 1;
            Style.warned("当前歌曲无法播放，已自动跳过",0);
            if(Options.settings.cycleIndex === 2) {
                Playback.next(true);
            } else {
                Playback.next(false);
            }
        }

        // 新音轨真正起播后再淡回，避免音频设备重开的瞬间已经有音量
        onPlayingChanged: {
            if (playing)
                Playback.finishSwap();
        }

        onSourceChanged: {
            Playback.clearAb();
        }
        onPlaybackStateChanged: {
            if (mainMedia.playbackState === AudioEngine.PlayingState)
                Playback.noteNowPlaying();
        }
    }
    SystemTrayManager {
        id: systemTray
        iconSource: "qrc:/QueMusic/resources/icon.ico"
        title: "QueMusic"
        playing: mainMedia.playbackState === AudioEngine.PlayingState
        nowPlaying: Playback.musicTitle !== "QueMusic"
                    ? Playback.musicTitle + (Playback.musicArtist && Playback.musicArtist !== "Artist"
                                           ? " - " + Playback.musicArtist : "")
                    : ""

        onShowWindowRequested: win.restoreWindow()
        onPlayPauseRequested: Playback.togglePlay()
        onPreviousRequested: Playback.previous()
        onNextRequested: Playback.next(false)
        onQuitRequested: win.quitApp()
        onActivated: (reason) => {
            // 左键单击 / 双击托盘图标：恢复主界面
            if(reason === SystemTrayManager.Trigger || reason === SystemTrayManager.DoubleClick)
                win.restoreWindow();
        }
    }

    SmtcBridge {
        id: smtc
        player: mainMedia
        queue: playListModel
        targetWindow: window
    }

    // 播放列表（C++ QueueModel：O(1) 路径查找、批量操作、角色化访问）
    QueueModel {
        id: playListModel
        playListIndex: -1
    }


    SearchCard {
        id: searchCard
        onSearchIndex: (index) => {
            const name = Options.settings.searchList[index];
            mainSearchInput.text = name;
            window.doSearch(name);
            searchCard.close();
        }
    }

    DesktopPlayer {
        id: desktopPlayer
    }

    Loader {
        id: desktopPlayerLoader
        active: false
        asynchronous: true
        visible: status == Loader.Ready
        source: "qrc:/QueMusic/components/DesktopPlayerWindow.qml"
    }
    Loader {
        id: desktopLyricsLoader
        active: false
        asynchronous: true
        visible: status == Loader.Ready
        source: "qrc:/QueMusic/components/DesktopLyrics.qml"
    }
    QAlertDialog {
        id: globalDialog
        title: "Dialog"
        message: "呃呃呃呃呃呃喵？(>-<)"
        isInput: false
        blurSource: mainLayout.visible ? mainLayout : settingsView

        property var dialogCallback: null

        function openSimpleDialog(title: string, text: string, callBack: var): void {
            globalDialog.title = title;
            globalDialog.message = text;
            globalDialog.isInput = false;
            globalDialog.dialogCallback = callBack || null;
            globalDialog.open();
        }

        onConfirm: {
            if (globalDialog.dialogCallback) {
                const cb = globalDialog.dialogCallback;
                globalDialog.dialogCallback = null;
                cb();
            }
        }
    }
    QWarn {
        id: mainWarn
        Connections {
            target: Style
            function onWarned(text: string, type: int): void {
                mainWarn.tiped(text,type);
            }
        }
    }
    QMessage {
        id: mainMessage
        function openSimpleDialog(title: string, text: string, callBack: var): void {
            mainMessage.dialog(title,text,"\uf11a");
        }
    }
    QOptionDialog {
        id: picWatch
        property string source: "qrc:/QueMusic/resources/app/musicpic.png"
        property string fileName: "Picture.png"
        title: "查看图片"
        cancelText: "保存"
        cancelIcon: "\uf00f"
        onCancel: {
            const sysPicPath = StandardPaths.writableLocation(StandardPaths.PicturesLocation)
            if(imageWatch.status === Image.Ready) {
                imageWatch.grabToImage(function(result) {
                    result.saveToFile(sysPicPath + "/" + picWatch.fileName);
                    console.log("图片已保存！");
                    mainWarn.tiped("已保存至系统图片文件夹",1);
                },Qt.size(512,512))
            } else {
                mainWarn.tiped("图片尚未加载完成，请稍候", 0);
            }
        }
        function dialog(_source: string, _title: string): void {
            source = _source;
            fileName = _title + ".png";
            picWatch.open();
        }

        options: Column {
                    width: parent.width
                    Image {
                        anchors.horizontalCenter: parent.horizontalCenter
                        id: imageWatch
                        source: picWatch.source
                        width: 256
                        height: 256
                        cache: false
                        sourceSize.width: 512
                        sourceSize.height: 512
                        fillMode: Image.PreserveAspectCrop
                        MouseArea {
                            id: dragImageArea
                            anchors.fill: imageWatch
                            drag.target: dragImage
                        }
                    }
                    Item {
                        id: dragImage
                        Drag.active: dragImageArea.drag.active
                        Drag.dragType: Drag.Automatic
                        Drag.supportedActions: Qt.CopyAction
                        Drag.imageSource: imageWatch.source
                        Drag.imageSourceSize: Qt.size(64, 64)
                        Drag.mimeData: { "text/uri-list": imageWatch.source }
                    }
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: parent.width
                        height: 40
                        text: picWatch.fileName
                        font.pixelSize: Style.settings.textH2
                        color: Style.textColor
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

    // 扩展点：整窗覆盖层（插件自己定位，默认不吃鼠标事件）
    Item {
        id: pluginOverlay
        anchors.fill: parent
        z: 20
        PluginSlot { slotName: "window.overlay"; target: pluginOverlay }
    }

    PluginHost {
        onPluginLoaded: window.syncTitleBarHitTest()
    }
}