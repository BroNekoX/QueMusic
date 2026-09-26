// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// QueMusic Center —— 沉浸式主界面（独立窗口）
// 顶部：品牌 / 页面 Tab 栏 / 音源 / 窗口控制；内容区最大宽 1400 居中
// 数据来自全局单例 Playback / MusicApi / Style / Options 与根上下文模型
// （Songs / MyFolders / LocalFolders / FavoriteSongs / FavoritePlaylists / FavoriteArtists）
// 窗口 id 为 center：centers/ 内组件用 root 指自己、center 指本窗口
//
// 注意：Window 不是 Item，直接子项一律用显式宽高绑定 center.width/height，
// 不用 anchors.fill: parent —— contentItem 尺寸在构造期未就绪会让整条布局链锁死在 0。
//
import QtQuick
import QtQuick.Shapes
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
    color: "#05060b"
    visible: true

    // 复用项目组件（QListView 等）依赖 Style.themes 配色，
    // 沉浸背景恒为深色，故打开期间固定深色主题，关闭时还原
    property int savedTheme: -1

    function enter(): void {
        if (Style.settings.theme !== 1) {
            savedTheme = Style.settings.theme
            Style.settings.theme = 1
        }
        // 色板只在 darkis 变化时才刷新；启动即深色时 StyleThemes 还停在浅色默认值上，
        // 这里补发一次信号，保证中心内的 Style.themes 取到深色值
        Style.changeTheme()
        visible = true
        if (startFullscreen)
            showFullScreen()
        else
            show()
        raise()
        requestActivate()
    }
    function exit(): void {
        if (visibility === Window.FullScreen)
            showNormal()
        else
            musicCenter.active = false;
        if (savedTheme >= 0) {
            Style.settings.theme = savedTheme
            savedTheme = -1
        }
    }
    function toggleFull(): void {
        if (visibility === Window.FullScreen)
            showNormal()
        else
            showFullScreen()
    }
    onClosing: close => { close.accepted = false; exit() }

    FontLoader { id: iconFont; source: "qrc:/QueMusic/resources/fonts/feather.ttf" }

    // ==== 播放状态 ====
    readonly property AudioEngine player: Playback.player
    readonly property QueueModel queue: Playback.queue
    readonly property int trackIndex: queue ? queue.playListIndex : -1
    readonly property var track: queue && trackIndex >= 0 && trackIndex < queue.count
                                 ? queue.get(trackIndex) : null
    readonly property string songTitle: track ? track.name || "" : (player ? player.noTitle || "" : "")
    readonly property string songArtist: track ? track.songer || "" : ""
    readonly property url cover: player && player.urlStr ? player.urlStr : Options.lastSongs.cover
    readonly property bool hasMedia: player ? player.mediaStatus !== AudioEngine.NoMedia : false
    readonly property bool playing: player ? player.playing : false

    // ==== 封面主色：极光与强调色全部由它驱动 ====
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
    function coverOf(c: var): string {
        const s = c ? String(c).replace("{size}", "256") : ""
        return s !== "" ? s : "qrc:/QueMusic/resources/app/musicpic.png"
    }
    function validSource(s: var): var {
        return s !== undefined && s !== null ? s : MusicApi.songSource
    }
    function playOnline(d: var): void {
        if (!d)
            return
        const q = Options.settings.soundQuality
        const h = (q === 2 ? d.hashsq : q === 1 ? d.hashhq : d.hash) || d.hash || d.favId || d.id
        if (h)
            MusicApi.getMusicInfo(h, 0, validSource(d.source))
    }
    // 与 main.qml 的 playLocalSong 等价：换源前淡出，避免爆音
    function playLocal(path: string, name: string): void {
        if (!player || !path)
            return
        player.noTitle = name || path
        player.urlStr = "qrc:/QueMusic/resources/app/musicpic.png"
        MusicApi.setLocalLyrics()
        MusicApi.readLocalLyricsAsync(path, name || path, "", 0, true)
        Playback.swap(function() { player.source = path; player.play() })
    }
    function doSearch(text: string): void {
        const key = text.trim()
        if (key === "")
            return
        searchKey = key
        MusicApi.searchSongsResults.clear()
        MusicApi.nowIndex = 0
        MusicApi.searchSongs(key, 0, 1, 20)
        pageIndex = 5
        if (pages.status === Loader.Ready)
            pages.item.switchTab(0)
    }

    // ==== 极光背景：封面主色驱动的光团与丝带，随歌换色 ====
    Item {
        id: backdrop
        width: center.width
        height: center.height
        z: 0

        // 径向光团：真径向渐变，GPU 几何直绘、无条带
        component GlowOrb: Item {
            id: orb
            property color tint: center.c1
            property real glowRadius: 320
            property real baseX: 0
            property real baseY: 0
            property real drift: 80
            property int period: 32000
            x: baseX
            y: baseY
            width: glowRadius * 2
            height: glowRadius * 2
            Shape {
                anchors.fill: parent
                ShapePath {
                    strokeWidth: -1
                    fillGradient: RadialGradient {
                        centerX: orb.width / 2
                        centerY: orb.height / 2
                        focalX: orb.width / 2
                        focalY: orb.height / 2
                        centerRadius: Math.min(orb.width, orb.height) / 2
                        focalRadius: 0
                        GradientStop { position: 0; color: Qt.rgba(orb.tint.r, orb.tint.g, orb.tint.b, 0.46) }
                        GradientStop { position: 0.5; color: Qt.rgba(orb.tint.r, orb.tint.g, orb.tint.b, 0.15) }
                        GradientStop { position: 1; color: Qt.rgba(orb.tint.r, orb.tint.g, orb.tint.b, 0.0) }
                    }
                    PathAngleArc {
                        centerX: orb.width / 2
                        centerY: orb.height / 2
                        radiusX: Math.min(orb.width, orb.height) / 2
                        radiusY: Math.min(orb.width, orb.height) / 2
                        startAngle: 0
                        sweepAngle: 359.9
                    }
                }
            }
            SequentialAnimation {
                running: center.visible
                loops: Animation.Infinite
                NumberAnimation { target: orb; property: "x"; to: orb.baseX + orb.drift;
                                  duration: orb.period / 2; easing.type: Easing.InOutSine }
                NumberAnimation { target: orb; property: "x"; to: orb.baseX - orb.drift;
                                  duration: orb.period; easing.type: Easing.InOutSine }
            }
        }

        // 极光丝带：闭合波浪带 + 垂直渐变，边缘渐隐
        component AuroraRibbon: Shape {
            id: ribbon
            property color tint: center.c1
            property color tint2: center.c2
            property real baseY: 0
            property real waveA: 210
            property real waveB: 290
            property real waveC: 170
            property real thickness: 170
            y: baseY
            height: 560
            opacity: 0.26
            ShapePath {
                strokeWidth: -1
                fillGradient: LinearGradient {
                    x1: 0; y1: 0; x2: 0; y2: 1
                    GradientStop { position: 0.0; color: Qt.rgba(ribbon.tint.r, ribbon.tint.g, ribbon.tint.b, 0) }
                    GradientStop { position: 0.42; color: Qt.rgba(ribbon.tint.r, ribbon.tint.g, ribbon.tint.b, 0.9) }
                    GradientStop { position: 0.68; color: Qt.rgba(ribbon.tint2.r, ribbon.tint2.g, ribbon.tint2.b, 0.75) }
                    GradientStop { position: 1.0; color: Qt.rgba(ribbon.tint2.r, ribbon.tint2.g, ribbon.tint2.b, 0) }
                }
                startX: -160
                startY: ribbon.waveB
                PathCurve { x: ribbon.width * 0.22; y: ribbon.waveA }
                PathCurve { x: ribbon.width * 0.52; y: ribbon.waveB }
                PathCurve { x: ribbon.width + 160; y: ribbon.waveC }
                PathLine { x: ribbon.width + 160; y: ribbon.waveC + ribbon.thickness * 1.15 }
                PathCurve { x: ribbon.width * 0.52; y: ribbon.waveB + ribbon.thickness * 0.7 }
                PathCurve { x: ribbon.width * 0.22; y: ribbon.waveA + ribbon.thickness * 1.25 }
                PathLine { x: -160; y: ribbon.waveB + ribbon.thickness }
            }
            SequentialAnimation {
                running: center.visible
                loops: Animation.Infinite
                NumberAnimation { target: ribbon; property: "y"; to: ribbon.baseY - 30;
                                  duration: 27000; easing.type: Easing.InOutSine }
                NumberAnimation { target: ribbon; property: "y"; to: ribbon.baseY + 30;
                                  duration: 27000; easing.type: Easing.InOutSine }
            }
        }

        GlowOrb {
            tint: center.c1
            baseX: center.width * 0.08 - 320
            baseY: -240
            glowRadius: 330
            drift: 100; period: 32000
        }
        GlowOrb {
            tint: center.c2
            baseX: center.width - 600
            baseY: center.height * 0.16 - 210
            glowRadius: 310
            drift: 76; period: 41000
        }
        GlowOrb {
            tint: center.c3
            baseX: center.width * 0.3 - 280
            baseY: center.height - 410
            glowRadius: 350
            drift: 64; period: 37000
        }

        AuroraRibbon {
            tint: center.c1
            tint2: center.c2
            baseY: -60
            width: center.width
        }
        AuroraRibbon {
            tint: center.c3
            tint2: center.c2
            baseY: center.height - 400
            opacity: 0.17
            waveA: 250; waveB: 170; waveC: 300
            width: center.width
        }

        // 压暗蒙层：保证任意封面色下文字可读，底部再沉一档承托播放坞
        Rectangle {
            width: center.width
            height: center.height
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#5c05060b" }
                GradientStop { position: 0.5; color: "#7d070810" }
                GradientStop { position: 1.0; color: "#ea04050a" }
            }
        }
    }

    // 对话框类组件以 mainLayout 作为模糊源，故根容器沿用该 id
    Item {
        id: mainLayout
        width: center.width
        height: center.height

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
            x: 22
            y: 22
            width: Math.max(0, mainLayout.width - 44)
            height: 46

            Row {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 10
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "QueMusic"
                    font.family: "Poppins"
                    font.pixelSize: 19
                    font.weight: Font.DemiBold
                    font.letterSpacing: 0.4
                    color: "#f4f6fb"
                }
                Text {
                    anchors { verticalCenter: parent.verticalCenter; verticalCenterOffset: 4 }
                    text: "CENTER"
                    font.pixelSize: 10
                    font.weight: Font.DemiBold
                    font.letterSpacing: 3.2
                    color: "#7f8b9d"
                }
                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    width: devLabel.implicitWidth + 16
                    height: 20
                    radius: 10
                    color: "#22ffffff"
                    border.width: 1
                    border.color: "#2affffff"
                    Text {
                        id: devLabel
                        anchors.centerIn: parent
                        text: "DEV"
                        font.pixelSize: 10
                        font.weight: Font.DemiBold
                        font.letterSpacing: 1.5
                        color: "#c8d2e0"
                    }
                }
            }

            CenterTabs {
                anchors.centerIn: parent
                model: ["推荐", "分类", "收藏", "本地", "下载", "搜索"]
                tabWidth: 82
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
                    model: ["酷狗音乐", "网易云音乐", "哔哩哔哩", "QQ音乐(x)", "自定义源(x)"]
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

        // 播放坞固定在底部，内容区吃掉剩余高度
        CenterDock {
            id: dock
            x: 22
            y: Math.max(0, mainLayout.height - height - 22)
            width: Math.max(0, mainLayout.width - 44)
            blurSource: mainLayout
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

        Item {
            id: content
            x: 30
            y: header.y + header.height + 18
            width: Math.max(0, mainLayout.width - 60)
            height: Math.max(0, dock.y - y - 14)

            Loader {
                id: pages
                x: Math.max(0, (content.width - width) / 2)
                y: 0
                width: Math.min(content.width, center.maxContentWidth)
                height: content.height
                // 用 URL 字符串而非 Component 对象：数组/分支返回 Component 在 AOT 下不可靠
                source: {
                    switch (center.pageIndex) {
                    case 1: return "qrc:/QueMusic/centers/CenterCategoryPage.qml"
                    case 2: return "qrc:/QueMusic/centers/CenterFavoritePage.qml"
                    case 3: return "qrc:/QueMusic/centers/CenterLocalPage.qml"
                    case 4: return "qrc:/QueMusic/centers/CenterDownloadPage.qml"
                    case 5: return "qrc:/QueMusic/centers/CenterSearchPage.qml"
                    default: return "qrc:/QueMusic/centers/CenterRecommendPage.qml"
                    }
                }
                onStatusChanged: {
                    if (status === Loader.Error)
                        console.warn("[Center] 页面加载失败:", source)
                }
            }
        }

        CenterQueue {
            blurSource: mainLayout
            areaTop: content.y
            areaHeight: content.height
            areaRight: 30
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
