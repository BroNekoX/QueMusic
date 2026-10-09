// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// 沉浸播放页宿主框架：页面壳（沉浸偏移、自动隐藏控件、控制按钮、主题色动画）+ 歌词界面加载。
// 歌词界面是 layout/MainLyric.qml 或 lyricsui/*（见 lyricThemes），只依赖下方注入契约，可整包替换。
import QtQuick
import QueMusic 1.0

Rectangle {
    id: musicControlMax
    // 宿主注入：播放引擎不再靠上下文继承访问宿主的局部 id
    readonly property AudioEngine player: Playback.player
    property int hideHeight: 0
    color: "#000000"
    property color mainColor: "#00ee66"
    property color secondColor: "#00b1ee"
    property color thirdColor: "#9d4edd"

    // 歌词界面状态由宿主持有：Loader 卸载重建后不丢失
    property bool lyricBasicCd: false        // false=封面卡片 true=经典黑胶
    property int lyricThemeMode: 0           // 0=默认 1=封面 2=歌词
    property int lyricMoveMs: 0              // 歌词位置校准
    property bool lyricTranslateOpen: true

    // 歌词界面：内置界面 + 已安装插件由 LyricsPlugins 统一维护，换界面 = 换 Loader 加载的文件
    readonly property string lyricThemeSource: LyricsPlugins.source

    // 样式写入口（白名单）
    readonly property var styleAllow: ({
        basicCd: "host", lyricType: "host",        // 宿主自身状态
        premiumLyricAnime: 1, waveDisplay: 1       // 跨主题共用的外观开关
    })
    function applyStyleRequest(key: string, value: var): void {
        const spec = musicControlMax.styleAllow[key];
        if (spec === undefined) return;
        if (spec === "host") {
            if (key === "basicCd") musicControlMax.lyricBasicCd = value === true;
            else musicControlMax.lyricThemeMode = Number(value);
        } else if (spec === 1) {
            Style[key] = value;
        } else {
            const v = Number(value);
            if (!Number.isFinite(v)) return;
            Style[key] = Math.max(spec[0], Math.min(spec[1], v));
        }
    }

    // 数据注入：模块只读这些属性（缺失的跳过，便于直接换插件）；改样式一律经 applyStyleRequest
    function inject(it: Item): void {
        if (!it) return;
        const src = {
            position: () => Playback.player.position,
            playbackRate: () => Playback.player.playbackRate,
            playing: () => Playback.player.playing,
            mediaActive: () => Playback.player.onMedia,
            lyricsModel: () => MusicApi.lyricsData || [],
            translateModel: () => MusicApi.lyricsTranslate || [],
            currentIndex: () => MusicApi.lyricIndex,
            title: () => Playback.musicTitle,
            artist: () => Playback.musicArtist,
            coverUrl: () => Playback.player.urlStr || "qrc:/QueMusic/resources/app/musicpic.png",
            mainColor: () => musicControlMax.mainColor,
            secondColor: () => musicControlMax.secondColor,
            thirdColor: () => musicControlMax.thirdColor,
            hideHeight: () => musicControlMax.hideHeight,
            lyricSize: () => Style.lyricSize,
            basicCd: () => musicControlMax.lyricBasicCd,
            lyricType: () => musicControlMax.lyricThemeMode,
            lyricMove: () => musicControlMax.lyricMoveMs,
            openTranslate: () => musicControlMax.lyricTranslateOpen
        };
        for (const k in src)
            if (k in it) it[k] = Qt.binding(src[k]);
        if ("requestStyle" in it)
            it.requestStyle = musicControlMax.applyStyleRequest;
    }

    function changeColor(c1: color,c2: color,c3: color): void {
        rectcolorAnime.running = false;
        rectcolorAnime.color1 = c1;
        rectcolorAnime.color2 = c2;
        rectcolorAnime.color3 = c3;
        rectcolorAnime.running = true;
    }

    Behavior on hideHeight { enabled: musicControlMax.visible; NumberAnimation { duration: 480; easing.type: Easing.OutExpo } }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        propagateComposedEvents: false  // 阻止事件穿透
        acceptedButtons: Qt.AllButtons
        onPositionChanged: {
            if(Style.lyricHideGui) {
                musicControlMax.hideHeight = 0;
                hideDelay.running = false;
                hideDelay.running = true;
            }
        }
    }

    ParallelAnimation {
        id: rectcolorAnime
        property color color1
        property color color2
        property color color3
        ColorAnimation { target: musicControlMax; property: "mainColor"; to: rectcolorAnime.color1; duration: 320; easing.type: Easing.OutCubic }
        ColorAnimation { target: musicControlMax; property: "secondColor"; to: rectcolorAnime.color2; duration: 320; easing.type: Easing.OutCubic }
        ColorAnimation { target: musicControlMax; property: "thirdColor"; to: rectcolorAnime.color3; duration: 320; easing.type: Easing.OutCubic }
    }

    Timer {
        id: hideDelay
        interval: 3000
        running: musicControlMax.y == 0
        onTriggered: {
            if(Style.lyricHideGui && musicControlMax.y == 0) {
                musicControlMax.hideHeight = 76;
            } else {
                musicControlMax.hideHeight = 0;
            }
        }
    }

    // 歌词界面模块：换主题 = 换加载的文件；宿主不可见时卸载（active: false）
    Loader {
        id: lyricLoader
        anchors.fill: parent
        active: musicControlMax.visible
        asynchronous: true
        source: musicControlMax.lyricThemeSource
        onLoaded: musicControlMax.inject(lyricLoader.item)
        function funConnect(): void {
            if(item) {
                item.timerFunction();
            }
        }
    }

    Binding { target: MusicApi; property: "lyricFollowActive"; value: musicControlMax.visible && Playback.player.onMedia }
    Binding { target: MusicApi; property: "lyricPositionMs"; value: Playback.player.position }
    Binding { target: MusicApi; property: "lyricOffsetMs"; value: musicControlMax.lyricMoveMs }

    // 歌词界面心跳：索引由 MusicApi 计算，这里只驱动模块刷新
    Timer {
        interval: 320
        repeat: true
        running: musicControlMax.visible && Playback.player.onMedia
        onTriggered: lyricLoader.funConnect()
    }

    SButton {
        id: playerminedButton
        width: 40
        height: 40
        radius: 10
        z: 11
        buttonColor: "transparent"
        hoverColor: Qt.rgba(0, 0, 0, 0.2)
        iconColor: "#eeeeee"
        shadowEnabled: false
        x: 20
        y: Options.isMacOS ? 40 - musicControlMax.hideHeight * 2 : 10 - musicControlMax.hideHeight
        iconCharacter: "\uf096"
        iconSize: Style.texticonH
        onClicked: {
            Options.openMaxLyric.running = false;
            closeMaxLyric.running = true;
        }
    }
    SButton {
        id: centerThemeButton
        width: 40
        height: 40
        radius: 10
        z: 11
        buttonColor: "transparent"
        hoverColor: Qt.rgba(0, 0, 0, 0.2)
        iconColor: "#eeeeee"
        shadowEnabled: false
        x: 62
        y: playerminedButton.y
        iconCharacter: "\uf116"
        iconSize: Style.texticon
        onClicked: maxLyricsThemeDialog.open()
    }
    SButton {
        id: centerStyleButton
        width: 40
        height: 40
        radius: 10
        z: 11
        buttonColor: "transparent"
        hoverColor: Qt.rgba(0, 0, 0, 0.2)
        iconColor: "#eeeeee"
        shadowEnabled: false
        x: 104
        y: playerminedButton.y
        iconCharacter: "\uf005"
        iconSize: Style.texticon
        onClicked: maxLyricsDialog.open()
    }
    SButton {
        x: musicControlMax.width - 60
        y: musicControlMax.height - 130 + musicControlMax.hideHeight
        z: 5
        iconCharacter: "\uf079"
        visible: MusicApi.lyricsTranslate.length !== 0
        width: 36
        height: 36
        radius: 18
        buttonColor: musicControlMax.lyricTranslateOpen ? "#88ffffff" : "#55e1e1e1"
        hoverColor: "#42000000"
        iconColor: musicControlMax.lyricTranslateOpen ? "#555555" : "#fbfbfb"
        borderColor: "#66ffffff"
        borderWidth: 1
        iconSize: Style.texticon
        shadowEnabled: false
        tipText: "翻译"
        onClicked: {
            musicControlMax.lyricTranslateOpen = !musicControlMax.lyricTranslateOpen;
        }
    }

    // 歌词界面切换：竖排列表（内置界面 + 插件，选中即换 Loader 加载的文件）
    QOptionDialog {
        id: maxLyricsThemeDialog
        title: "歌词界面"
        width: 460
        options: Column {
            width: parent.width
            spacing: 6
            Repeater {
                model: LyricsPlugins.plugins
                delegate: Rectangle {
                    id: themeRow
                    required property string author
                    required property string preview
                    required property string name
                    required property string id
                    required property string version
                    width: parent.width
                    height: 64
                    radius: Style.labelRadius
                    color: themeRow.selected ? Theme.themeColor
                           : (themeArea.containsMouse ? Theme.hoverColor : "transparent")
                    readonly property bool selected: themeRow.id === LyricsPlugins.selectedId
                    Behavior on color { ColorAnimation { duration: 120 } }

                    QPicture {
                        x: 8
                        y: 8
                        width: 48
                        height: 48
                        radius1: 10
                        radius2: 10
                        radius3: 10
                        radius4: 10
                        source: themeRow.preview ? themeRow.preview
                                                  : "qrc:/QueMusic/resources/app/musicpic.png"
                    }

                    Column {
                        x: 68
                        anchors.verticalCenter: parent.verticalCenter
                        width: themeRow.width - 110
                        spacing: 2
                        Text {
                            width: parent.width
                            text: themeRow.name
                            elide: Text.ElideRight
                            color: themeRow.selected ? "#ffffff" : Theme.fontColor
                            font.pixelSize: Style.textmain
                            font.bold: themeRow.selected
                        }
                        Text {
                            width: parent.width
                            elide: Text.ElideRight
                            text: (themeRow.author ? themeRow.author + " · " : "") + (themeRow.version ? "v" + themeRow.version : "内置")
                            color: themeRow.selected ? "#b3ffffff" : Theme.textColor
                            font.pixelSize: Style.textTip
                        }
                    }

                    Text {
                        anchors.right: parent.right
                        anchors.rightMargin: 16
                        anchors.verticalCenter: parent.verticalCenter
                        visible: themeRow.selected
                        text: "\uf099"
                        font.family: Fonts.icon
                        font.pixelSize: Style.texticon
                        color: "#ffffff"
                    }

                    MouseArea {
                        id: themeArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: LyricsPlugins.selectedId = themeRow.id
                    }
                }
            }
        }
    }
    QOptionDialog {
        id: maxLyricsDialog
        title: "播放器样式"
        width: 640

        options: Column {
            width: parent.width
            spacing: 16
            // 常驻项：与具体歌词界面实现无关
            SettingItem {
                label: "标准歌词大小"
                width: parent.width
                QSlider {
                    anchors.right: parent.right
                    from: 0
                    to: 20
                    stepSize: 2
                    width: 160
                    height: 36
                    leftText: true
                    valueText: value
                    value: Style.lyricSize
                    onMoved: {
                        Style.lyricSize = value
                    }
                }
            }
            SettingItem {
                label: "歌词位置校准"
                width: parent.width
                QSlider {
                    anchors.right: parent.right
                    from: -5000
                    to: 5000
                    stepSize: 200
                    width: 160
                    height: 36
                    leftText: true
                    valueText: (value / 1000).toFixed(1) + "s"
                    value: musicControlMax.lyricMoveMs
                    onMoved: {
                        musicControlMax.lyricMoveMs = value;
                    }
                }
            }
            SettingItem {
                label: "自动进入沉浸模式"
                width: parent.width
                QSwitch {
                    height: 36; width: 120
                    anchors.right: parent.right
                    switchTrue: Style.lyricHideGui
                    onToggled: {
                        Style.lyricHideGui = !Style.lyricHideGui;
                        hideDelay.running = false;
                        musicControlMax.hideHeight = 0;
                    }
                }
            }
            // 以下由当前歌词界面模块提供
            Loader {
                width: parent.width
                active: maxLyricsDialog.visible
                sourceComponent: (lyricLoader.item && lyricLoader.item.styleOptions) ? lyricLoader.item.styleOptions : null
            }
        }
    }
}
