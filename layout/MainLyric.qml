// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// 歌词界面模块（默认主题）：流体背景 / 封面 / 歌曲文本 / 歌词。
// 数据由宿主经下方注入属性提供，改样式一律经 request（宿主白名单），因此可用同接口插件替换。
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Shapes
import QtQuick.Effects
import Qt5Compat.GraphicalEffects   // 仅逐字染色仍用 LinearGradient
import QueMusic 1.0
import 'qrc:/QueMusic/components'

Item {
    id: mainLyrics

    // ── 宿主注入（契约）──
    property real position: 0               // 播放进度 ms
    property real playbackRate: 1.0
    property bool playing: false
    property bool mediaActive: false        // 媒体在播放
    property var lyricsModel: []            // 歌词行 {time,text,info}
    property var translateModel: []         // 译文，与歌词行同下标
    property int currentIndex: 0            // 当前行索引（宿主定时器遍历列表后注入）
    property string title: ""
    property string artist: ""
    property string coverUrl: ""
    property color mainColor: "#00ee66"
    property color secondColor: "#00b1ee"
    property color thirdColor: "#9d4edd"
    property int hideHeight: 0              // 宿主沉浸模式偏移
    property int lyricSize: 0               // 歌词大小设置

    // 受控写入口：模块不直接改宿主状态，改样式一律经此请求（宿主按白名单处理）
    property var requestStyle: null
    function request(key: string, value: var): void {
        if (requestStyle) requestStyle(key, value);
    }

    // ── 对外状态（宿主注入，模块只渲染）──
    property bool basicCd: false             // false=封面卡片 true=经典黑胶
    property int lyricType: 0                // 0=默认 1=封面 2=歌词
    property bool openTranslate: true        // 是否显示译文
    property int lyricMove: 0                // 歌词位置校准 ms
    property bool typeChangeXAnime: false    // 主题模式位移过渡（模块内部使用）
    readonly property bool hasTranslate: translateModel.length > 0
    readonly property int standHeight: lyricSize + height / 32 + width / 56
    readonly property int piclong: width < 1280 ? height / 3 + width / 8 - 100 : height / 3 + 60

    // 主题模式变化时播放位移动画；模块创建时的初始赋值不触发
    property bool uiReady: false
    Component.onCompleted: uiReady = true
    onLyricTypeChanged: if (uiReady) typeChangeXAnime = true

    // 宿主每 320ms 调用一次：按注入的索引刷新列表位置与等待动画
    function timerFunction(): void { lyricContent.tick(mainLyrics.currentIndex) }

    // 频谱波形（作为模糊源，不直接显示）
    Shape {
        id: waveItem
        width: 512
        height: 80
        visible: false
        asynchronous: true
        vendorExtensionsEnabled: true

        ShapePath {
            id: wavePath
            fillColor: Qt.hsva(mainLyrics.mainColor.hsvHue,mainLyrics.mainColor.hsvSaturation,mainLyrics.mainColor.hsvValue * 0.5 + 0.5,0.7)
            strokeWidth: 0
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin
            startX: 0
            startY: waveItem.height
            PathPolyline {
                path: getWave.wavePath
            }
        }
    }

    MultiEffect {
        x: 0
        y: mainLyrics.height - 158 + mainLyrics.hideHeight
        width: mainLyrics.width
        height: 80
        z: 9
        source: waveItem
        blurEnabled: true
        blurMax: 32
        blur: 1.0
        visible: Style.settings.waveDisplay
    }

    MeshGradientItem {
        anchors.fill: parent
        coverUrl: mainLyrics.coverUrl || "qrc:/QueMusic/resources/app/musicpic.png"
        color1: mainLyrics.mainColor
        color2: mainLyrics.secondColor
        color3: mainLyrics.thirdColor
        algorithm: Style.settings.flowStyle
        animating: true
        clip: true
    }

    // 静态渐变背景（关闭流动时）
    Rectangle {
        anchors.fill: parent
        visible: Style.settings.flowStyle === 2
        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: mainLyrics.mainColor
            }
            GradientStop {
                position: 1.0
                color: mainLyrics.secondColor
            }
        }
    }

    Item {
        id: musicpic
        z: 5
        width: mainLyrics.piclong
        height: mainLyrics.piclong
        clip: false
        visible: !mainLyrics.basicCd
        x: mainLyrics.lyricType == 0 ? mainLyrics.width * 0.23 - mainLyrics.piclong * 0.5 : mainLyrics.lyricType == 1 ? mainLyrics.width * 0.5 - mainLyrics.piclong * 0.5 : -100 - mainLyrics.piclong
        y: mainLyrics.height * 0.6 - mainLyrics.piclong
        scale: mainLyrics.playing ? 1.0 : 0.84
        Behavior on scale { NumberAnimation { duration: 320; easing.type: Easing.BezierSpline; easing.bezierCurve: [ 0.20, 0.04, 0.00, 1.64, 1, 1 ] } }
        Behavior on x { enabled: mainLyrics.typeChangeXAnime; NumberAnimation { duration: 360; easing.type: Easing.BezierSpline; easing.bezierCurve: [ 0.30, 0.08, 0.00, 1.00, 1, 1 ]; onFinished: mainLyrics.typeChangeXAnime = false } }
        RectangularShadow {
            id: musicpicShadow
            anchors.fill: musicpic
            z: 0
            offset.x: 2
            offset.y: 12
            radius: 24
            blur: 32
            visible: true
            color: "#55000000"
        }
        Image {
            id: sourcepic
            anchors.fill: musicpic
            fillMode: Image.PreserveAspectCrop
            visible: false
            source: mainLyrics.coverUrl || "qrc:/QueMusic/resources/app/musicpic.png"
            sourceSize: Qt.size(512, 512)
        }
        Rectangle {
            id: maskpic
            anchors.fill: musicpic
            color: "#ff000000"
            radius: 24
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
    }

    Text {
        id: titleMax
        y: mainLyrics.height * 0.6 + 20
        x: musicpic.x
        height: mainLyrics.standHeight
        text: mainLyrics.title
        font.weight: 600
        width: mainLyrics.piclong
        elide: Text.ElideRight
        visible: x !== -400 && !mainLyrics.basicCd
        font.pixelSize: mainLyrics.standHeight / 2
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: mainLyrics.lyricType == 1 ? Text.AlignHCenter : Text.AlignLeft

        color: Qt.rgba(1,1,1,1)
        layer.enabled: true
        layer.effect: DropShadow {
            horizontalOffset: 2
            verticalOffset: 2
            radius: 12.0
            samples: 16
            fast: true
            color: "#32000000"
            source: titleMax // 阴影绑定到主内容区域
        }
    }
    Text {
        id: artistMax
        anchors.top: titleMax.bottom
        x: musicpic.x
        height: mainLyrics.standHeight / 3
        text: mainLyrics.artist
        width: mainLyrics.piclong
        elide: Text.ElideRight
        horizontalAlignment: mainLyrics.lyricType == 1 ? Text.AlignHCenter : Text.AlignLeft
        font.bold: false
        font.pixelSize: mainLyrics.standHeight / 3.6
        verticalAlignment: Text.AlignVCenter
        visible: x !== -400 && !mainLyrics.basicCd
        color: Qt.rgba(1,1,1,0.7)
    }

    Loader {
        active: mainLyrics.basicCd
        visible: mainLyrics.basicCd
        sourceComponent: CdAlbum {
            width: mainLyrics.piclong
            height: mainLyrics.piclong
            y: (mainLyrics.height - height) * 0.5
            x: musicpic.x
            rotation: mainLyrics.playing
            source: mainLyrics.coverUrl || "qrc:/QueMusic/resources/app/musicpic.png"
        }
    }

    // 歌词内容。定位与逐字进度在此计算（320ms 定时器），模块随宿主不可见而整体卸载。
    Item {
        id: lyricContent
        x: mainLyrics.lyricType == 0 ? mainLyrics.width * 0.46 : mainLyrics.lyricType == 1 ? mainLyrics.width : mainLyrics.width * 0.2
        y: 60
        Behavior on x { enabled: mainLyrics.typeChangeXAnime; NumberAnimation { duration: 360; easing.type: Easing.BezierSpline; easing.bezierCurve: [ 0.30, 0.08, 0.00, 1.00, 1, 1 ] } }
        width: mainLyrics.lyricType == 2 ? mainLyrics.width * 0.6 : mainLyrics.width * 0.48
        height: parent.height - 120
        visible: mainLyrics.lyricType !== 1
        layer.enabled: true
        layer.effect: ShaderEffect {
            property real fadeTop: 0.15
            property real fadeBottom: 0.3
            property real blurTop: 0.3
            property real blurBottom: 0.5
            property real blurRadius: Style.settings.maskBlur ? 8 : 0
            property vector2d srcSize: Qt.vector2d(lyricContent.width * Screen.devicePixelRatio, lyricContent.height * Screen.devicePixelRatio)
            fragmentShader: "qrc:/shaders/shaders/lyricfade.frag.qsb"
        }

        property int currentPlayTime: mainLyrics.position + mainLyrics.lyricMove
        readonly property int lyricHeight: mainLyrics.standHeight / 2
        property real alignPos: 0.32        // 当前行停在视口高度比例
        property real lineSpacing: mainLyrics.standHeight / 1.6
        property int currentLine: 0
        property real springValue: 0.0
        property bool isUserScrolling: false// 滚轮
        property int scrollOffset: 0       // 用户手动滚动的额外偏移量
        property real fixedH: 0
        property real finalH: 0

        property list<int> heights: [] //高度缓存
        property list<int> prefixSum: [] //y缓存

        // 固定融合动画
        SequentialAnimation {
            id: fixedAnime
            property int to: 0
            NumberAnimation {
                target: lyricContent
                property: "fixedH"
                duration: 460
                to: fixedAnime.to
                easing.type: Easing.BezierSpline
                easing.bezierCurve: [ 0.24, 0.06, lyricContent.springValue, 1.03, 1, 1 ]
            }
            NumberAnimation {
                target: lyricContent
                property: "finalH"
                duration: 460
                to: fixedAnime.to
                easing.type: Easing.BezierSpline
                easing.bezierCurve: [ 0.24, 0.06, lyricContent.springValue, 1.03, 1, 1 ]
            }
        }

        // 换源/换歌：重排歌词（放在子项内，避免创建期属性初始化提前触发）
        Connections {
            target: mainLyrics
            function onLyricsModelChanged(): void {
                lyricContent.heights = [];
                lyricContent.prefixSum = [];
                lyricContent.currentLine = 0;
                lyricContent.scrollOffset = 0;
                Qt.callLater(lyricContent.rebuild);
            }
        }

        // 宿主每 320ms 调用（见 PlayerMaxCenter 的 lyricTimer），idx 由宿主遍历列表得出
        function tick(idx: int): void {
            const data = mainLyrics.lyricsModel;
            if (!data || data.length === 0 || idx < 0 || idx >= lyricRep.count) return;
            // 用户手动滚动期间只同步索引，不打断浏览
            if (lyricContent.isUserScrolling) {
                lyricContent.currentLine = idx;
                return;
            }
            if (lyricContent.scrollOffset !== 0) {
                scrollAnime.running = false;
                scrollAnime.to = 0;
                scrollAnime.running = true;
            }
            if (idx !== lyricContent.currentLine) {
                if (waitAnimeSection.visible) {
                    waitOpenAnime.running = false;
                    waitOutAnime.running = true;
                }
                const animeHeight = lyricContent.prefixSum[idx] - lyricContent.prefixSum[lyricContent.currentLine];
                lyricContent.springValue = Math.max(animeHeight - 400 > 0 ? Math.floor((animeHeight - 400) / 20) / -200 : 0, -0.50);
                lyricContent.currentLine = idx;
                fixedAnime.running = false;
                fixedAnime.to = -animeHeight;
                const baseY = lyricContent.prefixSum[lyricContent.currentLine];
                const alignY = lyricContent.height * lyricContent.alignPos;
                for (let i = 0; i < lyricRep.count; i++) {
                    const it = lyricRep.itemAt(i);
                    if (it) it.animeTo(Math.floor(lyricContent.prefixSum[i] - baseY + alignY));
                }
                lyricContent.fixedH = 0;
                lyricContent.finalH = 0;
                fixedAnime.running = true;
            }
            checkWaitAnime(idx);
        }

        // 行间隔过长（>2.5s）时插入等待动画
        function checkWaitAnime(idx: int): void {
            const data = mainLyrics.lyricsModel;
            const line = data[idx];
            const info = line ? line.info : undefined;
            const next = data[idx + 1];
            if (!info || info.length === 0 || !next) return;
            const last = info[info.length - 1];
            if (!last || last.offset === undefined || last.duration === undefined
                || next.time === undefined || line.time === undefined) return;
            if (next.time - line.time - last.offset - last.duration <= 2500
                || mainLyrics.position <= line.time + last.offset + last.duration
                || waitAnimeSection.visible) return;
            waitOpenAnime.running = false;
            waitOutAnime.running = false;
            waitOpenAnime.running = true;
            const baseY = lyricContent.prefixSum[lyricContent.currentLine + 1];
            const alignY = lyricContent.height * lyricContent.alignPos;
            for (let i = 0; i <= idx; i++) {
                const it = lyricRep.itemAt(i);
                if (it) it.animeTo(Math.floor(lyricContent.prefixSum[i] - baseY + alignY));
            }
            for (let i = idx + 1; i < lyricRep.count; i++) {
                const it = lyricRep.itemAt(i);
                if (it) it.animeTo(Math.floor(lyricContent.prefixSum[i] - baseY + alignY + mainLyrics.standHeight));
            }
        }

        function rebuild(): void {
            let sum = 0
            const arr = [];
            for (let i = 0; i < lyricRep.count; i++) {
                arr.push(sum);
                const it = lyricRep.itemAt(i);
                const h = it ? it.height : 0;
                heights[i] = h;
                sum += h; // 累加下一行起点
            }
            prefixSum = arr;
            for (let i = 0; i < lyricRep.count; i++) {
                const basicIndexY = prefixSum[currentLine];
                if (lyricRep.itemAt(i)) lyricRep.itemAt(i).standY = Math.floor(prefixSum[i] - basicIndexY + lyricContent.height * lyricContent.alignPos);
            }
        }

        Component.onCompleted: Qt.callLater(rebuild)

        Repeater {
            id: lyricRep
            model: mainLyrics.lyricsModel ? mainLyrics.lyricsModel : [{time: 0, text: "纯音乐，请欣赏"}]

            delegate: Item {
                id: lyricItem
                x: 10
                width: lyricContent.width - 20
                height: lyricsText.implicitHeight + lyricTransText.height + lyricContent.lineSpacing

                readonly property bool isCurrent: index === lyricContent.currentLine
                readonly property bool isFlowActive: modelData.info ? (index == lyricContent.currentLine || index == lyricContent.currentLine - 1) : false
                readonly property int nowPosition: isFlowActive ? lyricContent.currentPlayTime - modelData.time : 0
                property real opacityAnime: isCurrent && !waitAnimeSection.visible ? 1.0 : 0.0
                Behavior on opacityAnime { NumberAnimation { duration: 320 } }
                property real standY: 0.0
                y: standY + lyricContent.scrollOffset

                SequentialAnimation {
                    id: lyricAnime
                    property int pauseMs: 0
                    property int animeMs: 460
                    property int toY
                    PauseAnimation { duration: lyricAnime.pauseMs }
                    NumberAnimation {
                        target: lyricItem
                        property: "standY"
                        duration: lyricAnime.animeMs
                        to: lyricAnime.toY
                        easing.type: Easing.BezierSpline
                        easing.bezierCurve: [ 0.24, 0.06, lyricContent.springValue, 1.03, 1, 1 ]
                    }
                }

                function animeTo(ty: real): void {
                    lyricAnime.running = false;
                    const d = index - lyricContent.currentLine;
                    let durationMs;
                    lyricItem.standY = lyricItem.standY;
                    // 不使用弹簧动画的外部区域（非index>-3至7)，使用融合动画：绑定至统一的动画值，提升性能喵~
                    if(d < -3) {
                        // 检测防止动画乱跑，检测稳定即融合
                        if(ty - lyricItem.standY - fixedAnime.to == 0) {
                            const nowY = lyricItem.standY;
                            lyricItem.standY = Qt.binding(function(): real { return (lyricContent.fixedH + nowY) });
                            return;
                        } else {
                            durationMs = 0;
                        }
                    } else if(d > 7) {
                        if(ty - lyricItem.standY - fixedAnime.to == 0) {
                            const nowY = lyricItem.standY;
                            lyricItem.standY = Qt.binding(function(): real { return (lyricContent.finalH + nowY) });
                            return;
                        } else {
                            durationMs = 460;
                        }
                    } else {
                        durationMs = (d + 4) ** 1.2 * 24;
                    }
                    lyricAnime.toY = ty;
                    lyricAnime.pauseMs = durationMs;
                    lyricAnime.animeMs = d < -3 ? 460 : 460 + (d + 4) * 32;
                    lyricAnime.running = true;
                }

                onHeightChanged: {
                    lyricContent.heights[index] = height;
                    Qt.callLater(lyricContent.rebuild);
                }
                Component.onCompleted: {
                    lyricContent.heights[index] = height;
                    Qt.callLater(lyricContent.rebuild);
                    if(Style.settings.fontFamily) {
                        lyricsText.font.family = Style.settings.fontFamily;
                        lyricTransText.font.family = Style.settings.fontFamily;

                    }
                }

                Text {
                    z: 0
                    id: lyricsText
                    width: lyricItem.width - lyricContent.lyricHeight / 4
                    text: modelData.text || ""
                    font.weight: Style.settings.textWidth
                    font.pixelSize: lyricContent.lyricHeight
                    color: modelData.info ? Qt.rgba(0.91,0.91,0.91,1.0) : Qt.rgba(0.91 + lyricItem.opacityAnime * 0.09,0.91 + lyricItem.opacityAnime * 0.09,0.91 + lyricItem.opacityAnime * 0.09,1.0)
                    transformOrigin: modelData.isOther ? Item.BottomRight : Item.BottomLeft
                    wrapMode: Text.Wrap
                    scale: lyricItem.isCurrent && !modelData.info ? 1.02 : 1.00
                    opacity: modelData.info ? 0.5 : (0.5 + lyricItem.opacityAnime * 0.4)
                    visible: modelData.info ? !lyricItem.isFlowActive : true
                    horizontalAlignment: modelData.isOther ? Text.AlignRight : Text.AlignLeft
                    Behavior on scale { NumberAnimation { duration: 640; easing.type: Easing.InOutCubic } }
                }

                Text {
                    id: lyricTransText
                    anchors.top: lyricsText.bottom
                    transformOrigin: modelData.isOther ? Item.TopRight : Item.TopLeft
                    scale: lyricItem.isCurrent && !waitAnimeSection.visible ? 1.02 : 1.00
                    visible: text !== ""
                    height: visible ? implicitHeight * 1.5 : 0
                    text: mainLyrics.translateModel.length !== 0 && mainLyrics.openTranslate ? (mainLyrics.translateModel[index] || "") : ""
                    width: parent.width
                    horizontalAlignment: modelData.isOther ? Text.AlignRight : Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    font.weight: Style.settings.textWidth
                    color: "#ffe8e8e8"
                    opacity: 0.5 + lyricItem.opacityAnime * 0.2
                    Behavior on scale { NumberAnimation { duration: 640; easing.type: Easing.InOutCubic } }
                    font.pixelSize: lyricContent.lyricHeight / 1.5
                }

                CustomFlow {
                    id: lyricFlow
                    width: lyricItem.width
                    height: lyricItem.height
                    alignment: modelData.isOther ? CustomFlow.AlignRight : CustomFlow.AlignLeft

                    transformOrigin: modelData.isOther ? Item.BottomRight : Item.BottomLeft
                    scale: lyricItem.isCurrent && !waitAnimeSection.visible ? 1.02 : 1.00
                    x: 0
                    visible: lyricItem.isFlowActive
                    z: 1
                    Behavior on scale { NumberAnimation { duration: 640; easing.type: Easing.InOutCubic } }
                    Repeater {
                        id: linesText
                        model: lyricItem.isFlowActive ? (modelData.info || 0) : 0
                        delegate: Item {
                            width: lyricFlowText.width
                            height: lyricFlowText.height
                            readonly property bool toTextAnimeValue: lyricItem.nowPosition > linesText.model[index].offset && lyricItem.isCurrent
                            onToTextAnimeValueChanged: {
                                if(toTextAnimeValue) {
                                    outFlowText.running = false;
                                    toFlowText.running = true;
                                } else {
                                    toFlowText.running = false;
                                    outFlowText.running = true;
                                }
                            }

                            ParallelAnimation {
                                id: toFlowText
                                NumberAnimation { target: lyricFlowText; property: "y"; to: -3; duration: 240 + linesText.model[index].duration * 10; easing.type: Easing.OutExpo }
                            }
                            ParallelAnimation {
                                id: outFlowText
                                NumberAnimation { target: lyricFlowText; property: "y"; to: 0; duration: 640; easing.type: Easing.InOutCubic }
                            }

                            Text {
                                id: lyricFlowText
                                text: linesText.model[index].text
                                y: 0//lyricItem.nowPosition > linesText.model[index].offset && lyricItem.isCurrent ? -3 : 0
                                font.weight: Style.settings.textWidth
                                font.pixelSize: lyricContent.lyricHeight
                                font.family: lyricsText.font.family
                                color: "#ffe8e8e8"
                                opacity: 0.5
                            }
                            LinearGradient {
                                property int countToWidth: lyricItem.nowPosition > linesText.model[index].offset && lyricItem.isFlowActive ? width + 16 : 0
                                Behavior on countToWidth { NumberAnimation { Component.onCompleted: duration = linesText.model[index].duration / mainLyrics.playbackRate * (width + 16) / width } }
                                width: parent.width
                                height: parent.height
                                y: lyricFlowText.y
                                opacity: lyricItem.opacityAnime
                                source: lyricFlowText
                                start: Qt.point(countToWidth - 16, 0)
                                end: Qt.point(countToWidth, 0)
                                gradient: Gradient {
                                    GradientStop { position: 0.0; color: "#ffffffff" }
                                    GradientStop { position: 1.0; color: "#66e8e8e8" }
                                }
                            }
                        }
                    }
                }
            }
        }

        Item {
            id: waitAnimeSection
            scale: 0.0
            transformOrigin: Popup.BottomLeft
            y: lyricContent.height * lyricContent.alignPos + lyricContent.lyricHeight * 0.2 + lyricContent.scrollOffset
            x: 10
            visible: false
            width: lyricContent.lyricHeight * 2
            height: lyricContent.lyricHeight * 0.5
            property int lightState: 0
            Rectangle {
                width: waitAnimeSection.height
                height: waitAnimeSection.height
                radius: waitAnimeSection.height / 2
                color: waitAnimeSection.lightState > 0 ? "#ffffffff" : "#99ffffff"
                Behavior on color { ColorAnimation { duration: 320; easing.type: Easing.OutCubic } }
            }
            Rectangle {
                x: waitAnimeSection.height * 1.5
                width: waitAnimeSection.height
                height: waitAnimeSection.height
                radius: waitAnimeSection.height / 2
                color: waitAnimeSection.lightState > 1 ? "#ffffffff" : "#99ffffff"
                Behavior on color { ColorAnimation { duration: 320; easing.type: Easing.OutCubic } }
            }
            Rectangle {
                x: waitAnimeSection.height * 3
                width: waitAnimeSection.height
                height: waitAnimeSection.height
                radius: waitAnimeSection.height / 2
                color: waitAnimeSection.lightState > 2 ? "#ffffffff" : "#99ffffff"
                Behavior on color { ColorAnimation { duration: 320; easing.type: Easing.OutCubic } }
            }
        }

        SequentialAnimation {
            id: waitOpenAnime
            property int lightDuration: 2500
            onStarted: {
                waitAnimeSection.visible = true;
                waitAnimeSection.lightState = 0;
                const line = mainLyrics.lyricsModel[lyricContent.currentLine];
                const next = mainLyrics.lyricsModel[lyricContent.currentLine + 1];
                if (line && next && line.info && line.info.length > 0
                    && (next.time !== undefined) && (line.time !== undefined)) {
                    const last = line.info[line.info.length - 1];
                    if (last && (last.offset !== undefined) && (last.duration !== undefined))
                        waitOpenAnime.lightDuration = next.time - line.time - last.offset - last.duration - 420;
                    else
                        waitOpenAnime.lightDuration = 2500;
                } else {
                    waitOpenAnime.lightDuration = 2500;
                }
            }
            PauseAnimation { duration: 100 }
            ParallelAnimation {
                NumberAnimation { target: waitAnimeSection; property: "opacity"; from: 0; to: 1; duration: 460; easing.type: Easing.OutCubic }
                NumberAnimation { target: waitAnimeSection; property: "scale"; from: 0; to: 1; duration: 460; easing.type: Easing.OutCubic }
            }
            NumberAnimation { target: waitAnimeSection; property: "lightState"; from: 0; to: 3; duration: waitOpenAnime.lightDuration / mainLyrics.playbackRate }
        }
        ParallelAnimation {
            id: waitOutAnime
            NumberAnimation { target: waitAnimeSection; property: "opacity"; from: 1; to: 0; duration: 320 }
            NumberAnimation { target: waitAnimeSection; property: "scale"; from: 1; to: 0; duration: 320 }
            onFinished: waitAnimeSection.visible = false
        }

        WheelHandler {
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            onWheel: (event) => {
                lyricContent.isUserScrolling = true;
                scrollAnime.running = false;
                const to = Math.max(Math.min(scrollAnime.to + event.angleDelta.y * 0.25 * Qt.application.styleHints.wheelScrollLines,lyricContent.prefixSum[lyricContent.currentLine]),lyricContent.prefixSum[lyricContent.currentLine] - lyricContent.prefixSum[lyricRep.count - 1]);
                scrollAnime.to = to;
                scrollAnime.running = true;
                event.accepted = true;
            }
        }

        NumberAnimation {
            id: scrollAnime
            target: lyricContent
            property: "scrollOffset"
            duration: 640
            easing.type: Easing.OutExpo
            onFinished: lyricContent.isUserScrolling = false
        }
    }

    // ── 提供给宿主的“播放器样式”弹窗内容 ──
    // 宿主把它插在常驻项之后；换掉本文件即换掉这部分。状态改动一律经 requestStyle 请求宿主
    property Component styleOptions: Component {
        Column {
            width: parent ? parent.width : 0
            spacing: 16
            SettingItem {
                label: "封面模式"
                isBigItem: true
                width: parent.width
                height: 156
                QBigDrop {
                    x: 0
                    y: 36
                    width: parent.width
                    picModel: ["qrc:/QueMusic/resources/app/musicpic.png","qrc:/QueMusic/resources/app/cd.png"]
                    model: ["封面卡片","经典黑胶"]
                    choice: mainLyrics.basicCd ? 1 : 0
                    onTransformed: (choiced) => {
                        mainLyrics.request("basicCd", choiced === 1);
                    }
                }
            }
            SettingItem {
                label: "设置主题模式"
                isBigItem: true
                width: parent.width
                QWideDrop {
                    x: 0
                    y: 36
                    width: parent.width
                    model: ["默认","封面","歌词"]
                    choice: mainLyrics.lyricType
                    onTransformed: (choiced) => {
                        mainLyrics.request("lyricType", choiced);
                    }
                }
            }
            SettingItem {
                label: "高级逐行弹簧动画"
                width: parent.width
                QSwitch {
                    height: 36; width: 120
                    anchors.right: parent.right
                    switchTrue: Style.settings.premiumLyricAnime
                    onToggled: mainLyrics.request("premiumLyricAnime", !Style.settings.premiumLyricAnime)
                }
            }
            SettingItem {
                label: "显示音波效果"
                width: parent.width
                QSwitch {
                    height: 36; width: 120
                    anchors.right: parent.right
                    switchTrue: Style.settings.waveDisplay
                    onToggled: mainLyrics.request("waveDisplay", !Style.settings.waveDisplay)
                }
            }
        }
    }
}
