// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
pragma ComponentBehavior: Bound
import QtQuick
import QueMusic 1.0
import QtCore
import QWindowKit 1.0
import QtQuick.Controls.Basic

Window {
    id: window
    width: 1140
    height: 760
    minimumWidth: 810
    minimumHeight: 540
    color: Theme.primaryColor
    title: "QueMusic"

    Component.onCompleted: {
        Options.queue = Playback.queue
        Options.dialog = globalDialog
        Options.coverHelper = Playback.coverHelper
        Options.mainLayout = mainLayout
        Options.getWave = getWave
        Options.textWatch = textWatch
        Options.picWatch = picWatch
        Options.songInfo = songInfo
        Options.favPicker = favPicker

        Options.openMaxLyric = openMaxLyric
        Options.settingOutAnime = settingOutAnime
        Options.mainSearchInput = mainSearchInput
        windowAgent.setup(window);
        // dark-mode / extra-margins / title-bar-height 是 Windows 专有属性
        if(!Options.isMacOS) {
            // dwm-blur acrylic-material mica mica-alt
            windowAgent.setWindowAttribute("dark-mode", false);
            if(Config.noWindowKit) {
                windowAgent.setWindowAttribute("title-bar-height", 40);
            }
        }

        const apiCheck = MusicApi.selfCheck()
        if (!apiCheck.ok) {
            console.warn("[api self-check]", JSON.stringify(apiCheck.issues))
        } else if (Config.debug) {
            console.log("[api self-check] passed", apiCheck.passed, "checks,",
                        apiCheck.elapsedMs, "ms")
        }

        if(Config.rememberWindow && Config.winW > 0) {
            window.x = Config.winX;
            window.y = Config.winY;
            window.width = Config.winW;
            window.height = Config.winH;
        }

        window.visible = true;

        Options.changeUi();
        Options.changeTheme();

        Qt.callLater(function() {
            Playback.coverHelper.pruneCache(Config.cacheSize);
        });
    }

    // 静默更新检查已迁常驻层 UpdateChecker（main.cpp 启动时 startDeferredCheck()）


    // 标题栏是窗口拖拽区：插件挂进去的对象要标记可命中，否则点击会被拖拽吞掉
    function syncTitleBarHitTest(): void {
        const kids = barRightWidgets.children;
        for (let i = 0; i < kids.length; ++i)
            windowAgent.setHitTestVisible(kids[i], true);
    }

    function doSearch(text: string): void {
        Options.searchText = text;
        MusicApi.searchSongsResults.clear();
        mainContent.contentIndexed(5);
        Config.searchList = Config.searchList.filter(value => value !== text);
        Config.searchList.splice(0, 0, text);
        MusicApi.searchSongs(text, MusicApi.nowIndex, 1, 20);
        Options.reSearch();
        Options.exitIndex = 1;
    }

    // 系统级关闭（Alt+F4、任务栏右键"关闭窗口"）与关闭按钮行为保持一致
    onClosing: (close) => {
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
        onSessionClosing: {
            // 会话收尾（阶段 4b：常驻层保存/退出流程在此接入）
        }
    }

    Shortcut {
        sequence: "Esc"
        context: Qt.ApplicationShortcut
        enabled: !Options.recordingShortCut
        onActivated: {
            Options.exit();
            console.log("Exit");
            if(Options.exitIndex > 0) {
                Options.exitIndex -= 1;
            }
            mainLayout.forceActiveFocus();
        }
    }
    // 键位与开关都走设置页（ShortCut / globalShortcut*）
    Instantiator {
        model: [
            { k: "volumeUp",      a: function() { Playback.stepVolume(Playback.volumeStep); mainWarn.tiped("音量 " + Math.round(Playback.volume * 100) + "%", 0) } },
            { k: "volumeDown",    a: function() { Playback.stepVolume(-Playback.volumeStep); mainWarn.tiped("音量 " + Math.round(Playback.volume * 100) + "%", 0) } },
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
            required property var modelData
            readonly property string flag: "globalShortcut" + modelData.k.charAt(0).toUpperCase() + modelData.k.slice(1)
            sequence: ShortCut[modelData.k]
            context: Qt.ApplicationShortcut
            enabled: Config.openShortCut && ShortCut[flag]
            onActivated: modelData.a()
        }
    }

    WindowAgent {
        id: windowAgent
    }

    // 顶部栏 - 与qwindowkit和window耦合，难抽为组件
    Item {
        id: titleBar
        x: 0
        y: 0
        z: 10
        width: window.width
        height: 60
        //color: "transparent"
        property bool toBarLyric: musicControlMax.y === 0

        // 此组件创建时，将此组件与 qwindowkit 绑定，标题栏事件由此传入
        Component.onCompleted: windowAgent.setTitleBar(titleBar);

        Item {
            id: appTitleBlock
            x: 10
            y: 10
            width: 140
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
                source: Options.darkis ? "qrc:/QueMusic/resources/window-bar/returnd.svg" : "qrc:/QueMusic/resources/window-bar/return.svg"
                onClicked: {
                    Options.exit()
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
                color: Theme.textColor
                font.pixelSize: Style.textmain
                verticalAlignment: Text.AlignVCenter
                selectionColor: Theme.containColor
                focus: false
                onReleased: searchCard.open();
                onAccepted: {
                    if(text.trim() === "") {
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
                    color: Theme.primaryColor
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
                    if(mainSearchInput.text.trim() === "") {
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
                buttonColor: window.visibility === Window.FullScreen ? Theme.containColor : "transparent"
                source: Options.darkis || titleBar.toBarLyric ? "qrc:/QueMusic/resources/window-bar/airplayd.svg" : "qrc:/QueMusic/resources/window-bar/airplay.svg"
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
                buttonColor: settingsView.visible ? Theme.containColor : "transparent"
                source: Options.darkis || titleBar.toBarLyric ? "qrc:/QueMusic/resources/window-bar/settingd.svg" : "qrc:/QueMusic/resources/window-bar/setting.svg"
                onClicked: {
                    settingsView.active = true;
                }
                Component.onCompleted: windowAgent.setHitTestVisible(settingButton, true);
            }

            QWKButton {
                id: minButton
                source: Options.darkis || titleBar.toBarLyric ? "qrc:/QueMusic/resources/window-bar/minimized.svg" : "qrc:/QueMusic/resources/window-bar/minimize.svg"
                onClicked: window.showMinimized();
                Component.onCompleted: {
                    if(Options.isMacOS) {
                        visible = false;
                    } else {
                        windowAgent.setSystemButton(WindowAgent.Minimize, minButton);
                    }
                }
            }

            QWKButton {
                readonly property string maximized: Options.darkis || titleBar.toBarLyric ? "qrc:/QueMusic/resources/window-bar/maximized.svg" : "qrc:/QueMusic/resources/window-bar/maximize.svg"
                readonly property string restored: Options.darkis || titleBar.toBarLyric ? "qrc:/QueMusic/resources/window-bar/restored.svg" : "qrc:/QueMusic/resources/window-bar/restore.svg"
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
                    if(Options.isMacOS) {
                        visible = false;
                    } else {
                        windowAgent.setSystemButton(WindowAgent.Maximize, maxButton);
                    }
                }
            }

            QWKButton {
                readonly property string hover: Options.darkis ? "qrc:/QueMusic/resources/window-bar/close.svg" : "qrc:/QueMusic/resources/window-bar/closed.svg"
                readonly property string unhover: Options.darkis || titleBar.toBarLyric ? "qrc:/QueMusic/resources/window-bar/closed.svg" : "qrc:/QueMusic/resources/window-bar/close.svg"
                id: closeButton
                source: closeButton.hovered ? hover : unhover
                hoverColor: "#ee4848"
                onClicked: win.toClosing();
                Component.onCompleted: {
                    if(Options.isMacOS) {
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
            target: Options
            function onChangeTheme(): void {
                windowAgent.setWindowAttribute("dwm-blur", false);
                if(Style.backmode === 0) {
                    backGround.visible = false;
                    sidebar.color = Style.sidebarColor ? Theme.secondaryColor : Theme.primaryColor;
                    window.color = Theme.primaryColor;
                    mainContent.color = Theme.secondaryColor;
                } else if(Style.backmode === 1) {
                    backGround.visible = false;
                    sidebar.color = Style.sidebarColor ? Theme.blurOverlayColor : Theme.primaryBlurColor;
                    window.color = Theme.containColor;
                    mainContent.color = Theme.blurOverlayColor;
                } else if(Style.backmode === 2) {
                    backGround.visible = true;
                    sidebar.color = Style.sidebarColor ? Theme.blurOverlayColor : Theme.primaryBlurColor;
                    window.color = Theme.primaryColor;
                    mainContent.color = Theme.blurOverlayColor;
                    backGround.source = "qrc:/QueMusic/resources/pic/cloudRainbow.png";
                } else if(Style.backmode === 3) {
                    backGround.visible = true;
                    sidebar.color = Style.sidebarColor ? Theme.blurOverlayColor : Theme.primaryBlurColor;
                    window.color = Theme.primaryColor;
                    mainContent.color = Theme.blurOverlayColor;
                    switch(Style.backpic) {
                        case 0:
                            backGround.source = "qrc:/QueMusic/resources/pic/back1.jpg";
                            break;
                        case 1:
                            backGround.source = "qrc:/QueMusic/resources/pic/back2.jpg";
                            break;
                        case 2:
                            backGround.source = "qrc:/QueMusic/resources/pic/back3.jpg";
                            break;
                        case 3:
                            backGround.source = Style.backgroundImage;
                            break;
                    }
                } else if(Style.backmode === 4) {
                    backGround.visible = false;
                    sidebar.color = Theme.primaryBlurColor;
                    window.color = "transparent";
                    mainContent.color = Theme.primaryBlurColor;
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
            //sourceSize: Qt.size(width, height)
            fillMode: Image.PreserveAspectCrop

        }

        LeftSideBar {
            z: 2
            id: sidebar
            x: 0
            y: 0
            height: parent.height - 78
            mainCont: mainContent
        }

        // 页面切换主内容
        Rectangle {
            id: mainContent
            x: sidebar.width
            y: 0
            z: 1
            width: parent.width - x
            height: parent.height - 78
            color: Theme.secondaryColor
            property int pageHeight: height - 60

            // 页面顺序 = 侧边栏导航下标：首页 0 / 分类 1 / 收藏 2 / 本地 3 / 下载 4 / 搜索 5
            property list<Component> pageItem: [homePage, playlistPage, favouritePage, filePage, downloadPage, searchPage]
            property int pageIndex: 0

            function contentIndexed(choice: int): void {
                if (choice === pageIndex) return;
                pageLoader.opacity = 0;
                pageLoader.shiftY = 180;
                pageLoader.sourceComponent = pageItem[choice];
                mainContent.pageIndex = choice;
            }

            ParallelAnimation {
                id: pageAnime
                //property var target
                NumberAnimation {
                    property: "opacity"
                    target: pageLoader
                    from: 0
                    to: 1
                    duration: 280
                    easing.type: Easing.OutCubic
                }
                NumberAnimation {
                    property: "shiftY"
                    target: pageLoader
                    from: 180
                    to: 0
                    duration: 320
                    easing.type: Easing.OutExpo
                }
            }

            Loader {
                id: pageLoader
                x: 0
                y: 60
                opacity: 1
                width: mainContent.width
                height: mainContent.pageHeight
                property real shiftY: 0
                visible: true
                active: true
                sourceComponent: homePage
                transform: Translate { y: pageLoader.shiftY }
                onLoaded: { pageAnime.running = false; pageAnime.running = true }
            }

            Component { id: homePage; HomePage {} }
            Component { id: playlistPage; PlaylistPage {} }
            Component { id: favouritePage; FavouritePage {} }
            Component { id: filePage; FilePage {} }
            Component { id: downloadPage; DownloadPage {} }
            Component { id: searchPage; SearchPage {} }
        }

        PlayerControl {
            id: musicControlMin
            x: 0
            y: mainLayout.height - 78 + musicControlMax.hideHeight
            width: mainLayout.width
            height: 78
            z: 4
            onCenterChange: {
                if(musicControlMax.visible) {
                    openMaxLyric.running = false;
                    closeMaxLyric.running = true;
                } else {
                    closeMaxLyric.running = false;
                    openMaxLyric.running = true;
                }
            }
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
            onStarted: if(!settingsView.visible) barLeftWidgets.y = 12;
            onFinished: {
                musicControlMax.visible = false;
                musicControlMax.hideHeight = 0;
            }
        }

        // 取色在工作线程完成，结果回填
        ColorExtractor {
            id: colorExtractor
            imageSource: Playback.player.urlStr
            signal colorExtractFinished()
            onColorsExtracted: (colors) => {
                const color1 = colors.length > 0 ? colors[0] : "#00b1ee";
                const color2 = colors.length > 1 ? colors[1] : "#9d4edd";
                const color3 = colors.length > 2 ? colors[2] : (colors.length === 2 ? colors[0] : "#00ea64");
                musicControlMax.changeColor(color1,color2,color3);
                const mainColor = Qt.hsva(color1.hsvHue, color1.hsvSaturation * 0.6, color1.hsvValue * 0.5 + 0.1, 1.0);
                musicControlMin.changeColor(mainColor);
                colorExtractFinished();
            }

            onColorsExtractedAsString: (colors) => {
                console.log("颜色字符串:", colors);
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
        sourceComponent: SettingsView {
            backSource: backGround.source
            sidebarColor: sidebar.color
            mainContentColor: mainContent.color
        }
        opacity: visible ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 120 } }
        onLoaded: {
            visible = true;
            settingAnime.running = true;
            barLeftWidgets.y = -48;
        }
    }

    Loader {
        active: Config.displayFps
        visible: active
        anchors.top: mainLayout.top
        anchors.right: mainLayout.right
        anchors.topMargin: 68
        anchors.rightMargin: 16
        width: 78
        height: 24
        z: 99
        sourceComponent: Item {
            id: fpsCounter
            visible: true
            property int frames: 0
            property real fps: 0
            Rectangle {
                anchors.fill: parent
                radius: 12
                color: Theme.shadowColor
                opacity: 0.75
            }
            Text {
                anchors.centerIn: parent
                text: fpsCounter.fps.toFixed(0) + " FPS"
                color: Theme.fontColor
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
                enabled: Config.displayFps
                target: window
                function onAfterRendering(): void { fpsCounter.frames++ }
            }
        }
    }

    Loader {
        active: Config.debug
        asynchronous: true
        visible: status === Loader.Ready
        source: "qrc:/QueMusic/components/ConsoleWindow.qml"
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
        onStarted: {
            mainLayout.visible = true;
            barLeftWidgets.y = 12;
        }
        onFinished: {
            settingsView.visible = false;
            settingsView.active = false;
        }
    }

    GetWave {
        id: getWave
        engine: Playback.player
        renderWindow: window
        enabled: Style.waveDisplay && musicControlMax.visible
        bands: 128
    }

    // 托盘与 SMTC 已迁至 C++ 常驻层（main.cpp）：托盘信号在 C++ 侧接线，
    // 恢复窗口 / 播放控制 / 退出收尾由 SessionController + Playback 完成；
    // SMTC 以隐藏宿主窗口注册（SmtcController），主窗口销毁后仍存活。

    SearchCard {
        id: searchCard
        onSearchIndex: (index) => {
            const name = Config.searchList[index];
            mainSearchInput.text = name;
            window.doSearch(name);
            searchCard.close();
        }
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
            target: Options
            function onWarned(text: string, type: int): void {
                mainWarn.tiped(text,type);
            }
        }
        // Playback.warned 已被常驻层 WarningBuffer 捕获（QML 侧不再直连，避免双弹）；
        // 实时警告统一从 WarningBuffer.warnedArrived 接入（含 UpdateChecker 更新提示）
        Connections {
            target: WarningBuffer
            function onWarnedArrived(text: string, type: int): void {
                mainWarn.tiped(text, type);
            }
        }
        // 场景创建/重建时重放常驻层缓存的最近警告（QML 场景不在期间产生的提示不丢）
        Component.onCompleted: {
            const recent = WarningBuffer.recent()
            for (let i = 0; i < recent.length; ++i)
                mainWarn.tiped(recent[i].text, recent[i].type)
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
                font.pixelSize: Style.textH2
                color: Theme.textColor
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
    // 歌曲歌单信息-Options.songInfo.show(item, isList)
    SongInfoDialog {
        id: songInfo
    }

    // 歌曲收藏夹选择-Options.favPicker.show(item)
    FavoriteFolderDialog {
        id: favPicker
    }

    Loader {
        id: textWatch
        anchors.fill: parent
        active: false
        asynchronous: true
        visible: status === Loader.Ready
        source: "qrc:/QueMusic/components/QTextWindow.qml"
    }

    // 扩展点：整窗覆盖层（插件自己定位，默认不吃鼠标事件）
    PluginSlot { slotName: "window.overlay"; target: Overlay.overlay }
    PluginHost {
        onPluginLoaded: window.syncTitleBarHitTest()
    }
}