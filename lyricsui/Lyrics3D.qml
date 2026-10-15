// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// 3D 歌词主题：一个相机、一个场景。背景着色器、点云舞台、歌词板共用同一套球面轨道相机，
// 拖动时整个场景一起转；歌词板放在场景正前方的世界里，不跟随相机。
pragma ComponentBehavior: Bound
import QtQuick
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

    readonly property real rowH: (lyricSize * 1.35 + 78) * 1.45
    readonly property real bigFont: Math.min(lyricSize * 2 + 30, width * 0.105)
    readonly property real tau: Math.PI * 2

    // 预设：名称 / 背景着色器 / 点云预设（-1 = 不用粒子预设，改补一层环境星屑）
    readonly property var presetTable: [
        { n: "方块场", s: "bg_cubes", p: -1 },
        { n: "平面星空", s: "bg_stars", p: -1 },
        { n: "星尘", s: "bg_void", p: 0 },
        { n: "光隧", s: "bg_void", p: 1 },
        { n: "星环", s: "bg_void", p: 2 },
        { n: "舞台", s: "bg_stage", p: 3 }
    ]
    readonly property int presetIndex: Math.max(0, Math.min(presetTable.length - 1, Lyrics3DConfig.preset))
    readonly property var preset: presetTable[presetIndex]
    readonly property int envPreset: preset.p >= 0 ? preset.p : 4
    readonly property int envCount: preset.p >= 0 ? 14000 : 4200

    readonly property color textColor: Lyrics3DConfig.textColorCustom !== "" ? Lyrics3DConfig.textColorCustom : "#ffffff"
    readonly property color glowColor: Lyrics3DConfig.glowColorCustom !== "" ? Lyrics3DConfig.glowColorCustom : mainColor

    // ── 相机 ──
    property real camTheta: 0
    property real camPhi: 0.34                              // 俯仰，限 ±1.35
    property real camTargetY: 0.30
    readonly property real camRadius: Lyrics3DConfig.camDist
    readonly property real focal: 1.0 / Math.tan(Math.max(20, Lyrics3DConfig.fov) * Math.PI / 360)
    property bool dragging: false
    property real velTheta: 0                               // 松手后的角速度
    property real velPhi: 0
    property real pressTheta: 0
    property real pressPhi: 0
    readonly property real spinSens: 0.0042 * Lyrics3DConfig.camSens

    // 镜头呼吸；拖动时减弱，免得和手抢方向
    readonly property real camThetaView: camTheta + Math.sin(animTime * 0.08) * 0.014 * (dragging ? 0.25 : 1.0)
    readonly property real camPhiView: camPhi + Math.sin(animTime * 0.06 + 1.0) * 0.010 * (dragging ? 0.25 : 1.0)

    // ── 歌词板（世界坐标）──
    // 半高是常量，不跟相机距离走：拉远相机时整块跟着变小，而不是永远占满画面。
    readonly property real lyricHalfH: 2.35 * Lyrics3DConfig.lyricScale
    readonly property real lyricY: 0.30 + Lyrics3DConfig.lyricY * 0.06
    readonly property real lyricDist: 1.60 + Lyrics3DConfig.lyricZ * 1.20   // 沿 +X 往前放多远
    readonly property real floorY: -1.05                                    // 地面高度，倒影用

    function rebaseTheta(): void {
        if (Math.abs(camTheta) > 1000)
            camTheta -= tau * Math.round(camTheta / tau);
    }

    function resetView(): void {
        _resetFromTheta = camTheta;
        _resetFromPhi = camPhi;
        velTheta = 0;
        velPhi = 0;
        camResetAnim.restart();
    }

    property real animTime: 0
    NumberAnimation on animTime {
        from: 0; to: 1000; duration: 1000000
        loops: Animation.Infinite
        running: root.visible
    }

    // 双击回正：偏航走最短角差
    property real resetProgress: 1
    property real _resetFromTheta: 0
    property real _resetFromPhi: 0
    NumberAnimation on resetProgress {
        id: camResetAnim
        running: false
        from: 0; to: 1; duration: 620; easing.type: Easing.OutCubic
    }
    onResetProgressChanged: {
        if (resetProgress >= 1)
            return;
        let d = -_resetFromTheta;
        d -= tau * Math.round(d / tau);
        camTheta = _resetFromTheta + d * resetProgress;
        camPhi = _resetFromPhi + (0.34 - _resetFromPhi) * resetProgress;
    }

    // ── 音频：峰值归一化 → 非对称 EMA → 鼓点脉冲 + 慢速副歌闸门 ──
    property real uLow: 0
    property real uMid: 0
    property real uHigh: 0
    property real uAir: 0
    property real uLevel: 0
    property real uBeat: 0
    property real uSun: 0
    property real _bassPeak: 0.03
    property real _midPeak: 0.026
    property real _trebPeak: 0.018
    property real _energyPeak: 0.030
    property real _onsetPeak: 0.10
    property real _sunAvg: 0
    property real _sunPeak: 0
    property real _lastTick: 0
    // 地形涟漪：两个槽交替，强鼓点时在圆盘内放一圈扩散波（着色器按出生时刻算波前）
    property vector4d _ripA: Qt.vector4d(0, 0, -99, 0)
    property vector4d _ripB: Qt.vector4d(0, 0, -99, 0)
    property real _ripLast: -9

    function clamp01(v: real): real { return v < 0 ? 0 : (v > 1 ? 1 : v) }

    // 起音快、释放慢；按 dt 换算，帧率无关
    function asym(prev: real, next: real, attack: real, release: real, dt: real): real {
        const k = next > prev ? attack : release;
        return prev + (next - prev) * (1 - Math.pow(1 - k, dt * 60));
    }

    // wavePath 首尾各一个哨兵点、y ∈ 0..80；采样 32 点折算四段
    function updateSpectrum(): void {
        const p = Options.getWave.wavePath;
        const inner = p ? p.length - 2 : 0;
        const dt = 0.06;
        if (inner < 8) {
            const dec = 1 - Math.pow(0.16, dt * 60);
            uLow -= uLow * dec; uMid -= uMid * dec; uHigh -= uHigh * dec;
            uAir -= uAir * dec; uLevel -= uLevel * dec;
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
        const rawBass = a / 4, rawMid = b / 6, rawHigh = c / 10, rawAir = d / 12;

        // 缓慢衰减的峰值做归一化 ⇒ 音量大小不影响观感
        _bassPeak = Math.max(0.030, _bassPeak * 0.994);
        _midPeak = Math.max(0.026, _midPeak * 0.993);
        _trebPeak = Math.max(0.018, _trebPeak * 0.992);
        _energyPeak = Math.max(0.030, _energyPeak * 0.995);
        _bassPeak = Math.max(_bassPeak, rawBass);
        _midPeak = Math.max(_midPeak, rawMid);
        _trebPeak = Math.max(_trebPeak, rawHigh);
        _energyPeak = Math.max(_energyPeak, rawAir);

        const nb = clamp01(Math.pow(rawBass / _bassPeak, 0.78));
        const nm = clamp01(Math.pow(rawMid / _midPeak, 0.86));
        const nh = clamp01(Math.pow(rawHigh / _trebPeak, 0.92));
        const ne = clamp01(Math.pow(rawAir / _energyPeak, 0.82));

        uLow = asym(uLow, nb, 0.28, 0.075, dt);
        uMid = asym(uMid, nm, 0.18, 0.060, dt);
        uHigh = asym(uHigh, nh, 0.18, 0.055, dt);
        uAir = asym(uAir, ne, 0.16, 0.055, dt);
        uLevel = (uLow + uMid + uHigh + uAir) / 4;

        // 鼓点：能量突然冲高才算
        if (nb > _onsetPeak * 1.16 && nb > 0.30)
            uBeat = Math.max(uBeat, Math.min(1.0, nb * 1.05));
        if (uBeat > 0.55 && animTime - _ripLast > 1.9) {
            _ripLast = animTime;
            const a = animTime * 2.39996;                       // 黄金角，落点铺得开
            const rr = 1.4 + Math.abs(Math.sin(animTime * 1.7)) * 5.2;
            const v = Qt.vector4d(Math.cos(a) * rr, Math.sin(a) * rr, animTime,
                                  Math.min(1.0, 0.50 + uBeat * 0.70));
            if (_ripA.z < _ripB.z)
                _ripA = v;
            else
                _ripB = v;
        }
        _onsetPeak = Math.max(0.10, _onsetPeak * 0.90);

        // 副歌闸门：长平均 + 峰值保持，只有持续的高潮段才点亮"太阳"
        const sunRaw = clamp01(uLevel * 0.36 + uHigh * 0.18 + uMid * 0.26 + uAir * 0.20);
        _sunAvg += (sunRaw - _sunAvg) * Math.min(1, 0.006 * dt * 60);
        _sunPeak = Math.max(sunRaw, _sunPeak * 0.9985);
        uSun = asym(uSun, sunRaw > Math.max(0.55, _sunAvg + 0.20, _sunPeak * 0.74) ? 1.0 : 0.0, 0.075, 0.030, dt);
    }

    Timer {
        interval: 60
        running: root.visible
        repeat: true
        onTriggered: root.updateSpectrum()
    }

    // 每帧一次：惯性甩动 + 鼓点衰减（时间无关，高刷屏手感一致）
    function tickFrame(): void {
        const now = Date.now();
        const dt = Math.min(0.05, Math.max(0.001, (now - _lastTick) / 1000));
        _lastTick = now;
        uBeat *= Math.pow(0.36, dt);
        if (uBeat < 0.002)
            uBeat = 0;
        if (dragging)
            return;
        if (velTheta !== 0 || velPhi !== 0) {
            camTheta += velTheta * dt;
            camPhi = Math.max(-1.35, Math.min(1.35, camPhi + velPhi * dt));
            const damp = Math.pow(0.90, dt * 60);
            velTheta *= damp;
            velPhi *= damp;
            if (Math.abs(velTheta) < 0.004 && Math.abs(velPhi) < 0.004) {
                velTheta = 0;
                velPhi = 0;
            }
            rebaseTheta();
        }
    }
    onAnimTimeChanged: root.tickFrame()
    Component.onCompleted: {
        _lastTick = Date.now();
        updateSpectrum();
    }

    // ── 拖动转向：按住才转，仅移动鼠标不转 ──
    DragHandler {
        id: spin
        target: null
        enabled: !diyPanel.visible
        onActiveChanged: {
            if (active) {
                camResetAnim.stop();
                root.dragging = true;
                root.velTheta = 0;
                root.velPhi = 0;
                root.pressTheta = root.camTheta;
                root.pressPhi = root.camPhi;
            } else {
                root.dragging = false;
                // 松手甩动
                root.velTheta = Math.max(-6.2, Math.min(6.2, spin.centroid.velocity.x * root.spinSens));
                root.velPhi = Math.max(-6.2, Math.min(6.2, spin.centroid.velocity.y * root.spinSens));
            }
        }
        onActiveTranslationChanged: {
            if (!spin.active)
                return;
            root.camTheta = root.pressTheta + spin.activeTranslation.x * root.spinSens;
            root.camPhi = Math.max(-1.35, Math.min(1.35,
                              root.pressPhi + spin.activeTranslation.y * root.spinSens));
            root.rebaseTheta();
        }
    }
    TapHandler {
        enabled: !diyPanel.visible
        onDoubleTapped: root.resetView()
    }
    WheelHandler {
        enabled: !diyPanel.visible
        onWheel: function (e) {
            Lyrics3DConfig.camDist = Math.max(5.0, Math.min(14.0,
                                     Lyrics3DConfig.camDist + e.angleDelta.y * 0.004));
            e.accepted = true;
        }
    }

    // ── 1) 背景 ──
    ShaderEffect {
        anchors.fill: parent
        property color uColor: root.mainColor
        property color uColor2: root.secondColor
        property vector2d uResolution: Qt.vector2d(width, height)
        property real uCamTheta: root.camThetaView
        property real uCamPhi: root.camPhiView
        property real uCamRadius: root.camRadius
        property real uFocal: root.focal
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
        property real uCamTargetY: root.camTargetY
        property real uBeat: root.uBeat
        property vector4d uRipA: root._ripA
        property vector4d uRipB: root._ripB
        fragmentShader: "qrc:/shaders/shaders/" + root.preset.s + ".frag.qsb"
    }

    // ── 2) 身后来那层点云（GPU 算位置与透视衰减）──
    LyricsStage {
        anchors.fill: parent
        preset: root.envPreset
        count: root.envCount
        time: root.animTime
        camTheta: root.camThetaView
        camPhi: root.camPhiView
        camRadius: root.camRadius
        camFocal: root.focal
        camTargetY: root.camTargetY
        audioLow: root.uLow
        audioMid: root.uMid
        audioHigh: root.uHigh
        audioAir: root.uAir
        audioLevel: root.uLevel
        color1: root.mainColor
        color2: root.secondColor
        audioGain: Lyrics3DConfig.audio
        density: Lyrics3DConfig.density * 0.85
    }

    // ── 歌词板纹理源 ──
    ListView {
        id: lyricList
        anchors.fill: parent
        clip: true
        interactive: false
        model: root.lyricsModel
        spacing: 20
        property int contentTrueY: root.currentIndex * (root.rowH + 20) + root.rowH / 2 - height * 0.4
        contentY: contentTrueY
        Behavior on contentY { NumberAnimation { duration: 420; easing.type: Easing.BezierSpline; easing.bezierCurve: [ 0.24, 0.06, 0.00, 1.03, 1, 1 ] } }

        delegate: Item {
            id: lyricDel
            width: lyricList.width
            height: root.rowH

            readonly property real dist: Math.abs(index - root.currentIndex)
            readonly property bool active: index === root.currentIndex
            required property int index
            required property var modelData

            // 景深和雾在着色器里按行距做，这里只留"当前行更亮"
            opacity: Math.max(0.16, 1 - dist * 0.18)
            scale: active ? 1.2 : 1.0
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
                    //elide: Text.ElideRight
                    text: lyricDel.modelData.text || ""
                    color: root.textColor
                    opacity: lyricDel.active ? 1.0 : 0.72
                    font.pixelSize: root.bigFont
                    font.weight: lyricDel.active ? Font.DemiBold : Font.Normal
                    font.family: Style.fontFamily
                    Behavior on opacity { NumberAnimation { duration: 360; easing.type: Easing.OutCubic } }
                }
                Text {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    maximumLineCount: 1
                    //elide: Text.ElideRight
                    visible: root.openTranslate && text !== ""
                    text: root.translateModel[lyricDel.index] ? root.translateModel[lyricDel.index] : ""
                    color: root.textColor
                    opacity: 0.60
                    font.pixelSize: root.bigFont * 0.42
                    font.family: Style.fontFamily
                }
            }
        }
    }

    ShaderEffectSource {
        id: lyricTex
        sourceItem: lyricList
        hideSource: true
        recursive: false
        live: true
        mipmap: true                 // 远行用 mip LOD 做景深虚化
        smooth: true
    }

    // 行切换瞬间刷新一次，滚动动画结束后再刷新一次（否则纹理停在旧行）
    onCurrentIndexChanged: {
        lyricTex.scheduleUpdate();
        refreshTimer.restart();
    }
    Timer {
        id: refreshTimer
        interval: 460
        onTriggered: lyricTex.scheduleUpdate()
    }

    // ── 3) 歌词板 ──
    ShaderEffect {
        anchors.fill: parent
        property color uGlowColor: root.glowColor
        property vector2d uResolution: Qt.vector2d(width, height)
        property real uTime: root.animTime
        property real uCamTheta: root.camThetaView
        property real uCamPhi: root.camPhiView
        property real uCamRadius: root.camRadius
        property real uCamTargetY: root.camTargetY
        property real uFocal: root.focal
        property real uHalfH: root.lyricHalfH
        property real uPlaneDist: root.lyricDist
        property real uYOffset: root.lyricY
        property real uFloorY: root.floorY
        property real uGlow: Lyrics3DConfig.glow
        property real uSweep: Lyrics3DConfig.sweep
        property real uBeatGlow: Lyrics3DConfig.beatGlow
        property real uLevel: root.uLevel
        property real uBeat: root.uBeat
        property real uSun: root.uSun
        property real uReflect: Lyrics3DConfig.reflect
        property real uReadab: Lyrics3DConfig.readab
        property real uFog: Lyrics3DConfig.fog
        property real uDof: Lyrics3DConfig.dof
        property real uFloat: Lyrics3DConfig.floatAmt
        property ShaderEffectSource uLyrics: lyricTex
        fragmentShader: "qrc:/shaders/shaders/lyricplane.frag.qsb"
    }

    // ── 4) 前景浮尘：盖在歌词上面 ⇒ 歌词处在场景内部而不是浮在最上层 ──
    LyricsStage {
        anchors.fill: parent
        preset: 5
        count: 150
        time: root.animTime
        camTheta: root.camThetaView
        camPhi: root.camPhiView
        camRadius: root.camRadius
        camFocal: root.focal
        camTargetY: root.camTargetY
        audioLow: root.uLow
        audioMid: root.uMid
        audioHigh: root.uHigh
        audioAir: root.uAir
        audioLevel: root.uLevel
        color1: root.mainColor
        color2: root.secondColor
        audioGain: Lyrics3DConfig.audio
        density: Lyrics3DConfig.density
    }

    // 滑杆：一份定义，面板与宿主弹窗共用
    component OptSlider: SettingItem {
        width: parent ? parent.width : 0
        property var spec
        label: spec[1]
        QSlider {
            anchors.right: parent.right
            from: parent.spec[2]
            to: parent.spec[3]
            stepSize: parent.spec[4]
            width: 160
            height: 36
            leftText: true
            valueText: (value / parent.spec[5]).toFixed(2) + parent.spec[6]
            value: Math.round(Lyrics3DConfig[parent.spec[0]] * parent.spec[5])
            onMoved: Lyrics3DConfig[parent.spec[0]] = value / parent.spec[5]
        }
    }

    component OptColor: SettingItem {
        id: optRoot
        width: parent ? parent.width : 0
        property var spec
        label: spec[1]
        Rectangle {
            anchors.right: parent.right
            width: 120
            height: 36
            radius: 10
            color: Lyrics3DConfig[optRoot.spec[0]] !== "" ? Lyrics3DConfig[optRoot.spec[0]] : "#33ffffff"
            border.width: 1
            border.color: "#40ffffff"
            Text {
                anchors.centerIn: parent
                text: Lyrics3DConfig[optRoot.spec[0]] !== "" ? Lyrics3DConfig[optRoot.spec[0]].toUpperCase() : "跟随主题"
                color: "#ffffff"
                font.pixelSize: 11
            }
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    colorPick.key = optRoot.spec[0];
                    colorPick.openColor(Lyrics3DConfig[optRoot.spec[0]] !== "" ? Lyrics3DConfig[optRoot.spec[0]] : root.mainColor);
                }
            }
        }
    }

    SButton {
        id: diyButton
        x: root.width - 60 - 48
        y: root.height - 130 + root.hideHeight
        z: 20
        width: 36
        height: 36
        radius: 18
        iconCharacter: "\uf0a4"
        iconColor: diyPanel.visible ? "#555555" : "#fbfbfb"
        iconSize: Style.texticon
        buttonColor: diyPanel.visible ? "#88ffffff" : "#55e1e1e1"
        hoverColor: "#42000000"
        borderColor: "#66ffffff"
        borderWidth: 1
        shadowEnabled: false
        onClicked: diyPanel.visible = !diyPanel.visible
    }

    Rectangle {
        id: diyPanel
        visible: false
        z: 21
        width: 360
        height: Math.min(root.height * 0.66, 470)
        radius: 18
        color: Theme.primaryColor
        border.width: 1
        border.color: Theme.sideColor
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
                    font.family: Style.fontFamily
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
                            color: index === root.presetIndex ? root.mainColor : Theme.primaryColor
                            border.width: 1
                            border.color: index === root.presetIndex ? root.mainColor : Theme.sideColor
                            required property int index
                            required property var modelData
                            Text {
                                anchors.centerIn: parent
                                text: parent.modelData.n
                                color: Theme.textColor
                                font.pixelSize: 11
                                font.family: Style.fontFamily
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: Lyrics3DConfig.preset = parent.index
                            }
                        }
                    }
                }

                Text {
                    text: "拖动旋转 · 滚轮缩放 · 双击回正"
                    color: "#7f8ea3"
                    font.pixelSize: 11
                    font.family: Style.fontFamily
                }

                Repeater {
                    model: root.sliderTable
                    delegate: OptSlider { required property var modelData; spec: modelData }
                }
                Repeater {
                    model: root.colorTable
                    delegate: OptColor { required property var modelData; spec: modelData }
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
                delegate: OptSlider { required property var modelData; spec: modelData }
            }
        }
    }

    // [设置键, 名称, min, max, step, 显示倍数, 单位]，面板与宿主弹窗共用
    readonly property var sliderTable: [
        ["bright", "背景亮度", 20, 200, 10, 100, "x"],
        ["saturate", "色彩饱和度", 100, 220, 10, 100, "x"],
        ["audio", "音频反应", 0, 200, 10, 100, "x"],
        ["density", "点云密度", 20, 200, 10, 100, "x"],
        ["fieldScale", "方块场大小", 50, 160, 10, 100, "x"],
        ["fov", "视场角", 30, 70, 2, 1, "°"],
        ["camDist", "相机距离", 50, 140, 5, 10, "x"],
        ["camSens", "拖动灵敏度", 3, 20, 1, 10, "x"],
        ["lyricScale", "歌词大小", 60, 160, 10, 100, "x"],
        ["lyricY", "歌词上下", -100, 100, 10, 100, ""],
        ["lyricZ", "歌词景深", -100, 100, 10, 100, ""],
        ["glow", "歌词溢光", 0, 150, 10, 100, "x"],
        ["sweep", "当前行发光", 0, 100, 10, 100, "%"],
        ["beatGlow", "溢光随鼓点", 0, 100, 10, 100, "%"],
        ["reflect", "地面倒影", 0, 100, 10, 100, "%"],
        ["readab", "暗底衬", 0, 100, 10, 100, "%"],
        ["fog", "深度雾", 0, 100, 10, 100, "%"],
        ["dof", "景深虚化", 0, 100, 10, 100, "%"],
        ["floatAmt", "上下浮动", 0, 100, 10, 100, "%"]
    ]
    readonly property var colorTable: [["textColorCustom", "歌词文字颜色"], ["glowColorCustom", "歌词溢光颜色"]]

    function timerFunction(): void { }           // 宿主接口：索引由宿主计算
}