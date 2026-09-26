// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// 3d 歌词主题，三层：① 背景着色器（按预设换文件）② 点云舞台 LyricsStage（C++，GPU 算点位置）
// ③ 歌词平面 lyricplane.frag（固定 LOD 0 保持字清晰 + 字体发光）。
// 本主题的设置全部落在模块自己的 Settings（category "Lyrics3D"），不写宿主 StyleSettings、不走白名单。
import QtQuick
import QtCore
import QueMusic 1.0
import 'qrc:/QueMusic/components'

Item {
    id: root

    property var lyricsModel: []
    property int currentIndex: 0
    property int lyricSize: 0
    property bool openTranslate: true
    property var translateModel: []
    property color mainColor: "#00ee66"
    property color secondColor: "#00b1ee"

    readonly property real rowH: (lyricSize * 1.35 + 78) * 1.45      // 行距拉大
    readonly property real bigFont: Math.min(lyricSize * 2 + 30, width * 0.105)

    // 模块自有设置（自动持久化）
    Settings {
        id: cfg
        category: "Lyrics3D"
        property int preset: 0
        property real bright: 0.85
        property real saturate: 1.45
        property real audio: 1.0
        property real density: 1.0
        property real fieldScale: 1.0
        property real camDist: 9.0
        property real camSens: 1.0
        property real tilt: 0.5
        property real lyricScale: 1.0
        property real lyricY: 0.0
        property real lyricZ: 0.0
        property real glow: 0.55
        property real sweep: 0.7
        property real beatGlow: 0.6
        property real floatAmt: 0.4
        property string textColorCustom: ""
        property string glowColorCustom: ""
    }

    // 预设表：名称 / 背景着色器 / 点云舞台预设（-1 = 该预设不用点云）
    readonly property var presetTable: [
        { n: "方块场", s: "bg_cubes", p: -1 },
        { n: "光栅地平", s: "bg_grid", p: -1 },
        { n: "平面星空", s: "bg_stars", p: -1 },
        { n: "光棱", s: "bg_beams", p: -1 },
        { n: "星尘", s: "bg_void", p: 0 },
        { n: "光隧", s: "bg_void", p: 1 },
        { n: "星环", s: "bg_void", p: 2 },
        { n: "舞台", s: "bg_stage", p: 3 }     // 实体舞台（SDF 几何）+ 周围环绕星尘
    ]
    readonly property int presetIndex: Math.max(0, Math.min(presetTable.length - 1, cfg.preset))
    readonly property var preset: presetTable[presetIndex]

    readonly property color textColor: cfg.textColorCustom !== "" ? cfg.textColorCustom : "#ffffff"
    readonly property color glowColor: cfg.glowColorCustom !== "" ? cfg.glowColorCustom : mainColor

    // 滑杆表：[设置键, 名称, min, max, step, 显示倍数, 单位]（改动即时写入 cfg）
    readonly property var sliderTable: [
        ["bright", "背景亮度", 20, 200, 10, 100, "x"],
        ["saturate", "色彩饱和度", 100, 220, 10, 100, "x"],
        ["audio", "音频反应", 0, 200, 10, 100, "x"],
        ["density", "点云密度", 20, 200, 10, 100, "x"],
        ["fieldScale", "方块场大小", 50, 160, 10, 100, "x"],
        ["camDist", "相机距离", 60, 140, 5, 10, "x"],
        ["camSens", "旋转灵敏度", 3, 20, 1, 10, "x"],
        ["tilt", "歌词跟随鼠标", 0, 100, 10, 100, "%"],
        ["lyricScale", "歌词大小", 60, 160, 10, 100, "x"],
        ["lyricY", "歌词上下", -100, 100, 10, 100, ""],
        ["lyricZ", "歌词景深", -100, 100, 10, 100, ""],
        ["glow", "歌词溢光", 0, 150, 10, 100, "x"],
        ["sweep", "当前行发光", 0, 100, 10, 100, "%"],
        ["beatGlow", "溢光随鼓点", 0, 100, 10, 100, "%"],
        ["floatAmt", "上下浮动", 0, 100, 10, 100, "%"]
    ]
    readonly property var colorTable: [["textColorCustom", "歌词文字颜色"], ["glowColorCustom", "歌词溢光颜色"]]

    function timerFunction(): void { }           // 宿主接口：索引由宿主计算

    property real mouseX: 0
    property real mouseY: 0
    Behavior on mouseX { NumberAnimation { duration: 260; easing.type: Easing.OutCubic } }
    Behavior on mouseY { NumberAnimation { duration: 260; easing.type: Easing.OutCubic } }

    property real animTime: 0
    NumberAnimation on animTime {
        from: 0; to: 1000; duration: 1000000
        loops: Animation.Infinite
        running: root.visible
    }

    // 频谱四段 + 总电平（着色器同名 uniform；Qt 不支持从 JS 数组绑定数组 uniform）
    property real uLow: 0
    property real uMid: 0
    property real uHigh: 0
    property real uAir: 0
    property real uLevel: 0

    // HoverHandler 而非 MouseArea：不吞点击；面板打开时暂停跟随
    HoverHandler {
        id: hover
        enabled: !diyPanel.visible
        onPointChanged: {
            const p = hover.point;
            if (!p) return;
            root.mouseX = Math.max(-1, Math.min(1, p.position.x / root.width * 2 - 1));
            root.mouseY = Math.max(-1, Math.min(1, 1 - p.position.y / root.height * 2));
        }
    }

    // wavePath 首尾各一个哨兵点、y ∈ 0..80；采样 32 点折算四段，40ms 一次
    function updateSpectrum(): void {
        const p = getWave.wavePath;
        const inner = p ? p.length - 2 : 0;
        if (inner < 8) {
            root.uLow = root.uMid = root.uHigh = root.uAir = root.uLevel = 0;
            return;
        }
        let a = 0, b = 0, c = 0, d = 0;
        for (let i = 0; i < 32; i++) {
            const pt = p[1 + Math.floor(i * inner / 32)];
            const v = pt ? Math.max(0, Math.min(1, 1 - pt.y / 80)) : 0;
            if (i < 4) a += v;
            else if (i < 10) b += v;
            else if (i < 20) c += v;
            else d += v;
        }
        root.uLow = a / 4;
        root.uMid = b / 6;
        root.uHigh = c / 10;
        root.uAir = d / 12;
        root.uLevel = (root.uLow + root.uMid + root.uHigh + root.uAir) / 4;
    }

    Timer {
        interval: 40
        running: root.visible
        repeat: true
        onTriggered: root.updateSpectrum()
    }
    Component.onCompleted: root.updateSpectrum()

    // ── 1) 背景 ──
    ShaderEffect {
        anchors.fill: parent
        property color uColor: root.mainColor
        property color uColor2: root.secondColor
        property vector2d uResolution: Qt.vector2d(width, height)
        property vector2d uMouse: Qt.vector2d(root.mouseX, root.mouseY)
        property real uTime: root.animTime
        property real uLow: root.uLow
        property real uMid: root.uMid
        property real uHigh: root.uHigh
        property real uAir: root.uAir
        property real uLevel: root.uLevel
        property real uBright: cfg.bright
        property real uSaturate: cfg.saturate
        property real uAudio: cfg.audio
        property real uField: cfg.fieldScale
        property real uCamDist: cfg.camDist
        property real uCamSens: cfg.camSens
        fragmentShader: "qrc:/shaders/shaders/" + root.preset.s + ".frag.qsb"
    }

    // ── 2) 点云舞台（真 3D：GPU 算点位置 + 透视尺寸/亮度衰减）──
    LyricsStage {
        anchors.fill: parent
        visible: root.preset.p >= 0
        preset: Math.max(0, root.preset.p)
        count: 14000
        time: root.animTime
        mouseX: root.mouseX
        mouseY: root.mouseY
        audioLow: root.uLow
        audioMid: root.uMid
        audioHigh: root.uHigh
        audioAir: root.uAir
        audioLevel: root.uLevel
        color1: root.mainColor
        color2: root.secondColor
        audioGain: cfg.audio
        camDist: cfg.camDist
        camSens: cfg.camSens
        density: cfg.density * 0.85
    }

    // ── 歌词平板（纹理源）──
    ListView {
        id: lyricList
        anchors.fill: parent
        clip: true
        interactive: false
        model: root.lyricsModel
        contentY: root.currentIndex * root.rowH + root.rowH / 2 - height / 2
        Behavior on contentY { NumberAnimation { duration: 420; easing.type: Easing.BezierSpline; easing.bezierCurve: [ 0.24, 0.06, 0.00, 1.03, 1, 1 ] } }

        delegate: Item {
            width: lyricList.width
            height: root.rowH

            readonly property real dist: Math.abs(index - root.currentIndex)
            readonly property bool active: index === root.currentIndex

            opacity: Math.max(0, 1 - dist * 0.22)
            scale: active ? 1.22 : Math.max(0.60, 1 - dist * 0.10)   // 当前行明显更大
            transformOrigin: Item.Center
            Behavior on scale { NumberAnimation { duration: 360; easing.type: Easing.OutCubic } }

            Column {
                anchors.horizontalCenter: parent.horizontalCenter
                y: parent.height / 2 - height / 2
                width: parent.width * 0.74

                Text {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    maximumLineCount: 1
                    elide: Text.ElideRight
                    text: modelData.text || ""
                    color: root.textColor
                    opacity: parent.parent.active ? 1.0 : 0.72
                    font.pixelSize: root.bigFont
                    font.weight: parent.parent.active ? Font.DemiBold : Font.Normal
                    font.family: Style.settings.fontFamily
                    Behavior on opacity { NumberAnimation { duration: 360; easing.type: Easing.OutCubic } }
                }
                Text {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    maximumLineCount: 1
                    elide: Text.ElideRight
                    visible: root.openTranslate && text !== ""
                    text: root.translateModel[index] ? root.translateModel[index] : ""
                    color: root.textColor
                    opacity: 0.60
                    font.pixelSize: root.bigFont * 0.42
                    font.family: Style.settings.fontFamily
                }
            }
        }
    }

    ShaderEffectSource {
        id: lyricTex
        sourceItem: lyricList
        hideSource: true
        recursive: false
        live: root.lyricsModel.length > 0
        mipmap: false
        smooth: true
    }

    // ── 3) 歌词平面（字体发光在这里做）──
    ShaderEffect {
        anchors.fill: parent
        property color uGlowColor: root.glowColor
        property vector2d uResolution: Qt.vector2d(width, height)
        property vector2d uMouse: Qt.vector2d(root.mouseX, root.mouseY)
        property real uTime: root.animTime
        property real uGlow: cfg.glow
        property real uTilt: cfg.tilt
        property real uLineV: 0.5             // 当前行恒在纹理纵向中心（contentY 已对齐）
        property real uSweep: cfg.sweep
        property real uBeatGlow: cfg.beatGlow
        property real uLevel: root.uLevel
        property real uLyricScale: cfg.lyricScale
        property real uLyricY: cfg.lyricY
        property real uLyricZ: cfg.lyricZ
        property real uCamDist: cfg.camDist
        property real uCamSens: cfg.camSens
        property real uFloat: cfg.floatAmt
        property ShaderEffectSource uLyrics: lyricTex
        fragmentShader: "qrc:/shaders/shaders/lyricplane.frag.qsb"
    }

    // 滑杆：一份定义，面板与宿主弹窗共用
    component OptSlider: SettingItem {
        width: parent ? parent.width : 0
        property var spec
        label: spec[1]
        QSlider {
            anchors.right: parent.right
            from: spec[2]
            to: spec[3]
            stepSize: spec[4]
            width: 160
            height: 36
            leftText: true
            valueText: (value / spec[5]).toFixed(2) + spec[6]
            value: Math.round(cfg[spec[0]] * spec[5])
            onMoved: cfg[spec[0]] = value / spec[5]
        }
    }

    component OptColor: SettingItem {
        width: parent ? parent.width : 0
        property var spec
        label: spec[1]
        Rectangle {
            anchors.right: parent.right
            width: 120
            height: 36
            radius: 10
            color: cfg[spec[0]] !== "" ? cfg[spec[0]] : "#33ffffff"
            border.width: 1
            border.color: "#40ffffff"
            Text {
                anchors.centerIn: parent
                text: cfg[spec[0]] !== "" ? cfg[spec[0]].toUpperCase() : "跟随主题"
                color: "#ffffff"
                font.pixelSize: 11
            }
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    colorPick.key = spec[0];
                    colorPick.openColor(cfg[spec[0]] !== "" ? cfg[spec[0]]
                                                            : (spec[0] === "textColorCustom" ? "#ffffff" : root.mainColor));
                }
            }
        }
    }

    // ── 自定义按钮：与翻译按钮同一行、在它左边 ──
    SButton {
        id: diyButton
        x: root.width - 60 - 48
        y: root.height - 130
        z: 20
        width: 36
        height: 36
        radius: 18
        iconCharacter: "\uf013"
        iconColor: diyPanel.visible ? "#555555" : "#fbfbfb"
        iconSize: Style.settings.texticon
        buttonColor: diyPanel.visible ? "#88ffffff" : "#55e1e1e1"
        hoverColor: "#42000000"
        borderColor: "#66ffffff"
        borderWidth: 1
        shadowEnabled: false
        onClicked: diyPanel.visible = !diyPanel.visible
    }

    // ── 卡片式面板 ──
    Rectangle {
        id: diyPanel
        visible: false
        z: 21
        width: 360
        height: Math.min(root.height * 0.66, 470)
        radius: 18
        color: "#f20b0d12"
        border.width: 1
        border.color: "#33ffffff"
        anchors.right: diyButton.right
        anchors.bottom: diyButton.top
        anchors.bottomMargin: 12

        Flickable {
            anchors.fill: parent
            anchors.margins: 14
            contentWidth: width
            contentHeight: panelCol.height
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            Column {
                id: panelCol
                width: parent.width
                spacing: 14

                Text {
                    text: "背景预设"
                    color: "#7f8ea3"
                    font.pixelSize: 12
                    font.family: Style.settings.fontFamily
                }
                Grid {
                    width: parent.width
                    columns: 4
                    spacing: 8
                    Repeater {
                        model: root.presetTable
                        delegate: Rectangle {
                            width: (panelCol.width - 24) / 4
                            height: 38
                            radius: 10
                            color: index === root.presetIndex ? root.mainColor : "#26ffffff"
                            border.width: 1
                            border.color: index === root.presetIndex ? root.mainColor : "#33ffffff"
                            Text {
                                anchors.centerIn: parent
                                text: modelData.n
                                color: "#ffffff"
                                font.pixelSize: 11
                                font.family: Style.settings.fontFamily
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: cfg.preset = index
                            }
                        }
                    }
                }

                Repeater {
                    model: root.sliderTable
                    delegate: OptSlider { spec: modelData }
                }
                Repeater {
                    model: root.colorTable
                    delegate: OptColor { spec: modelData }
                }

                ColorPickerDialog {
                    id: colorPick
                    property string key: ""
                    onAccepted: cfg[colorPick.key] = selectedColor.toString()
                }
            }
        }
    }

    // ── 同一份选项也提供给宿主「播放器样式」弹窗 ──
    property Component styleOptions: Component {
        Column {
            width: parent ? parent.width : 0
            spacing: 14
            Repeater {
                model: root.sliderTable
                delegate: OptSlider { spec: modelData }
            }
        }
    }
}
