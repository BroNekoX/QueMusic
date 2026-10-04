// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// 3d 歌词主题，三层：① 背景着色器（按预设换文件）② 点云舞台 LyricsStage（C++，GPU 算点位置）
// ③ 歌词平面 lyricplane.frag（固定 LOD 0 保持字清晰 + 字体发光）。
// 本主题的设置全部落在模块自己的 Settings（category "Lyrics3D"），不写宿主 StyleSettings、不走白名单。
import QtQuick
import QtCore
import QueMusic 1.0

Item {
    id: root

    property var lyricsModel: []
    property int currentIndex: 0
    property int lyricSize: 0
    property bool openTranslate: true
    property var translateModel: []
    property color mainColor: "#00ee66"
    property color secondColor: "#00b1ee"
    property int hideHeight: 0              // 宿主沉浸模式偏移

    readonly property real rowH: (lyricSize * 1.35 + 78) * 1.45      // 行距拉大
    readonly property real bigFont: Math.min(lyricSize * 2 + 30, width * 0.105)

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
    readonly property int presetIndex: Math.max(0, Math.min(presetTable.length - 1, Lyrics3DConfig.preset))
    readonly property var preset: presetTable[presetIndex]

    readonly property color textColor: Lyrics3DConfig.textColorCustom !== "" ? Lyrics3DConfig.textColorCustom : "#ffffff"
    readonly property color glowColor: Lyrics3DConfig.glowColorCustom !== "" ? Lyrics3DConfig.glowColorCustom : mainColor

    // 滑杆表：[设置键, 名称, min, max, step, 显示倍数, 单位]（改动即时写入 Lyrics3DConfig）
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
        blocking: false
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
        property real uBright: Lyrics3DConfig.bright
        property real uSaturate: Lyrics3DConfig.saturate
        property real uAudio: Lyrics3DConfig.audio
        property real uField: Lyrics3DConfig.fieldScale
        property real uCamDist: Lyrics3DConfig.camDist
        property real uCamSens: Lyrics3DConfig.camSens
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
        audioGain: Lyrics3DConfig.audio
        camDist: Lyrics3DConfig.camDist
        camSens: Lyrics3DConfig.camSens
        density: Lyrics3DConfig.density * 0.85
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
        property real uGlow: Lyrics3DConfig.glow
        property real uTilt: Lyrics3DConfig.tilt
        property real uLineV: 0.5             // 当前行恒在纹理纵向中心（contentY 已对齐）
        property real uSweep: Lyrics3DConfig.sweep
        property real uBeatGlow: Lyrics3DConfig.beatGlow
        property real uLevel: root.uLevel
        property real uLyricScale: Lyrics3DConfig.lyricScale
        property real uLyricY: Lyrics3DConfig.lyricY
        property real uLyricZ: Lyrics3DConfig.lyricZ
        property real uCamDist: Lyrics3DConfig.camDist
        property real uCamSens: Lyrics3DConfig.camSens
        property real uFloat: Lyrics3DConfig.floatAmt
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
            value: Math.round(Lyrics3DConfig[spec[0]] * spec[5])
            onMoved: Lyrics3DConfig[spec[0]] = value / spec[5]
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
            color: Lyrics3DConfig[spec[0]] !== "" ? Lyrics3DConfig[spec[0]] : "#33ffffff"
            border.width: 1
            border.color: "#40ffffff"
            Text {
                anchors.centerIn: parent
                text: Lyrics3DConfig[spec[0]] !== "" ? Lyrics3DConfig[spec[0]].toUpperCase() : "跟随主题"
                color: "#ffffff"
                font.pixelSize: 11
            }
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    colorPick.key = spec[0];
                    colorPick.openColor(Lyrics3DConfig[spec[0]] !== "" ? Lyrics3DConfig[spec[0]]
                                                            : (spec[0] === "textColorCustom" ? "#ffffff" : root.mainColor));
                }
            }
        }
    }

    // ── 自定义按钮：与翻译按钮同一行、在它左边 ──
    SButton {
        id: diyButton
        x: root.width - 60 - 48
        y: root.height - 130 + root.hideHeight
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
        color: Style.primaryColor
        border.width: 1
        border.color: Style.sideColor
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
                            color: index === root.presetIndex ? root.mainColor : Style.primaryColor
                            border.width: 1
                            border.color: index === root.presetIndex ? root.mainColor : Style.sideColor
                            Text {
                                anchors.centerIn: parent
                                text: modelData.n
                                color: Style.textColor
                                font.pixelSize: 11
                                font.family: Style.settings.fontFamily
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: Lyrics3DConfig.preset = index
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
                    onAccepted: Lyrics3DConfig[colorPick.key] = selectedColor.toString()
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
