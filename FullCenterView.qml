// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// QueMusic Center —— 沉浸式主界面（独立窗口）
// 顶部：品牌 / 页面 Tab 栏 / 音源 / 窗口控制；内容区最大宽 1400 居中
// 数据来自全局单例 Playback / MusicApi / Style / Options 与根上下文模型
// （Songs / MyFolders / LocalFolders / FavoriteSongs / FavoritePlaylists / FavoriteArtists）
// 窗口 id 为 center：centers/ 内组件用 root 指自己、center 指本窗口
//
import QtQuick
import QtQuick.Effects
import QtMultimedia
import QueMusic 1.0
import 'qrc:/QueMusic/components'
import 'qrc:/QueMusic/centers'

Window {
    id: center

    property bool startFullscreen: true
    property int pageIndex: 0          // 0 推荐 1 分类 2 收藏 3 本地 4 下载 5 搜索
    property bool showQueue: false
    property string searchKey: ""
    property int maxContentWidth: 1400

    title: "QueMusic Center"
    width: 1360
    height: 860
    minimumWidth: 1020
    minimumHeight: 640
    color: "#06060a"
    visible: true

    // 复用项目组件（QListView 等）依赖 Style.themes 配色，
    // 沉浸背景恒为深色，故打开期间固定深色主题，关闭时还原
    property int savedTheme: -1

    function enter() {
        if (Style.settings.theme !== 1) {
            savedTheme = Style.settings.theme
            Style.settings.theme = 1
        }
        visible = true
        if (startFullscreen)
            showFullScreen()
        else
            show()
        raise()
        requestActivate()
    }
    function exit() {
        if (visibility === Window.FullScreen)
            showNormal()
        else
            hide()
        if (savedTheme >= 0) {
            Style.settings.theme = savedTheme
            savedTheme = -1
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

    // ==== 播放状态 ====
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

    // ==== 封面主色 ====
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
            center.c1 = colors[0]
            center.c2 = colors.length > 1 ? colors[1] : center.c1
            center.c3 = colors.length > 2 ? colors[2] : (colors.length === 2 ? center.c1 : "#22d3ee")
        }
    }
    onCoverChanged: extractor.extractColorsFromUrl(cover)
    Component.onCompleted: extractor.extractColorsFromUrl(cover)

    // ==== 页面共用操作 ====
    function coverOf(c) {
        var s = c ? String(c).replace("{size}", "256") : ""
        return s !== "" ? s : "qrc:/QueMusic/resources/app/musicpic.png"
    }
    function validSource(s) {
        return s !== undefined && s !== null ? s : MusicApi.songSource
    }
    function playOnline(d) {
        if (!d)
            return
        var q = Options.settings.soundQuality
        var h = q === 0 ? (d.hash || d.favId)
              : q === 1 ? (d.hashhq || d.hash || d.favId)
                        : (d.hashsq || d.hash || d.favId)
        if (h)
            MusicApi.getMusicInfo(h, 0, validSource(d.source))
    }
    // 与 main.qml 的 playLocalSong 等价：换源前淡出，避免爆音
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
    function enqueue(d) {
        if (!d)
            return
        var id = d.hash || d.favId || d.path
        if (!id)
            return
        if (Playback.indexOfPath(id) !== -1) {
            mainWarn.tiped("已在播放队列中", 0)
            return
        }
        queue.append({ name: d.title || d.name, path: id, songer: d.artist || d.singer || "",
                       source: validSource(d.source) })
        mainWarn.tiped("成功加入播放列表", 1)
    }
    function toggleFavorite(d) {
        if (!d)
            return
        var id = d.hash || d.favId || d.path
        if (!id)
            return
        if (FavoriteSongs.isFavorite(id, "song")) {
            FavoriteSongs.removeFavorite(id, "song")
            mainWarn.tiped("取消收藏", 0)
        } else {
            FavoriteSongs.addFavorite(id, d.title || d.name, d.artist || d.singer, coverOf(d.cover),
                                      validSource(d.source), d.duration || 0, "song")
            mainWarn.tiped("成功收藏", 1)
        }
    }
    function download(d) {
        if (d && (d.hash || d.favId))
            MusicApi.getMusicInfo(d.hash || d.favId, 1)
    }
    // QListView 的悬停工具：0 加入队列，1 收藏
    function toolAction(tool, d) {
        if (tool === 0) enqueue(d)
        else if (tool === 1) toggleFavorite(d)
    }
    // QListView 的右键菜单：0 下载到本地
    function menuAction(choice, d) {
        if (choice === 0) download(d)
    }
    function doSearch(text) {
        var key = text.trim()
        if (key === "")
            return
        searchKey = key
        MusicApi.searchSongsResults.clear()
        MusicApi.nowIndex = 0
        MusicApi.searchSongs(key, 0, 1, 30)
        pageIndex = 5
        if (pages.item)
            pages.item.switchTab(0)
    }

    // ==== 背景蒙层 ====
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
            case Qt.Key_Up: Playback.stepVolume(Playback.volumeStep); break
            case Qt.Key_Down: Playback.stepVolume(-Playback.volumeStep); break
            case Qt.Key_Escape: center.exit(); break
            case Qt.Key_F: center.toggleFull(); break
            case Qt.Key_M: Playback.toggleMute(); break
            case Qt.Key_Q: center.showQueue = !center.showQueue; break
            default: return
            }
            event.accepted = true
        }

        // 顶栏：品牌 / 页面 Tab / 音源 / 窗口控制
        Item {
            id: header
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: 24 }
            height: 42

            Row {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 10
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "QueMusic Center"
                    font.pixelSize: 18
                    font.family: "Poppins"
                    font.weight: Font.DemiBold
                    color: "#eef1f6"
                }
                Rectangle {
                    x: 150
                    y: 20
                    width: 40
                    height: 20
                    color: Style.themes.themeColor
                    radius: 6
                    Text {
                        anchors.centerIn: parent
                        text: "Dev"
                        font.pixelSize: 13
                        color:  Style.themes.primaryColor

                    }
                }
            }

            CenterTabs {
                anchors.centerIn: parent
                model: ["推荐", "分类", "收藏", "本地", "下载", "搜索"]
                tabWidth: 80
                currentIndex: center.pageIndex
                onTabClicked: i => center.pageIndex = i
            }

            Row {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8
                QDrop {
                    width: 118
                    height: 36
                    radius: 18
                    choice: MusicApi.songSource
                    model: ["酷狗音乐", "网易云音乐", "QQ音乐(x)", "自定义源(x)"]
                    onTransformed: i => MusicApi.songSource = i
                }
                CenterWinButtons {
                    anchors.verticalCenter: parent.verticalCenter
                    onMinimize: center.showMinimized()
                    onToggleFull: center.toggleFull()
                    onClose: center.exit()
                }
            }
        }

        Item {
            id: content
            anchors {
                left: parent.left; right: parent.right
                top: header.bottom; bottom: dock.top
                leftMargin: 28; rightMargin: 28; topMargin: 24; bottomMargin: 8
            }
            Loader {
                id: pages
                anchors { top: parent.top; bottom: parent.bottom; horizontalCenter: parent.horizontalCenter }
                width: Math.min(parent.width, center.maxContentWidth)
                sourceComponent: [recommendPage, categoryPage, favoritePage, localPage, downloadPage, searchPage][Math.min(center.pageIndex, 5)]
            }
        }

        CenterDock {
            id: dock
            anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
            anchors.margins: 24
            cover: center.cover
            title: center.songTitle
            artist: center.songArtist
            playing: center.playing
            position: center.player ? center.player.position : 0
            duration: center.player ? center.player.duration : 0
            volume: Options.settings.musicVolume
            muted: Playback.muted
            cycleIndex: Options.settings.cycleIndex
            onQueueClicked: center.showQueue = !center.showQueue
            onSeek: r => { if (center.player) center.player.position = Math.round(r * center.player.duration) }
            onVolumeMoved: v => Playback.setVolume(v)
        }

        CenterQueue {
            areaTop: content.y
            areaHeight: content.height
            areaRight: 28
            opened: center.showQueue
            queue: center.queue
            currentIndex: center.trackIndex
            onPicked: i => Playback.goTo(i)
        }

        QWarn { id: mainWarn }
    }

    Component { id: recommendPage; CenterRecommendPage { } }
    Component { id: categoryPage; CenterCategoryPage { } }
    Component { id: favoritePage; CenterFavoritePage { } }
    Component { id: localPage; CenterLocalPage { } }
    Component { id: downloadPage; CenterDownloadPage { } }
    Component { id: searchPage; CenterSearchPage { } }
}
