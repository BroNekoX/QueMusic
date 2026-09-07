// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// QueMusic Center —— 沉浸式主界面（独立窗口）
//
// 数据来源：全局单例 Playback / MusicApi / Style / Options，以及根上下文模型
//   songModel / myFolderModel / localFolderModel / favoritesSong / favoritesList / favoritesArtist
// 不依赖主窗口内部 id，可独立创建：center.enter()
//
// 复用组件：QContentCard / QPicture / SongRow / QHead / QCard / QButton / SButton /
//           QInput / QSlider / QSwitch / QDrop / SettingItem / QLoadBar / QWarn / QTip
//
import QtQuick
import QtQuick.Effects
import QtMultimedia
import QueMusic 1.0
import MeshGradientItem 1.0
import 'qrc:/QueMusic/components'

Window {
    id: root

    // ===== 对外接口 =====
    property bool startFullscreen: true
    property int pageIndex: 0          // 0 首页 1 发现 2 排行 3 歌单 4 收藏
                                       // 5 本地 6 下载 7 设置 8 搜索 9 歌单详情
    property bool showQueue: false
    property string searchKey: ""
    property string detailId: ""
    property string detailTitle: ""
    property string detailCover: ""

    title: "QueMusic Center"
    width: 1360
    height: 860
    minimumWidth: 1020
    minimumHeight: 640
    color: "#06060a"
    visible: true

    function enter() {
        visible = true
        if (startFullscreen)
            showFullScreen()
        else
            show()
        raise()
        requestActivate()
        loadPage(pageIndex)
    }
    function exit() {
        if (visibility === Window.FullScreen)
            showNormal()
        else {
            hide()
            musicCenter.active = false;
        }
    }
    function toggleFull() {
        if (visibility === Window.FullScreen)
            showNormal()
        else
            showFullScreen()
    }
    onClosing: close => { close.accepted = false; exit() }

    FontLoader { id: iconFont; source: "qrc:/QueMusic/resources/fonts/feather.ttf" }
    FontLoader { id: textFont; source: "qrc:/QueMusic/resources/fonts/poppins.ttf" }
    readonly property string uiFont: textFont.name

    // ===== 播放状态（唯一数据源：Playback 中枢） =====
    readonly property var player: Playback.player
    readonly property var queue: Playback.queue
    readonly property int trackIndex: queue ? queue.playListIndex : -1
    readonly property var track: queue && trackIndex >= 0 && trackIndex < queue.count
                                 ? queue.get(trackIndex) : null
    readonly property string songTitle: track ? track.name || "" : (player ? player.noTitle || "" : "")
    readonly property string songArtist: track ? track.songer || "" : ""
    readonly property url cover: player && player.urlStr ? player.urlStr : Options.lastSongs.cover
    readonly property bool hasMedia: player ? player.mediaStatus !== MediaPlayer.NoMedia : false
    readonly property bool playing: player ? player.playing : false
    readonly property real dur: player ? Math.max(player.duration, 1) : 1
    readonly property string nowPath: track ? track.path : ""

    // ===== 封面主色：驱动整窗氛围 =====
    property color c1: "#2b6cff"
    property color c2: "#8b5cf6"
    property color c3: "#22d3ee"
    Behavior on c1 { ColorAnimation { duration: 720; easing.type: Easing.OutCubic } }
    Behavior on c2 { ColorAnimation { duration: 720; easing.type: Easing.OutCubic } }
    Behavior on c3 { ColorAnimation { duration: 720; easing.type: Easing.OutCubic } }

    ColorExtractor {
        id: extractor
        onColorsExtracted: colors => {
            if (!colors || colors.length === 0)
                return
            root.c1 = colors[0]
            root.c2 = colors.length > 1 ? colors[1] : root.c1
            root.c3 = colors.length > 2 ? colors[2] : (colors.length === 2 ? root.c1 : "#22d3ee")
        }
    }
    onCoverChanged: extractor.extractColorsFromUrl(cover)
    Component.onCompleted: extractor.extractColorsFromUrl(cover)

    // ===== 工具 =====
    function coverOf(c) {
        var s = c ? String(c).replace("{size}", "256") : ""
        return s !== "" ? s : "qrc:/QueMusic/resources/app/musicpic.png"
    }
    function playOnline(d) {
        if (!d)
            return
        var h
        if (Options.settings.soundQuality === 0)
            h = d.hash || d.favId
        else if (Options.settings.soundQuality === 1)
            h = d.hashhq || d.hash || d.favId
        else
            h = d.hashsq || d.hash || d.favId
        if (!h)
            return
        MusicApi.getMusicInfo(h, 0, d.source !== undefined && d.source !== null ? d.source : MusicApi.songSource)
    }
    // 本地播放：与 main.qml 的 playLocalSong 等价（换源前淡出，避免爆音）
    function playLocal(path, name) {
        if (!player || !path)
            return
        player.urlLocal = true
        player.noTitle = name || path
        player.urlStr = "qrc:/QueMusic/resources/app/musicpic.png"
        MusicApi.setLocalLyrics()
        MusicApi.readLocalLyricsAsync(path, name || path, "", 0, true)
        Playback.swap(function() { player.source = path; player.play() })
    }
    function openPlaylist(d) {
        if (!d)
            return
        var id = d.hash || d.favId || d.tagid
        if (!id)
            return
        root.detailId = id
        root.detailTitle = d.title || d.name || "歌单"
        root.detailCover = coverOf(d.cover)
        MusicApi.playlistSong.clear()
        MusicApi.getPlaylistSongs(id, 1, 50)
        root.pageIndex = 9
    }
    function doSearch(text) {
        var key = text.trim()
        if (key === "")
            return
        root.searchKey = key
        MusicApi.searchSongsResults.clear()
        MusicApi.nowIndex = 0
        MusicApi.searchSongs(key, 0, 1, 30)
        root.pageIndex = 8
    }
    function loadPage(i) {
        switch (i) {
        case 0:
            MusicApi.getPersonalFm(1, 10, MusicApi.songSource)
            MusicApi.getHotPlaylists(1, 1, 12)
            MusicApi.getNewSongs(0, 1, 12)
            break
        case 1:
            MusicApi.getRecommendSongs(1, 1, 24, MusicApi.songSource)
            MusicApi.getNewSongs(0, 1, 24)
            MusicApi.getHotSingers()
            break
        case 2:
            MusicApi.getAllToplists()
            break
        case 3:
            MusicApi.getHotPlaylists(1, 1, 30)
            break
        case 5:
            myFolderModel.loadFromDatabase()
            localFolderModel.loadFromDatabase()
            break
        }
    }
    onPageIndexChanged: loadPage(pageIndex)

    // ===== 复用小部件 =====
    // 封面卡（QContentCard + 点击热区）
    component MediaCard: QContentCard {
        id: card
        signal picked
        cardColor: Style.themes.secondaryColor
        radius: Style.settings.cubeRadius
        MouseArea {
            z: 5
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: card.picked()
        }
    }

    // 卡片区：标题 + 自适应网格（模型需提供 count / 角色字段）
    component CardSection: Column {
        id: sec
        property string title: ""
        property var source: null
        property int cardWidth: 168
        signal picked(int index)

        width: parent ? parent.width : 0
        spacing: 12
        visible: source ? (source.count || 0) > 0 : false

        QHead { text: sec.title; width: sec.width }
        Flow {
            width: sec.width
            spacing: 16
            Repeater {
                model: sec.source
                delegate: MediaCard {
                    width: sec.cardWidth
                    height: sec.cardWidth + 58
                    picWidth: sec.cardWidth - 32
                    picHeight: sec.cardWidth - 32
                    picSource: root.coverOf(model.cover)
                    title: model.title || model.name || ""
                    text: model.artist || model.singer || ""
                    onPicked: sec.picked(index)
                }
            }
        }
    }

    // 歌曲列表（SongRow 委托；字段自动兼容在线 / 本地 / 收藏模型）
    component SongList: ListView {
        id: sl
        clip: true
        spacing: 4
        signal picked(int index, var data)
        delegate: Item {
            width: sl.width
            height: 42
            required property int index
            // 委托创建时快照一行数据，避免把 model 上下文带出委托
            readonly property var row: ({
                name: model.name, title: model.title, singer: model.singer, artist: model.artist,
                cover: model.cover, hash: model.hash, hashhq: model.hashhq, hashsq: model.hashsq,
                path: model.path, favId: model.favId, folderId: model.folderId, tagid: model.tagid,
                rankid: model.rankid, source: model.source, duration: model.duration
            })
            SongRow {
                x: 4
                y: 1
                width: parent.width - 8
                title: model.name || model.title || ""
                artist: model.singer || model.artist || ""
                cover: root.coverOf(model.cover)
                highlighted: (model.hash || model.path || model.favId) === root.nowPath
                onClicked: sl.picked(index, row)
            }
        }
    }

    // 细长进度 / 音量条
    component Track: Item {
        id: trk
        property real value: 0
        property bool live: false
        signal moved(real v)

        property bool seeking: false
        property real seekValue: 0
        readonly property real shown: seeking ? seekValue : value

        function clamp(v) { return Math.max(0, Math.min(1, v)) }

        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width
            height: ma.containsMouse || trk.seeking ? 8 : 5
            radius: height / 2
            color: "#eaebed"
            Behavior on height { NumberAnimation { duration: 140; easing.type: Easing.OutExpo } }
            Rectangle {
                width: parent.width * trk.shown
                height: parent.height
                radius: height / 2
                color: root.c1
            }
            Rectangle {
                x: parent.width * trk.shown - width / 2
                anchors.verticalCenter: parent.verticalCenter
                width: ma.containsMouse || trk.seeking ? 14 : 0
                height: width
                radius: width / 2
                color: "#222222"
                Behavior on width { NumberAnimation { duration: 140; easing.type: Easing.OutExpo } }
            }
        }
        MouseArea {
            id: ma
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onPressed: m => { trk.seeking = true; trk.seekValue = trk.clamp(m.x / width) }
            onPositionChanged: m => { if (trk.seeking) trk.seekValue = trk.clamp(m.x / width) }
            onReleased: m => { trk.seeking = false; trk.moved(trk.clamp(m.x / width)) }
            onCanceled: trk.seeking = false
        }
    }

    // 页面滚动容器：内部放一个 Column，宽度绑定到容器 id，contentHeight 绑定到该 Column
    component PageScroll: Flickable {
        anchors.fill: parent
        clip: true
        contentWidth: width
        boundsBehavior: Flickable.StopAtBounds
    }

    // ===== 背景：封面色网格渐变 + 压暗蒙版 =====
    MeshGradientItem {
        anchors.fill: parent
        coverUrl: extractor.renderUrl || root.cover
        color1: root.c1
        color2: root.c2
        color3: root.c3
        animating: root.visible && Style.settings.backFlowQuality === 0
        subDivisions: Style.settings.backFlowQuality === 0 ? 28 : 14
        flowSpeed: 0.9
        visible: Style.settings.backFlowQuality !== 2
    }
    Rectangle {
        anchors.fill: parent
        visible: Style.settings.backFlowQuality === 2
        gradient: Gradient {
            GradientStop { position: 0; color: root.c1 }
            GradientStop { position: 1; color: root.c2 }
        }
    }
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#5c000000" }
            GradientStop { position: 0.45; color: "#9905070c" }
            GradientStop { position: 1.0; color: "#f0060710" }
        }
    }

    // 对话框类组件以 mainLayout 作为模糊源，故根容器沿用该 id
    Item {
        id: mainLayout
        anchors.fill: parent

        focus: true
        Keys.onPressed: event => {
            switch (event.key) {
            case Qt.Key_Space: Playback.togglePlay(); break
            case Qt.Key_Left: Playback.previous(); break
            case Qt.Key_Right: Playback.next(false); break
            case Qt.Key_Up: Playback.stepVolume(0.05); break
            case Qt.Key_Down: Playback.stepVolume(-0.05); break
            case Qt.Key_Escape: root.exit(); break
            case Qt.Key_F: root.toggleFull(); break
            case Qt.Key_M: Playback.toggleMute(); break
            case Qt.Key_Q: root.showQueue = !root.showQueue; break
            default: return
            }
            event.accepted = true
        }

        // ===== 顶栏：品牌 / 搜索 / 音源 / 窗口控制 =====
        Item {
            id: topBar
            anchors { left: parent.left; right: parent.right; top: parent.top }
            height: 64

            Row {
                anchors { left: parent.left; leftMargin: 28; verticalCenter: parent.verticalCenter }
                spacing: 12
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "QueMusic Center"
                    font.family: root.uiFont
                    font.pixelSize: 16
                    font.weight: Font.DemiBold
                    color: "#eef1f6"
                }
                Rectangle {
                    width: 36
                    height: 20
                    color: Style.themes.themeColor
                    anchors.verticalCenter: parent.verticalCenter
                    radius: 6
                    Text {
                        anchors.centerIn: parent
                        text: "Dev"
                        font.pixelSize: 12
                        color:  Style.themes.primaryColor

                    }
                }
            }

            Row {
                anchors { horizontalCenter: parent.horizontalCenter; verticalCenter: parent.verticalCenter }
                spacing: 8
                QInput {
                    id: searchInput
                    width: 300
                    height: 38
                    radius: 19
                    color: Style.themes.primaryColor
                    onEntered: root.doSearch(searchInput.inputText)
                }
                SButton {
                    iconCharacter: "\uf100"
                    width: 38
                    height: 38
                    radius: 19
                    buttonColor: "transparent"
                    hoverColor: "#1fffffff"
                    iconColor: "#eef1f6"
                    shadowEnabled: false
                    onClicked: root.doSearch(searchInput.inputText)
                }
                QDrop {
                    width: 122
                    height: 38
                    radius: 19
                    choice: MusicApi.songSource
                    model: ["酷狗音乐", "网易云音乐", "QQ音乐(x)", "自定义源(x)"]
                    onTransformed: choiced => { MusicApi.songSource = choiced; root.loadPage(root.pageIndex) }
                }
            }

            Row {
                anchors { right: parent.right; rightMargin: 20; verticalCenter: parent.verticalCenter }
                spacing: 6
                SButton {
                    iconCharacter: "\uf055"
                    width: 40
                    height: 40
                    radius: 12
                    buttonColor: "transparent"
                    iconColor: "#fcfdff"
                    iconSize: 20
                    shadowEnabled: false
                    tipText: "Account"
                }
                SButton {
                    iconCharacter: "\uf07a"
                    width: 40
                    height: 40
                    radius: 12
                    buttonColor: "transparent"
                    iconColor: "#fcfdff"
                    iconSize: 20
                    shadowEnabled: false
                    onClicked: root.toggleFull();
                    tipText: "全屏"
                }
            }
        }

        // ===== 左侧导航（Media Center 式文字导航 + 滑动高亮） =====
        Rectangle {
            id: nav
            x: 24
            y: 76
            width: 196
            height: parent.height - y - 132
            radius: 22
            color: "#5e0b0c12"
            border.width: 1
            border.color: "#1cffffff"

            Column {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 4
                Repeater {
                    model: ["首页", "发现", "排行榜", "歌单", "收藏", "本地音乐", "下载管理", "设置"]
                    delegate: Rectangle {
                        width: nav.width - 28
                        height: 42
                        radius: 12
                        color: index === root.pageIndex || navArea.containsMouse ? "#24ffffff" : "transparent"
                        Behavior on color { ColorAnimation { duration: 160 } }

                        Rectangle {
                            x: 12
                            y: 12
                            width: 4
                            height: 18
                            radius: 2
                            color: "#ffffff"
                            opacity: index === root.pageIndex ? 1 : 0
                            Behavior on opacity { NumberAnimation { duration: 180 } }
                        }
                        Text {
                            x: 26
                            width: parent.width - 36
                            height: parent.height
                            text: modelData
                            font.family: root.uiFont
                            font.pixelSize: 14
                            font.weight: index === root.pageIndex ? Font.DemiBold : Font.Normal
                            color: index === root.pageIndex ? "#ffffff" : "#c3d3dcea"
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }
                        MouseArea {
                            id: navArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.pageIndex = index
                        }
                    }
                }
            }
        }

        // ===== 主内容面板 =====
        Item {
            id: panel
            anchors {
                left: nav.right
                right: parent.right
                top: parent.top
                bottom: dock.top
                leftMargin: 18
                rightMargin: 24
                topMargin: 76
                bottomMargin: 14
            }

            readonly property var pageComps: [
                homePage, discoverPage, toplistPage, playlistsPage, favoritePage,
                localPage, downloadPage, settingPage, searchPage, playlistDetailPage
            ]

            RectangularShadow {
                anchors.fill: parent
                z: 0
                offset.y: 18
                radius: 26
                blur: 48
                spread: -6
                color: "#66000000"
            }
            Rectangle {
                anchors.fill: parent
                z: 1
                radius: 26
                color: Style.themes.primaryColor
                opacity: 0.94
            }
            Loader {
                id: pages
                z: 2
                anchors.fill: parent
                anchors.margins: 24
                clip: true
                sourceComponent: panel.pageComps[Math.min(root.pageIndex, panel.pageComps.length - 1)]
            }
        }

        // ===== 底部控制坞 =====
        Rectangle {
            id: dock
            anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
            anchors.margins: 24
            height: 90
            radius: 24
            color: "#fafcfdff"
            border.width: 3
            border.color: "#dadbdd"

            Row {
                anchors { left: parent.left; leftMargin: 28; verticalCenter: parent.verticalCenter }
                spacing: 14
                QPicture {
                    width: 60
                    height: 60
                    radius: 12
                    source: root.cover || "qrc:/QueMusic/resources/app/musicpic.png"
                }
                Column {
                    width: 186
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 3
                    Text {
                        width: parent.width
                        text: root.songTitle || "未在播放"
                        font.family: root.uiFont
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                        color: "#111111"
                        elide: Text.ElideRight
                    }
                    Text {
                        width: parent.width
                        text: root.songArtist || "—"
                        font.family: root.uiFont
                        font.pixelSize: 12
                        color: "#444444"
                        elide: Text.ElideRight
                    }
                }
            }

            Column {
                anchors { horizontalCenter: parent.horizontalCenter; verticalCenter: parent.verticalCenter }
                spacing: 4
                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 6
                    SButton {
                        iconCharacter: ["\uf118", "\uf115", "\uf0e2", "\uf03b"][Options.settings.cycleIndex]
                        width: 46
                        height: 46
                        radius: 23
                        buttonColor: "transparent"
                        iconColor: "#222222"
                        shadowEnabled: false
                        iconSize: 17
                        onClicked: Options.settings.cycleIndex = (Options.settings.cycleIndex + 1) % 4
                        tipText: "播放模式"
                    }
                    SButton {
                        iconCharacter: "\uf0dc"
                        width: 46
                        height: 46
                        radius: 23
                        buttonColor: "transparent"
                        iconColor: "#222222"
                        iconSize: 18
                        shadowEnabled: false
                        onClicked: Playback.previous()
                        tipText: "上一首"
                    }
                    SButton {
                        iconCharacter: root.playing ? "\uf02f" : "\uf00e"
                        width: 46
                        height: 46
                        radius: 23
                        buttonColor: "#f2f5f9"
                        iconColor: "#111214"
                        shadowEnabled: false
                        iconSize: 20
                        onClicked: Playback.togglePlay()
                        tipText: root.playing ? "暂停" : "播放"
                    }
                    SButton {
                        iconCharacter: "\uf0d9"
                        width: 46
                        height: 46
                        radius: 23
                        buttonColor: "transparent"
                        iconColor: "#222222"
                        iconSize: 18
                        shadowEnabled: false
                        onClicked: Playback.next(false)
                        tipText: "下一首"
                    }
                    SButton {
                        iconCharacter: "\uf0e2"
                        width: 46
                        height: 46
                        radius: 23
                        buttonColor: "transparent"
                        iconColor: "#222222"
                        iconSize: 17
                        shadowEnabled: false
                        onClicked: Playback.next(true)
                        tipText: "随机播放"
                    }
                }
                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 10
                    Text {
                        width: 44
                        text: Playback.fmt(root.player ? root.player.position : 0)
                        font.family: root.uiFont
                        font.pixelSize: 12
                        color: "#b9333333"
                        horizontalAlignment: Text.AlignRight
                    }
                    Track {
                        width: 300
                        height: 16
                        live: true
                        value: (root.player ? root.player.position : 0) / root.dur
                        onMoved: v => { if (root.player) root.player.position = v * root.dur }
                    }
                    Text {
                        width: 44
                        text: Playback.fmt(root.player ? root.player.duration : 0)
                        font.family: root.uiFont
                        font.pixelSize: 12
                        color: "#b9333333"
                    }
                }
            }

            Row {
                anchors { right: parent.right; rightMargin: 28; verticalCenter: parent.verticalCenter }
                spacing: 10
                SButton {
                    iconCharacter: "\uf043"
                    width: 40
                    height: 40
                    radius: 20
                    buttonColor: "transparent"
                    iconColor: Playback.muted ? "#8a99a8" : "#222222"
                    shadowEnabled: false
                    onClicked: Playback.toggleMute()
                    tipText: "静音"
                    WheelHandler {
                        onWheel: e => {
                            Playback.stepVolume(e.angleDelta.y > 0 ? 0.05 : -0.05)
                            e.accepted = true
                        }
                    }
                }
                Track {
                    width: 92
                    height: 16
                    anchors.verticalCenter: parent.verticalCenter
                    value: Options.settings.musicVolume
                    onMoved: v => Playback.setVolume(v)
                }
                SButton {
                    iconCharacter: "\uf098"
                    width: 40
                    height: 40
                    radius: 20
                    buttonColor: "transparent"
                    iconColor: "#222222"
                    shadowEnabled: false
                    onClicked: root.showQueue = !root.showQueue
                    tipText: "播放队列"
                }
            }
        }

        // ===== 播放队列抽屉 =====
        Rectangle {
            id: queuePanel
            width: Math.min(380, root.width * 0.3)
            height: panel.height
            y: panel.y
            x: root.showQueue ? root.width - width - 24 : root.width + 12
            visible: x < root.width
            radius: 26
            color: "#0b0c12"
            border.width: 1
            border.color: "#1fffffff"
            Behavior on x { NumberAnimation { duration: 420; easing.type: Easing.OutExpo } }

            Column {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 12
                Text {
                    text: "播放队列 · " + (root.queue ? root.queue.count : 0) + " 首"
                    font.family: root.uiFont
                    font.pixelSize: 15
                    font.weight: Font.Medium
                    color: "#f2f3f7"
                }
                ListView {
                    width: parent.width
                    height: parent.height - 44
                    model: root.queue
                    clip: true
                    spacing: 4
                    delegate: Rectangle {
                        required property int index
                        required property string name
                        required property string songer
                        readonly property bool isCurrent: index === root.trackIndex

                        width: ListView.view.width - 8
                        height: 54
                        radius: 12
                        color: isCurrent ? "#1effffff" : (rowHover.containsMouse ? "#12ffffff" : "transparent")

                        MouseArea {
                            id: rowHover
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: Playback.goTo(index)
                        }
                        Text {
                            x: 12
                            width: 30
                            anchors.verticalCenter: parent.verticalCenter
                            text: index + 1
                            font.family: root.uiFont
                            font.pixelSize: 12
                            color: isCurrent ? "#ffffff" : "#8fd7dee8"
                            horizontalAlignment: Text.AlignHCenter
                        }
                        Text {
                            anchors { left: parent.left; leftMargin: 50; right: parent.right; rightMargin: 12; top: parent.top; topMargin: 9 }
                            text: name
                            font.family: root.uiFont
                            font.pixelSize: 14
                            font.weight: isCurrent ? Font.DemiBold : Font.Normal
                            color: isCurrent ? "#ffffff" : "#cde6eaf2"
                            elide: Text.ElideRight
                        }
                        Text {
                            anchors { left: parent.left; leftMargin: 50; right: parent.right; rightMargin: 12; bottom: parent.bottom; bottomMargin: 8 }
                            text: songer
                            font.family: root.uiFont
                            font.pixelSize: 12
                            color: "#9ac9d2de"
                            elide: Text.ElideRight
                        }
                    }
                }
            }
        }

        QWarn { id: mainWarn }
    }

    // ===== 页面定义 =====
    Component {
        id: homePage
        PageScroll {
            id: homeScroll
            contentHeight: homeCol.height
            Column {
                id: homeCol
                width: homeScroll.width
                height: implicitHeight

                Item {
                    width: parent.width
                    height: 228
                    Rectangle {
                        anchors.fill: parent
                        radius: 24
                        gradient: Gradient {
                            GradientStop { position: 0; color: root.c1 }
                            GradientStop { position: 1; color: root.c2 }
                        }
                        opacity: 0.94
                    }
                    QPicture {
                        x: 28
                        y: 34
                        width: 160
                        height: 160
                        radius: 18
                        source: root.cover || "qrc:/QueMusic/resources/app/musicpic.png"
                    }
                    Column {
                        x: 216
                        y: 44
                        width: parent.width - 244
                        spacing: 8
                        Text {
                            width: parent.width
                            text: root.hasMedia ? "正在播放" : "上次播放"
                            font.family: root.uiFont
                            font.pixelSize: 13
                            color: "#e9f4f6"
                        }
                        Text {
                            width: parent.width
                            text: root.hasMedia ? root.songTitle : (Options.lastSongs.name || "还没有播放记录")
                            font.family: root.uiFont
                            font.pixelSize: 26
                            font.weight: Font.DemiBold
                            color: "#ffffff"
                            elide: Text.ElideRight
                        }
                        Text {
                            width: parent.width
                            text: root.hasMedia ? root.songArtist : Options.lastSongs.artist
                            font.family: root.uiFont
                            font.pixelSize: 14
                            color: "#e2f1f4"
                            elide: Text.ElideRight
                        }
                    }
                    Row {
                        x: 216
                        y: 150
                        spacing: 10
                        QButton {
                            text: root.playing ? "暂停" : "播放"
                            iconCharacter: root.playing ? "\uf02f" : "\uf00e"
                            width: 116
                            height: 40
                            buttonColor: "#ffffff"
                            textColor: "#14161c"
                            iconColor: "#14161c"
                            onClicked: {
                                if (!root.hasMedia && Options.lastSongs.hash !== "") {
                                    MusicApi.getMusicInfo(Options.lastSongs.hash, 0, Options.lastSongs.source)
                                    return
                                }
                                Playback.togglePlay()
                            }
                        }
                        QButton {
                            text: "下一首"
                            iconCharacter: "\uf0d9"
                            width: 106
                            height: 40
                            buttonColor: "#2affffff"
                            textColor: "#ffffff"
                            iconColor: "#ffffff"
                            shadowEnabled: false
                            onClicked: Playback.next(false)
                        }
                        QButton {
                            text: "队列 " + (root.queue ? root.queue.count : 0)
                            iconCharacter: "\uf098"
                            width: 106
                            height: 40
                            buttonColor: "#2affffff"
                            textColor: "#ffffff"
                            iconColor: "#ffffff"
                            shadowEnabled: false
                            onClicked: root.showQueue = !root.showQueue
                        }
                    }
                }

                Item { width: 1; height: 22 }

                CardSection {
                    title: "私人漫游"
                    source: MusicApi.personalFm
                    cardWidth: 132
                    onPicked: i => root.playOnline(MusicApi.personalFm.get(i))
                }
                Item { width: 1; height: 22 }
                CardSection {
                    title: "推荐歌单"
                    source: MusicApi.hotPlayLists
                    cardWidth: 158
                    onPicked: i => root.openPlaylist(MusicApi.hotPlayLists.get(i))
                }
                Item { width: 1; height: 22 }
                Column {
                    width: parent.width
                    spacing: 10
                    QHead { text: "新歌速递"; width: parent.width }
                    SongList {
                        width: parent.width
                        height: Math.min(MusicApi.newSongs.count * 46, 240)
                        model: MusicApi.newSongs
                        onPicked: (i, d) => root.playOnline(d)
                    }
                }
            }
        }
    }

    Component {
        id: discoverPage
        PageScroll {
            id: discoverScroll
            contentHeight: discoverCol.height
            Column {
                id: discoverCol
                width: discoverScroll.width
                height: implicitHeight
                CardSection {
                    title: "每日推荐"
                    source: MusicApi.recommendSongs
                    cardWidth: 148
                    onPicked: i => root.playOnline(MusicApi.recommendSongs.get(i))
                }
                Item { width: 1; height: 22 }
                CardSection {
                    title: "热门歌手"
                    source: MusicApi.singerList
                    cardWidth: 124
                    onPicked: i => {
                        var d = MusicApi.singerList.get(i)
                        root.detailId = d.hash || ""
                        root.detailTitle = d.title || d.name || "歌手"
                        root.detailCover = root.coverOf(d.cover)
                        MusicApi.playlistSong.clear()
                        MusicApi.getSingerSongs(root.detailId, 1, 50)
                        root.pageIndex = 9
                    }
                }
                Item { width: 1; height: 22 }
                Column {
                    width: parent.width
                    spacing: 10
                    QHead { text: "新歌上架"; width: parent.width }
                    SongList {
                        width: parent.width
                        height: Math.min(MusicApi.newSongs.count * 46, 460)
                        model: MusicApi.newSongs
                        onPicked: (i, d) => root.playOnline(d)
                    }
                }
            }
        }
    }

    Component {
        id: toplistPage
        PageScroll {
            id: toplistScroll
            contentHeight: toplistCol.height
            Column {
                id: toplistCol
                width: toplistScroll.width
                height: implicitHeight
                CardSection {
                    title: "全部榜单"
                    source: MusicApi.toplistList
                    cardWidth: 148
                    onPicked: i => {
                        var d = MusicApi.toplistList.get(i)
                        root.detailTitle = d.title || d.name || "榜单"
                        MusicApi.musicToplist.clear()
                        MusicApi.getMusicToplist(1, 30, d.rankid || d.hash)
                    }
                }
                Item { width: 1; height: 22 }
                Column {
                    width: parent.width
                    spacing: 10
                    visible: MusicApi.musicToplist.count > 0
                    QHead { text: root.detailTitle || "榜单歌曲"; width: parent.width }
                    SongList {
                        width: parent.width
                        height: Math.min(MusicApi.musicToplist.count * 46, 460)
                        model: MusicApi.musicToplist
                        onPicked: (i, d) => root.playOnline(d)
                    }
                }
            }
        }
    }

    Component {
        id: playlistsPage
        PageScroll {
            id: playlistsScroll
            contentHeight: playlistsCol.height
            Column {
                id: playlistsCol
                width: playlistsScroll.width
                height: implicitHeight
                CardSection {
                    title: "热门歌单"
                    source: MusicApi.hotPlayLists
                    cardWidth: 158
                    onPicked: i => root.openPlaylist(MusicApi.hotPlayLists.get(i))
                }
                Item { width: 1; height: 22 }
                CardSection {
                    title: "歌单分类"
                    source: MusicApi.getHotlistMenu
                    cardWidth: 148
                    onPicked: i => {
                        var d = MusicApi.getHotlistMenu.get(i)
                        MusicApi.musicPlaylists.clear()
                        MusicApi.getMusicPlaylists(d.tagid || d.hash, 1, 30)
                        root.detailTitle = d.title || d.name || "分类"
                    }
                }
                Item { width: 1; height: 22 }
                CardSection {
                    title: "分类歌单"
                    source: MusicApi.musicPlaylists
                    cardWidth: 148
                    onPicked: i => root.openPlaylist(MusicApi.musicPlaylists.get(i))
                }
            }
        }
    }

    Component {
        id: favoritePage
        Item {
            id: favPage
            anchors.fill: parent
            property int favTab: 0

            Column {
                anchors.fill: parent
                spacing: 12
                Row {
                    spacing: 10
                    Repeater {
                        model: ["收藏歌曲", "收藏歌单", "收藏歌手"]
                        delegate: QButton {
                            text: modelData
                            width: 112
                            height: 36
                            buttonColor: index === favPage.favTab ? Style.themes.themeColor : Style.themes.secondaryColor
                            textColor: index === favPage.favTab ? "#ffffff" : Style.themes.textColor
                            shadowEnabled: false
                            onClicked: favPage.favTab = index
                        }
                    }
                }
                Item {
                    width: parent.width
                    height: parent.height - 48
                    SongList {
                        anchors.fill: parent
                        visible: favPage.favTab === 0
                        model: favoritesSong
                        onPicked: (i, d) => {
                            if (d.source === -1)
                                root.playLocal(d.favId, d.title)
                            else
                                MusicApi.getMusicInfo(d.favId, 0, d.source)
                        }
                    }
                    SongList {
                        anchors.fill: parent
                        visible: favPage.favTab === 1
                        model: favoritesList
                        onPicked: (i, d) => root.openPlaylist(d)
                    }
                    SongList {
                        anchors.fill: parent
                        visible: favPage.favTab === 2
                        model: favoritesArtist
                        onPicked: (i, d) => {
                            root.detailId = d.favId || ""
                            root.detailTitle = d.title || "歌手"
                            root.detailCover = root.coverOf(d.cover)
                            MusicApi.playlistSong.clear()
                            MusicApi.getSingerSongs(root.detailId, 1, 50)
                            root.pageIndex = 9
                        }
                    }
                }
            }
        }
    }

    Component {
        id: localPage
        Item {
            anchors.fill: parent
            Row {
                anchors.fill: parent
                spacing: 16
                Rectangle {
                    width: 236
                    height: parent.height
                    radius: 18
                    color: Style.themes.secondaryColor
                    Column {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 6
                        QHead { text: "我的文件夹"; width: parent.width }
                        SongList {
                            width: parent.width
                            height: 200
                            model: myFolderModel
                            onPicked: (i, d) => {
                                songModel.folderId = d.folderId
                                songModel.loadByFolder(d.folderId)
                            }
                        }
                        QHead { text: "本地目录"; width: parent.width }
                        SongList {
                            width: parent.width
                            height: parent.height - 290
                            model: localFolderModel
                            onPicked: (i, d) => {
                                songModel.folderId = d.folderId
                                songModel.loadByFolder(d.folderId)
                            }
                        }
                    }
                }
                Column {
                    width: parent.width - 252
                    height: parent.height
                    spacing: 10
                    QHead { text: "歌曲 " + songModel.rowCount(); width: parent.width }
                    SongList {
                        width: parent.width
                        height: parent.height - 42
                        model: songModel
                        onPicked: (i, d) => root.playLocal(d.path, d.name)
                    }
                }
            }
        }
    }

    Component {
        id: downloadPage
        Item {
            anchors.fill: parent
            Column {
                anchors.fill: parent
                spacing: 12
                Item {
                    width: parent.width
                    height: 36
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "下载任务 " + MusicApi.downloader.taskCount + " · 已完成 " + MusicApi.downloader.completedCount
                        font.family: root.uiFont
                        font.pixelSize: Style.settings.textH2
                        font.bold: true
                        color: Style.themes.fontColor
                    }
                    Row {
                        anchors.right: parent.right
                        spacing: 10
                        QButton {
                            text: "打开目录"
                            iconCharacter: "\uf0b6"
                            height: 36
                            onClicked: Qt.openUrlExternally("file:///" + MusicApi.downloader.effectiveDownloadDir())
                        }
                        QButton {
                            text: "清除已完成"
                            iconCharacter: "\uf025"
                            height: 36
                            onClicked: MusicApi.downloader.clearCompleted()
                        }
                    }
                }
                ListView {
                    width: parent.width
                    height: parent.height - 48
                    model: MusicApi.downloader
                    clip: true
                    spacing: 6
                    delegate: Rectangle {
                        width: ListView.view.width
                        height: 56
                        radius: 12
                        color: Style.themes.secondaryColor
                        QPicture {
                            x: 10
                            y: 10
                            width: 36
                            height: 36
                            radius: 8
                            source: root.coverOf(model.cover)
                        }
                        Text {
                            anchors { left: parent.left; leftMargin: 56; right: parent.right; rightMargin: 200; top: parent.top; topMargin: 8 }
                            text: model.title || model.fileName
                            font.family: root.uiFont
                            font.pixelSize: 13
                            font.bold: true
                            color: Style.themes.fontColor
                            elide: Text.ElideRight
                        }
                        Text {
                            anchors { left: parent.left; leftMargin: 56; bottom: parent.bottom; bottomMargin: 8 }
                            text: model.artist || model.status
                            font.pixelSize: 12
                            color: Style.themes.textColor
                        }
                        QLoadBar {
                            x: parent.width - 190
                            y: 14
                            width: 100
                            progress: model.progress
                        }
                        SButton {
                            x: parent.width - 92
                            y: 10
                            width: 36
                            height: 36
                            radius: 18
                            iconCharacter: "\uf0c7"
                            buttonColor: "transparent"
                            shadowEnabled: false
                            onClicked: MusicApi.downloader.retryTask(model.taskId)
                            tipText: "重试"
                        }
                        SButton {
                            x: parent.width - 50
                            y: 10
                            width: 36
                            height: 36
                            radius: 18
                            iconCharacter: "\uf025"
                            buttonColor: "transparent"
                            shadowEnabled: false
                            onClicked: MusicApi.downloader.removeTask(model.taskId)
                            tipText: "移除"
                        }
                    }
                }
            }
        }
    }

    Component {
        id: settingPage
        PageScroll {
            id: settingScroll
            contentHeight: settingCol.height
            Column {
                id: settingCol
                width: settingScroll.width
                height: implicitHeight
                spacing: 16

                QCard {
                    width: parent.width
                    height: lookCol.height + 24
                    cardColor: Style.themes.secondaryColor
                    Column {
                        id: lookCol
                        width: parent.width
                        height: implicitHeight
                        spacing: 6
                        QHead { text: "外观"; width: parent.width }
                        SettingItem {
                            label: "主题模式"
                            width: parent.width
                            QDrop {
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                width: 150
                                height: 36
                                choice: Style.settings.theme
                                model: ["浅色", "深色", "跟随系统"]
                                onTransformed: c => Style.settings.theme = c
                            }
                        }
                        SettingItem {
                            label: "主题色"
                            width: parent.width
                            Row {
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 8
                                Repeater {
                                    model: Style.settings.colorList
                                    delegate: Rectangle {
                                        width: 26
                                        height: 26
                                        radius: 13
                                        color: modelData
                                        border.width: Style.settings.color === index ? 3 : 0
                                        border.color: Style.themes.themeColor
                                        MouseArea { anchors.fill: parent; onClicked: Style.settings.color = index }
                                    }
                                }
                            }
                        }
                        SettingItem {
                            label: "背景流动质量"
                            width: parent.width
                            QDrop {
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                width: 150
                                height: 36
                                choice: Style.settings.backFlowQuality
                                model: ["高（流动）", "中（静态网格）", "静态渐变"]
                                onTransformed: c => Style.settings.backFlowQuality = c
                            }
                        }
                    }
                }

                QCard {
                    width: parent.width
                    height: playCol.height + 24
                    cardColor: Style.themes.secondaryColor
                    Column {
                        id: playCol
                        width: parent.width
                        height: implicitHeight
                        spacing: 6
                        QHead { text: "播放"; width: parent.width }
                        SettingItem {
                            label: "音量"
                            width: parent.width
                            QSlider {
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                width: 150
                                height: 36
                                to: 100
                                valueText: Math.floor(value)
                                value: Options.settings.musicVolume * 100
                                onMoved: Options.settings.musicVolume = value / 100
                            }
                        }
                        SettingItem {
                            label: "淡入淡出"
                            width: parent.width
                            QSwitch {
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                width: 120
                                height: 36
                                switchTrue: Options.settings.fadeEnabled
                                onToggled: Options.settings.fadeEnabled = !Options.settings.fadeEnabled
                            }
                        }
                        SettingItem {
                            label: "自动播放"
                            width: parent.width
                            QSwitch {
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                width: 120
                                height: 36
                                switchTrue: Options.settings.autoPlay
                                onToggled: Options.settings.autoPlay = !Options.settings.autoPlay
                            }
                        }
                        SettingItem {
                            label: "音质优先"
                            width: parent.width
                            QDrop {
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                width: 150
                                height: 36
                                choice: Options.settings.soundQuality
                                model: ["标准", "高品", "无损"]
                                onTransformed: c => Options.settings.soundQuality = c
                            }
                        }
                        SettingItem {
                            label: "睡眠定时"
                            width: parent.width
                            QDrop {
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                width: 150
                                height: 36
                                choice: Playback.sleepMode
                                model: ["关闭", "倒计时", "本首结束"]
                                onTransformed: c => {
                                    if (c === 0)
                                        Playback.stopSleep()
                                    else
                                        Playback.armSleep(c, Options.settings.sleepMinutes)
                                }
                            }
                        }
                        SettingItem {
                            label: "跳转步长"
                            width: parent.width
                            QSlider {
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                width: 150
                                height: 36
                                from: 1
                                to: 30
                                valueText: value + " 秒"
                                value: Options.settings.seekStep
                                onMoved: Options.settings.seekStep = value
                            }
                        }
                    }
                }

                QCard {
                    width: parent.width
                    height: advCol.height + 24
                    cardColor: Style.themes.secondaryColor
                    Column {
                        id: advCol
                        width: parent.width
                        height: implicitHeight
                        spacing: 6
                        QHead { text: "高级"; width: parent.width }
                        SettingItem {
                            label: "全局快捷键"
                            width: parent.width
                            QSwitch {
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                width: 120
                                height: 36
                                switchTrue: Options.settings.openShortCut
                                onToggled: Options.settings.openShortCut = !Options.settings.openShortCut
                            }
                        }
                        SettingItem {
                            label: "显示帧率"
                            width: parent.width
                            QSwitch {
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                width: 120
                                height: 36
                                switchTrue: Options.settings.displayFps
                                onToggled: Options.settings.displayFps = !Options.settings.displayFps
                            }
                        }
                        SettingItem {
                            label: "关闭时最小化"
                            width: parent.width
                            QSwitch {
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                width: 120
                                height: 36
                                switchTrue: Options.settings.closeToManage
                                onToggled: Options.settings.closeToManage = !Options.settings.closeToManage
                            }
                        }
                    }
                }
            }
        }
    }

    Component {
        id: searchPage
        Item {
            id: searchHost
            anchors.fill: parent
            property int searchTab: 0

            Column {
                anchors.fill: parent
                spacing: 12
                Item {
                    width: parent.width
                    height: 36
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "搜索：" + root.searchKey
                        font.family: root.uiFont
                        font.pixelSize: Style.settings.textH2
                        font.bold: true
                        color: Style.themes.fontColor
                    }
                    QDrop {
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        width: 132
                        height: 36
                        choice: searchHost.searchTab
                        model: ["歌曲", "歌单", "专辑", "歌词"]
                        onTransformed: c => {
                            searchHost.searchTab = c
                            MusicApi.searchSongsResults.clear()
                            MusicApi.nowIndex = c
                            MusicApi.searchSongs(root.searchKey, c, 1, 30)
                        }
                    }
                }
                SongList {
                    width: parent.width
                    height: parent.height - 48
                    visible: searchHost.searchTab === 0 || searchHost.searchTab === 3
                    model: MusicApi.searchSongsResults
                    onPicked: (i, d) => root.playOnline(d)
                }
                CardSection {
                    title: "歌单 / 专辑"
                    source: MusicApi.searchSongsResults
                    cardWidth: 148
                    visible: searchHost.searchTab === 1 || searchHost.searchTab === 2
                    onPicked: i => root.openPlaylist(MusicApi.searchSongsResults.get(i))
                }
            }
        }
    }

    Component {
        id: playlistDetailPage
        Item {
            anchors.fill: parent
            Column {
                anchors.fill: parent
                spacing: 14
                Row {
                    width: parent.width
                    height: 120
                    spacing: 18
                    QPicture {
                        width: 112
                        height: 112
                        radius: 16
                        source: root.detailCover || "qrc:/QueMusic/resources/app/musicpic.png"
                    }
                    Column {
                        width: parent.width - 130
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 10
                        Text {
                            width: parent.width
                            text: root.detailTitle || "歌单"
                            font.family: root.uiFont
                            font.pixelSize: 22
                            font.weight: Font.DemiBold
                            color: Style.themes.fontColor
                            elide: Text.ElideRight
                        }
                        Row {
                            spacing: 10
                            QButton {
                                text: "返回"
                                iconCharacter: "\uf0dc"
                                height: 36
                                onClicked: root.pageIndex = 3
                            }
                            QButton {
                                text: "全部加入队列"
                                iconCharacter: "\uf098"
                                height: 36
                                onClicked: {
                                    var m = MusicApi.playlistSong
                                    var n = 0
                                    for (var i = 0; i < m.count; i++) {
                                        var d = m.get(i)
                                        if (d && d.hash && Playback.indexOfPath(d.hash) === -1) {
                                            root.queue.append({ name: d.title, path: d.hash,
                                                                songer: d.artist, source: MusicApi.songSource })
                                            n++
                                        }
                                    }
                                    mainWarn.tiped("已加入 " + n + " 首", 1)
                                }
                            }
                        }
                    }
                }
                SongList {
                    width: parent.width
                    height: parent.height - 134
                    model: MusicApi.playlistSong
                    onPicked: (i, d) => root.playOnline(d)
                }
            }
        }
    }
}
