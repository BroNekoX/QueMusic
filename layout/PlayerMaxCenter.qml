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

    // 当前歌词行索引
    property int lyricIndex: 0

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
            Style.settings[key] = value;
        } else {
            const v = Number(value);
            if (!Number.isFinite(v)) return;
            Style.settings[key] = Math.max(spec[0], Math.min(spec[1], v));
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
            currentIndex: () => musicControlMax.lyricIndex,
            title: () => Playback.musicTitle,
            artist: () => Playback.musicArtist,
            coverUrl: () => Playback.player.urlStr || "qrc:/QueMusic/resources/app/musicpic.png",
            mainColor: () => musicControlMax.mainColor,
            secondColor: () => musicControlMax.secondColor,
            thirdColor: () => musicControlMax.thirdColor,
            hideHeight: () => musicControlMax.hideHeight,
            lyricSize: () => Style.settings.lyricSize,
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

    Behavior on hideHeight { enabled: musicControlMax.visible; NumberAnimation { duration: 480; easing.type: Easing.OutExpo } }

    Connections {
        target: colorExtractor
        function onColorExtractFinished(): void {
            rectcolorAnime.running = false;
            rectcolorAnime.running = true;
        }
    }

    ParallelAnimation {
        id: rectcolorAnime
        ColorAnimation { target: musicControlMax; property: "mainColor"; to: coverColor.color1; duration: 320; easing.type: Easing.OutCubic }
        ColorAnimation { target: musicControlMax; property: "secondColor"; to: coverColor.color2; duration: 320; easing.type: Easing.OutCubic }
        ColorAnimation { target: musicControlMax; property: "thirdColor"; to: coverColor.color3; duration: 320; easing.type: Easing.OutCubic }
    }

    Component.onCompleted: {
        rectcolorAnime.running = true;
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        propagateComposedEvents: false  // 阻止事件穿透
        acceptedButtons: Qt.AllButtons
        onPositionChanged: {
            if(Style.settings.lyricHideGui) {
                musicControlMax.hideHeight = 0;
                hideDelay.running = false;
                hideDelay.running = true;
            }
        }
    }

    Timer {
        id: hideDelay
        interval: 3000
        running: musicControlMax.y == 0
        onTriggered: {
            if(Style.settings.lyricHideGui && musicControlMax.y == 0) {
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
    }

    // 歌词行索引：宿主遍历列表按播放进度定位，随后驱动模块刷新
    Timer {
        id: lyricTimer
        interval: 320
        repeat: true
        running: musicControlMax.visible && Playback.player.onMedia
        onTriggered: {
            const data = MusicApi.lyricsData;
            if (data && data.length > 0) {
                const pos = Playback.player.position + musicControlMax.lyricMoveMs + 320;
                let idx = musicControlMax.lyricIndex;
                while (idx + 1 < data.length && pos >= data[idx + 1].time) idx++;
                while (idx > 0 && pos < data[idx].time) idx--;
                if (idx !== musicControlMax.lyricIndex)
                    musicControlMax.lyricIndex = idx;
            }
            const it = lyricLoader.item;
            if (it && it.timerFunction)
                it.timerFunction();
        }
    }

    // 换源/换歌：索引归零（模块自身也会重排）
    Connections {
        target: MusicApi
        function onLyricsDataChanged(): void { musicControlMax.lyricIndex = 0; }
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
        y: window.isMacOS ? 40 - musicControlMax.hideHeight * 2 : 10 - musicControlMax.hideHeight
        iconCharacter: "\uf096"
        iconSize: Style.settings.texticonH
        onClicked: {
            openMaxLyric.running = false;
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
        iconSize: Style.settings.texticon
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
        iconSize: Style.settings.texticon
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
        iconSize: Style.settings.texticon
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
                    width: parent.width
                    height: 64
                    radius: Style.settings.labelRadius
                    color: themeRow.selected ? Style.themeColor
                           : (themeArea.containsMouse ? Style.hoverColor : "transparent")
                    readonly property bool selected: modelData.id === LyricsPlugins.selectedId
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
                        source: modelData.preview ? modelData.preview
                                                  : "qrc:/QueMusic/resources/app/musicpic.png"
                    }

                    Column {
                        x: 68
                        anchors.verticalCenter: parent.verticalCenter
                        width: themeRow.width - 110
                        spacing: 2
                        Text {
                            width: parent.width
                            text: modelData.name
                            elide: Text.ElideRight
                            color: themeRow.selected ? "#ffffff" : Style.fontColor
                            font.pixelSize: Style.settings.textmain
                            font.bold: themeRow.selected
                        }
                        Text {
                            width: parent.width
                            elide: Text.ElideRight
                            text: (modelData.author ? modelData.author + " · " : "") + (modelData.version ? "v" + modelData.version : "内置")
                            color: themeRow.selected ? "#b3ffffff" : Style.textColor
                            font.pixelSize: Style.settings.textTip
                        }
                    }

                    Text {
                        anchors.right: parent.right
                        anchors.rightMargin: 16
                        anchors.verticalCenter: parent.verticalCenter
                        visible: themeRow.selected
                        text: "\uf099"
                        font.family: IconFont.name
                        font.pixelSize: Style.settings.texticon
                        color: "#ffffff"
                    }

                    MouseArea {
                        id: themeArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: LyricsPlugins.selectedId = modelData.id
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
                    value: Style.settings.lyricSize
                    onMoved: {
                        Style.settings.lyricSize = value
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
                    switchTrue: Style.settings.lyricHideGui
                    onToggled: {
                        Style.settings.lyricHideGui = !Style.settings.lyricHideGui;
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
