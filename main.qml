// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QueMusic 1.0
import QtCore
import QtMultimedia
import QWindowKit 1.0
import QtQuick.Effects
import QtQuick.Controls.Basic

Window {
    id: window
    width: 1140
    height: 720
    minimumWidth: 810
    minimumHeight: 540
    color: Style.themes.primaryColor
    title: "QueMusic"
    Component.onCompleted: {
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

        // 会话恢复与历史加载移出首帧：启动只做装配，数据就绪后回填
        Qt.callLater(function() {
            Playback.loadHistory();
            window.restoreSession();
        });

        if(Options.settings.cacheUrl)
            coverHelper.setCacheDir(Options.settings.cacheUrl);
        coverHelper.pruneCache(Options.settings.cacheSize);

        if(Options.settings.autoUpdate)
            autoUpdateTimer.start();
    }

    // 音质设置同步给 C++
    Binding {
        target: MusicApi
        property: "soundQuality"
        value: Options.settings.soundQuality
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

    Connections {
        target: Options.settings
        function onDownloadFolderChanged(): void {
            MusicApi.downloadPath = Options.settings.downloadFolder;
        }
    }
    property string musicTitle: "QueMusic"
    property string musicArtist: "Artist"
    property int exitIndex: 0

    // 托盘"退出"时置位：跳过"关闭到托盘"直接退出
    property bool forceQuit: false
    // 隐藏到托盘前的窗口状态，恢复时还原最大化/全屏
    property int trayRestoreVisibility: Window.Windowed
    // 本次运行是否已提示过"已最小化到系统托盘"
    property bool trayTipShown: false

    property string localLyricsRequestPath: ""
    property int pendingSeek: 0
    property string pendingSeekPath: ""
    property int smtcLastTimeline: 0
    property int smtcLastPosition: 0
    readonly property bool isMacOS: Qt.platform.os === "osx"

    // 统一搜索入口：清空结果、写入搜索历史并触发搜索
    function doSearch(text: string): void {
        MusicApi.searchSongsResults.clear();
        mainContent.contentIndexed(6);
        Options.settings.searchList = Options.settings.searchList.filter(value => value !== text);
        Options.settings.searchList.splice(0, 0, text);
        MusicApi.searchSongs(text, MusicApi.nowIndex, 1, 20);
        window.exitIndex = 1;
    }

    // 播放本地歌曲：立即起播不阻塞，元数据/封面/歌词在工作线程就绪后回填
    function playLocalSong(path: string, name: string): void {
        window.localLyricsRequestPath = path;
        mainMedia.urlLocal = true;
        mainMedia.noTitle = name;
        window.musicTitle = name;
        window.musicArtist = "";
        MusicApi.lyricsData = [];
        MusicApi.lyricsTranslate = [];
        MusicApi.readLocalMetadataAsync(path);

        Playback.swap(function() {
            mainMedia.source = path;
            mainMedia.play();
        });
    }

    // 首次加载内容临时存储，防止重新加载浪费内存
    property QtObject completedStart: QtObject {
        property bool homeLoaded: false
        property bool playlistLoaded: false
    }

    // 判断是否到托盘设置函数
    function closeToTray(): bool {
        return Options.settings.closeToManage && !window.forceQuit && systemTray.available;
    }

    // 关闭前保存最后播放的歌曲
    function toClosing(): void {
        if(window.closeToTray()) {
            window.hideToTray();
            return;
        }
        if(playListModel.count > 0 && playListModel.playListIndex >= 0) {
            Options.lastSongs.name = window.musicTitle;
            Options.lastSongs.artist = window.musicArtist;
            Options.lastSongs.cover = mainMedia.urlStr || "qrc:/QueMusic/resources/app/musicpic.png";
            Options.lastSongs.hash = playListModel.get(playListModel.playListIndex).path;
            Options.lastSongs.source = playListModel.get(playListModel.playListIndex).source;
            Options.lastSongs.position = mainMedia.position;
            console.log("保存当前音乐记录。");
        }
        // 隐藏在托盘中的窗口（Hidden）几何信息依然有效，同样保存
        if(Options.settings.rememberWindow
                && window.visibility !== Window.Maximized
                && window.visibility !== Window.FullScreen) {
            Options.settings.winX = window.x;
            Options.settings.winY = window.y;
            Options.settings.winW = window.width;
            Options.settings.winH = window.height;
        }
        window.saveQueue();
        Playback.flush();
        // 清理桌面悬浮窗
        desktopLyricsLoader.active = false;
        desktopPlayerLoader.active = false;
        // 必须显式退出：窗口可能正隐藏在托盘里（close() 对不可见窗口不生效），
        // 且已关闭 quitOnLastWindowClosed，进程不会因窗口关闭而自动结束
        systemTray.quitApplication();
    }

    // 隐藏到系统托盘：任务栏图标随窗口一起消失，只剩托盘图标
    function hideToTray(): void {
        if(!window.visible)
            return;
        window.trayRestoreVisibility = window.visibility;
        window.hide();
        console.log("[托盘] 已隐藏到托盘，原窗口状态=" + window.trayRestoreVisibility);
        if(!window.trayTipShown) {
            window.trayTipShown = true;
            systemTray.showMessage("QueMusic", "已最小化到系统托盘，点击托盘图标可恢复",
                                   SystemTrayManager.Information);
        }
    }

    // 从托盘恢复窗口：还原隐藏前的最大化/全屏状态
    function restoreWindow(): void {
        if(!window.visible) {
            if(window.trayRestoreVisibility === Window.Maximized)
                window.showMaximized();
            else if(window.trayRestoreVisibility === Window.FullScreen)
                window.showFullScreen();
            else
                window.showNormal();
        } else if(window.visibility === Window.Minimized) {
            window.showNormal();
        }
        window.raise();
        window.requestActivate();
    }

    // 托盘菜单"退出"：走正常保存/关闭流程，但不再隐藏到托盘
    function quitApp(): void {
        window.forceQuit = true;
        window.toClosing();
    }

    // 系统级关闭（Alt+F4、任务栏右键"关闭窗口"）与关闭按钮行为保持一致
    onClosing: function(close) {
        if(window.closeToTray()) {
            close.accepted = false;
            window.hideToTray();
            return;
        }
        // 未开启托盘时也要走保存与会话收尾，不能直接放行
        close.accepted = false;
        window.toClosing();
    }

    signal getKeys(var keys)
    signal exit() // 返回

    signal message(string title,string text,int type)

    Shortcut {
        sequence: "Esc" // 返回
        context: Qt.ApplicationShortcut
        enabled: !Options.recordingShortCut
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
        enabled: Options.settings.openShortCut && Options.shortCuts.globalShortcutPlay
        onActivated: {
            console.log("shortcut--play");
            Playback.togglePlay();
        }
    }
    Shortcut {
        sequence: Options.shortCuts.back // 上一首
        context: Qt.ApplicationShortcut
        enabled: Options.settings.openShortCut && Options.shortCuts.globalShortcutBack
        onActivated: {
            console.log("shortcut--back");
            musicControlMin.lastMedia();
        }
    }
    Shortcut {
        sequence: Options.shortCuts.forward // 下一首
        context: Qt.ApplicationShortcut
        enabled: Options.settings.openShortCut && Options.shortCuts.globalShortcutForward
        onActivated: {
            console.log("shortcut--forward");
            musicControlMin.enterMedia();
        }
    }
    Shortcut {
        sequence: Options.shortCuts.playList // 播放菜单
        context: Qt.ApplicationShortcut
        enabled: Options.settings.openShortCut && Options.shortCuts.globalShortcutPlayList
        onActivated: {
            console.log("shortcut--playList");
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
        enabled: Options.settings.openShortCut && Options.shortCuts.globalShortcutMusicControl
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

    // 辅助快捷键：音量 / 精确跳转 / 静音 / A-B / 收藏 / 播放器选项
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
            { k: "playerOptions", a: function() { musicControlMin.openPlayerOptions() } }
        ]
        delegate: Shortcut {
            readonly property string flag: "globalShortcut" + modelData.k.charAt(0).toUpperCase() + modelData.k.slice(1)
            sequence: Options.shortCuts[modelData.k]
            context: Qt.ApplicationShortcut
            enabled: Options.settings.openShortCut && Options.shortCuts[flag]
            onActivated: modelData.a()
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

    // 顶部栏 - 与qwindowkit和window耦合，难抽为组件
    Rectangle {
        id: titleBar
        x: 0
        y: 0
        z: 10
        width: window.width
        height: 60
        color: "transparent"

        // 此组件创建时，将此组件与 qwindowkit 绑定，标题栏事件由此传入
        Component.onCompleted: windowAgent.setTitleBar(titleBar);

        Item {
            id: appTitleBlock
            x: 10
            y: 10
            width: 180
            height: 40
            //visible: !window.isMacOS
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
                leftPadding: 16
                placeholderText: "搜索"
                color: Style.themes.textColor
                font.pixelSize: Style.settings.textmain
                verticalAlignment: Text.AlignVCenter
                selectionColor: Style.themes.containColor
                focus: false
                onReleased: searchCard.open();
                onAccepted: {
                    if(text.trim() == "") {
                        mainWarn.tiped("请输入文本>-<",0);
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
                    window.doSearch(mainSearchInput.text);
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

            QWKButton {
                id: fullDesktopButton
                largeicon: true
                buttonColor: musicCenter.active ? Style.themes.containColor : "transparent"
                source: Style.darkis || mainLayout.state !== "" ? "qrc:/QueMusic/resources/window-bar/airplayd.svg" : "qrc:/QueMusic/resources/window-bar/airplay.svg"
                onClicked: {
                    if(musicCenter.active) {
                        musicCenter.active = false;
                        musicCenter.source = "";
                    } else {
                        musicCenter.active = true;
                    }
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
                Component.onCompleted: {
                    if(window.isMacOS) {
                        visible = false;
                    } else {
                        windowAgent.setSystemButton(WindowAgent.Minimize, minButton);
                    }
                }
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
                readonly property string unhover: Style.darkis || mainLayout.state !== "" ? "qrc:/QueMusic/resources/window-bar/closed.svg" : "qrc:/QueMusic/resources/window-bar/close.svg"
                id: closeButton
                source: closeButton.hovered ? hover : unhover
                hoverColor: "#ee4848"
                onClicked: window.toClosing();
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
        }
        ParallelAnimation {
            id: minedAnimation
            NumberAnimation { target: controlMaxLoader; property: "y"; duration: 320; from: 0; to: mainLayout.height; easing.type: Easing.Bezier; easing.bezierCurve: [ 0.23, 0.06, 0.00, 1.00, 1, 1 ] }
            NumberAnimation { target: musicControlMin; property: "musicInfoX"; duration: 320; to: 100; easing.type: Easing.Bezier; easing.bezierCurve: [ 0.23, 0.06, 0.00, 1.00, 1, 1 ] }
            onFinished: {
                controlMaxLoader.visible = false;
                controlMaxLoader.active = false;
                controlMaxLoader.hideHeight = 0;
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
            function onChangeTheme(): void {
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
            // 为优化性能，取消鼠标点击相关设计，下载封面可到关于歌曲下载
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
                barLeftWidgets.y = -48;
                minedAnimation.stop();
                visible = true;
                maxedAnimation.start();
                switch(mainLayout.maxLyricType) {
                case 0:
                    mainLayout.state = "MaxedNormal";
                    break;
                case 1:
                    mainLayout.state = "MaxedCover";
                    break;
                case 2:
                    mainLayout.state = "MaxedLyric";
                    break;
                }
            }
            width: mainLayout.width
            height: mainLayout.height
            sourceComponent: PlayerMaxCenter {}//"qrc:/QueMusic/layout/PlayerMaxCenter.qml"
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
                // 关闭帧率显示时不挂每帧回调，避免白耗 JS 调用
                enabled: Options.settings.displayFps
                target: window
                function onAfterRendering(): void { fpsCounter.frames++ }
            }
        }
    }

    // 调试模式：运行时状态面板
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
                color: Style.themes.shadowColor
                opacity: 0.75
            }
            Text {
                anchors.fill: parent
                anchors.margins: 10
                text: debugHud.info
                color: Style.themes.fontColor
                font.pixelSize: 11
                font.family: textFont.name
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
            mainLayout.state = "";
            mainLayout.visible = false;
            barLeftWidgets.y = 12;
            minedAnimation.start();
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
        onStarted: mainLayout.visible = true
        onFinished: {
            settingsView.visible = false;
            settingsView.active = false;
        }
    }

    CoverHelper {
        id: coverHelper
        onLocalCoverReady: (path, coverUrl) => {
            if (path !== window.localLyricsRequestPath)
                return;
            const localCover = coverUrl || coverHelper.findLocalCover(path);
            mainMedia.urlStr = localCover || "qrc:/QueMusic/resources/app/musicpic.png";
            if (localCover)
                colorExtractor.extractColorsFromUrl(localCover);
        }
    }

    Connections {
        target: MusicApi
        function onUrlplay(playurl: string, title: string, artist: string, cover: string,
                           solve: string, hash: string, source: int): void {
            mainMedia.urlLocal = false;
            mainMedia.noTitle = title;
            window.musicTitle = title;
            window.musicArtist = artist;
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
        // C++ 下载/提示信号
        function onWarned(text: string, type: int): void {
            mainWarn.tiped(text,type);
        }
        function onLocalLyricsReady(filePath: string, lyrics: var, translate: var): void {
            if (filePath !== window.localLyricsRequestPath)
                return;
            MusicApi.lyricsData = lyrics;
            MusicApi.lyricsTranslate = translate || [];
        }
        function onLocalLyricsFailed(filePath: string): void {
            if (filePath !== window.localLyricsRequestPath)
                return;
            MusicApi.setLocalLyrics();
        }
        function onLocalMetadataReady(filePath: string, meta: var): void {
            if (filePath !== window.localLyricsRequestPath)
                return;
            if (meta.title)
                window.musicTitle = meta.title;
            if (meta.artist)
                window.musicArtist = meta.artist;
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


    // 切歌爆音抑制
    AudioOutput { id: volumeValue; volume: Playback.outVolume; device: Options.settings.useDefaultDevice ? musicDevices.defaultAudioOutput : musicDevices.audioOutputs[Options.settings.audioDevice] }
    MediaDevices { id: musicDevices }
    // 主媒体
    GetWave {
        id: getWave
        mediaPlayer: mainMedia
        renderWindow: window
        enabled: Style.settings.waveDisplay && controlMaxLoader.visible//mainMedia.playing
        bands: 128
    }

    MediaPlayer {
        id: mainMedia
        property string noTitle
        property string urlStr: "qrc:/QueMusic/resources/app/musicpic.png"
        property string album
        property string date
        property string type
        property bool urlLocal
        property bool onMedia: mediaStatus !== MediaPlayer.NoMedia
        property int audioBit: Math.floor(mainMedia.metaData.stringValue(MediaMetaData.AudioBitRate) / 1000)
        audioOutput: volumeValue

        source: ""
        autoPlay: Options.settings.autoPlay
        onMetaDataChanged: {
            if(!urlLocal)
                return;

            const title = mainMedia.metaData.stringValue(MediaMetaData.Title);
            const artist = mainMedia.metaData.stringValue(MediaMetaData.AlbumArtist) || mainMedia.metaData.value(MediaMetaData.Author);
            const album = mainMedia.metaData.stringValue(MediaMetaData.AlbumTitle);
            const date = mainMedia.metaData.value(MediaMetaData.Date);
            const type = mainMedia.metaData.value(MediaMetaData.MediaType);
            // 内嵌封面：优先大图，退化到缩略图
            const cover = mainMedia.metaData.value(MediaMetaData.CoverArtImage) || mainMedia.metaData.value(MediaMetaData.ThumbnailImage);

            window.musicTitle = title || noTitle
            if (artist)
                window.musicArtist = artist
            if (album)
                mainMedia.album = album
            if (date)
                mainMedia.date = date.toString()
            if (type)
                mainMedia.type = type.toString()

            if (cover) {
                colorExtractor.extractColorsFromImage(cover)
                urlStr = coverHelper.convertVariantToUrl(cover)
            } else {
                //var localCover = coverHelper.findLocalCover(mainMedia.source)
                //if (localCover)
                //    colorExtractor.extractColorsFromUrl(localCover)
            }
        }

        onMediaStatusChanged: {
            if(mediaStatus === MediaPlayer.EndOfMedia) {
                if(Playback.sleepMode === 2) {
                    Playback.sleepEnd();
                    return;
                }
                switch(Options.settings.cycleIndex) {
                    case 0:
                        musicControlMin.enterMedia();
                        break;
                    case 1:
                        // 重播同样会刷新缓冲，走淡出/淡入
                        Playback.swap(function() {
                            mainMedia.position = 0;
                            mainMedia.play();
                        })
                        break;
                    case 2:
                        musicControlMin.randomMedia();
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
            Style.warned("当前歌曲无法播放，已自动跳过",0);
            if(Options.settings.cycleIndex === 2) {
                musicControlMin.randomMedia();
            } else {
                musicControlMin.enterMedia();
            }
        }

        onUrlStrChanged: {
            if (windowsSmtc.available)
                smtcUpdateMediaInfo();
        }

        // 新音轨真正起播后再淡回，避免音频设备重开的瞬间已经有音量
        onPlayingChanged: {
            if (playing)
                Playback.finishSwap();
        }
    }
    // 依据播放列表上下文动态启用/禁用 SMTC 的上一首/下一首按钮
    // （无上一首/下一首时禁用对应按钮，避免一律恒启用）
    function updateSmtcControls(): void {
        if (!windowsSmtc.available)
            return;
        const count = playListModel.count;
        const idx = playListModel.playListIndex;
        windowsSmtc.setControlsEnabled(true, true, idx < count - 1, idx > 0);
    }

    // 统一将当前曲目信息推给 SMTC。
    function smtcUpdateMediaInfo(): void {
        if (!windowsSmtc.available)
            return
        let mediaId = ""
        if (playListModel.count > 0 && playListModel.playListIndex >= 0) {
            const item = playListModel.get(playListModel.playListIndex)
            if (item)
                mediaId = item.path
        }
        windowsSmtc.updateMediaInfo(window.musicTitle, window.musicArtist, mainMedia.album, mainMedia.urlStr, mediaId)
    }

    // Windows SMTC
    WindowsSmtcManager {
        id: windowsSmtc

        Component.onCompleted: {
            windowsSmtc.initialize(window);
            updateSmtcControls();
        }
        onPlayPressed: { Playback.togglePlay() }
        onPausePressed: { Playback.togglePlay() }
        onNextPressed: { musicControlMin.enterMedia() }
        onPreviousPressed: { musicControlMin.lastMedia() }
        onSeekRequested: (pos) => { mainMedia.position = pos }
    }

    // 系统托盘：常驻托盘图标 + 原生菜单，窗口关闭时隐藏到这里
    SystemTrayManager {
        id: systemTray
        iconSource: "qrc:/QueMusic/resources/icon.ico"
        title: "QueMusic"
        playing: mainMedia.playbackState === MediaPlayer.PlayingState
        nowPlaying: window.musicTitle !== "QueMusic"
                    ? window.musicTitle + (window.musicArtist && window.musicArtist !== "Artist"
                                           ? " - " + window.musicArtist : "")
                    : ""

        onShowWindowRequested: window.restoreWindow()
        onPlayPauseRequested: Playback.togglePlay()
        onPreviousRequested: musicControlMin.lastMedia()
        onNextRequested: musicControlMin.enterMedia()
        onQuitRequested: window.quitApp()
        onActivated: (reason) => {
            // 左键单击 / 双击托盘图标：恢复主界面
            if(reason === SystemTrayManager.Trigger || reason === SystemTrayManager.DoubleClick)
                window.restoreWindow();
        }
    }

    Connections {
        target: mainMedia

        function onSourceChanged(): void {
            Playback.clearAb() // A-B 片段按曲目绑定，切歌即失效
            // 切歌/新曲目开始播放的瞬间：主动推送 position=0 并刷新时间线；
            // duration 尚未就绪（<=0）时由 C++ 侧走全零重置分支清空上一首的残留进度。
            if (windowsSmtc.available)
                windowsSmtc.updateTimeline(0, mainMedia.duration)
            updateSmtcControls()
        }
        function onDurationChanged(): void {
            // 断点续播：恢复的曲目首次拿到时长时跳转到上次位置
            if (mainMedia.duration > 0 && window.pendingSeek > 0
                && playListModel.playListIndex >= 0
                && window.pendingSeekPath === playListModel.get(playListModel.playListIndex).path) {
                mainMedia.position = Math.min(window.pendingSeek, mainMedia.duration - 1000)
                window.pendingSeek = 0
                window.pendingSeekPath = ""
            }
            // 新歌时长加载完成：以 position=0 主动推送一条完整时间线，
            // 随后 onPositionChanged 会用实时位置持续刷新。
            if (windowsSmtc.available && mainMedia.duration > 0)
                windowsSmtc.updateTimeline(0, mainMedia.duration)
        }
        function onPlaybackStateChanged(): void {
            updateSmtcControls()
            if (mainMedia.playbackState === MediaPlayer.PlayingState)
                window.noteNowPlaying()
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
        function onPositionChanged(): void {
            if (!windowsSmtc.available)
                return
            const now = Date.now()
            const jump = Math.abs(mainMedia.position - window.smtcLastPosition)
            if (now - window.smtcLastTimeline < 5000 && jump < 3000
                && mainMedia.duration - mainMedia.position > 5000)
                return
            window.smtcLastTimeline = now
            window.smtcLastPosition = mainMedia.position
            windowsSmtc.updateTimeline(mainMedia.position, mainMedia.duration)
        }
    }

    onMusicTitleChanged: {
        if (windowsSmtc.available)
            smtcUpdateMediaInfo();
    }
    onMusicArtistChanged: {
        if (windowsSmtc.available)
            smtcUpdateMediaInfo();
    }

    // 播放列表（C++ QueueModel：O(1) 路径查找、批量操作、角色化访问）
    QueueModel {
        id: playListModel
        playListIndex: -1
        onCountChanged: updateSmtcControls()
        onPlayListIndexChanged: updateSmtcControls()
    }

    // 播放列表持久化
    function saveQueue(): void {
        const out = []
        for (let i = 0; i < playListModel.count; i++) {
            const e = playListModel.get(i)
            out.push({ name: e.name, path: e.path, songer: e.songer, source: e.source })
        }
        Options.settings.lastQueue = JSON.stringify(out)
        Options.settings.lastQueueIndex = playListModel.playListIndex
    }

    // 启动恢复上次列表，并记住断点位置（首次播放时跳转）
    function restoreSession(): void {
        if (!Options.settings.autoRestoreQueue) return
        try {
            const arr = JSON.parse(Options.settings.lastQueue || "[]")
            for (let i = 0; i < arr.length; i++) playListModel.append(arr[i])
            const idx = Options.settings.lastQueueIndex
            if (idx < 0 || idx >= playListModel.count) return
            playListModel.playListIndex = idx
            if (Options.settings.resumePosition && Options.lastSongs.position > 0) {
                window.pendingSeekPath = playListModel.get(idx).path
                window.pendingSeek = Options.lastSongs.position
            }
        } catch (err) {}
    }

    function startTrack(index: int): void {
        const e = playListModel.get(index)
        if (!e) return
        if (e.source === -1) window.playLocalSong(e.path, e.name)
        else { mainMedia.urlLocal = false; MusicApi.getMusicInfo(e.path, 0, e.source) }
    }

    function noteNowPlaying(): void {
        if (playListModel.playListIndex < 0 || playListModel.count === 0 || !window.musicTitle) return
        const e = playListModel.get(playListModel.playListIndex)
        Playback.pushHistory({
            title: window.musicTitle,
            artist: window.musicArtist,
            path: e.path,
            source: e.source,
            cover: mainMedia.urlStr || "",
            duration: Math.floor(mainMedia.duration / 1000),
            time: Date.now()
        })
    }

    Connections {
        target: Playback
        function onPlayIndex(index: int): void { window.startTrack(index) }
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

    // 沉浸模式
    Loader {
        id: musicCenter
        active: false
        asynchronous: true
        visible: status == Loader.Ready
        source: "qrc:/QueMusic/FullCenterView.qml"
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
    }
    QAlertDialog {
        id: globalDialog
        title: "Dialog"
        message: "呃呃呃呃呃呃喵？(>-<)"
        isInput: false
        blurSource: mainLayout.visible ? mainLayout : settingsView

        property var dialogCallback: null

        // 通用简单确认对话框：点击"确定"后执行 callBack 回调
        function openSimpleDialog(title: string, text: string, callBack: var): void {
            globalDialog.title = title;
            globalDialog.message = text;
            globalDialog.isInput = false;
            globalDialog.dialogCallback = callBack || null;
            globalDialog.open();
        }

        onConfirm: {
            // 若有回调则执行回调，否则保持原有默认行为（关闭窗口）
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
                mainWarn.tiped("图片正在快速加载",0);
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
