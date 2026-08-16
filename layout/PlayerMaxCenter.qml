// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Shapes
import Qt5Compat.GraphicalEffects
import QueMusic 1.0
// GetWave 已注册到 QueMusic 模块；MeshGradientItem 为独立模块（C++ QML_ELEMENT）
import MeshGradientItem 1.0
import 'qrc:/QueMusic/components'

Item {
    id: musicControlMax
    //color: "black"
    //layer.enabled: true
    readonly property int standHeight: Style.settings.lyricSize + mainLayout.height / 32 + mainLayout.width / 56
    readonly property int infoWidth: lyricModeText.width / 2
    property bool basicCd: false
    property color mainColor: "#00ee66"
    property color secondColor: "#00b1ee"
    property color thirdColor: "#9d4edd"
    Connections {
        target: colorExtractor
        function onColorExtractFinished() {
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

    Shape {
        id: waveItem
        width: 512
        height: 80
        //z: 9
        visible: false
        asynchronous: true
        vendorExtensionsEnabled: true
        layer.enabled: true
        layer.smooth: true

        ShapePath {
            id: wavePath
            fillColor: Qt.hsva(musicControlMax.mainColor.hsvHue,musicControlMax.mainColor.hsvSaturation,musicControlMax.mainColor.hsvValue * 0.7 + 0.3,0.7)
            //strokeColor: "#00ccff"
            //strokeWidth: 2
            strokeWidth: 0
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin
            // 起点：左下角
            startX: 0
            startY: waveItem.height
            PathPolyline {
                path: getWave.wavePath
            }
        }
    }
    FastBlur {
        x: 0
        y: musicControlMax.height - 158 + controlMaxLoader.hideHeight
        width: musicControlMax.width
        height: 80
        z: 9
        source: waveItem
        radius: 32
        //transparentBorder: true
        visible: Style.settings.waveDisplay
    }

    // 动态背景：AMLL Mesh Gradient 移植（Bicubic Hermite Patch Mesh）
    MeshGradientItem {
        id: bgMesh
        clip: true
        anchors.fill: parent
        visible: Style.settings.backFlowQuality !== 2
        coverUrl: colorExtractor.renderUrl || mainMedia.urlStr || "qrc:/QueMusic/resources/app/musicpic.png"
        volume: 0
        flowSpeed: 1.0
        animating: true
        subDivisions: 42
        // 网格渐变主色：跟随封面的主色调（AMLL 流体感的来源）
        color1: musicControlMax.mainColor
        color2: musicControlMax.secondColor
        color3: musicControlMax.thirdColor
    }

    LinearGradient {
        anchors.fill: parent
        visible: Style.settings.backFlowQuality === 2
        cached: true
        end: Qt.point(height / 3,height)
        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: musicControlMax.mainColor
            }
            GradientStop {
                position: 1.0
                color: musicControlMax.secondColor
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        propagateComposedEvents: false  // 阻止事件穿透
        // 阻止所有鼠标事件穿透
        onPressed: function(mouse) { mouse.accepted = true }
        onReleased: function(mouse) { mouse.accepted = true }
        onDoubleClicked: function(mouse) { mouse.accepted = true }
        onWheel: function(wheel) { wheel.accepted = true }
        onClicked: function(mouse) { mouse.accepted = true }
        onPositionChanged: {
            if(Style.settings.lyricHideGui) {
                controlMaxLoader.hideHeight = 0;
                hideDelay.running = false;
                hideDelay.running = true;
            }
        }
    }

    Timer {
        id: hideDelay
        interval: 3000
        running: true
        onTriggered: {
            if(Style.settings.lyricHideGui) {
                controlMaxLoader.hideHeight = 76;
            } else {
                controlMaxLoader.hideHeight = 0;
            }
        }
    }

    SButton {
        id: playerminedButton
        x: 20
        y: 10 - controlMaxLoader.hideHeight
        iconCharacter: "\uf096" // playermin icon
        width: 40
        height: 40
        radius: 10
        //visible: false
        buttonColor: "transparent"
        hoverColor: Qt.rgba(0,0,0,0.2)
        iconColor: "#eeeeee"
        iconSize: Style.settings.texticonH
        shadowEnabled: false
        onClicked: {
            window.playermined();
            minedAnimation.start();
            mainLayout.state = "";
        }
    }
    SButton {
        id: centerStyleButton
        x: 70
        y: 10 - controlMaxLoader.hideHeight
        iconCharacter: "\uf116" // playermin icon
        width: 40
        height: 40
        radius: 10
        //visible: false
        buttonColor: "transparent"
        iconColor: "#eeeeee"
        hoverColor: Qt.rgba(0,0,0,0.2)
        iconSize: Style.settings.texticon
        shadowEnabled: false
        onClicked: {
            maxLyricsDialog.open();
        }
    }
    SButton {
        x: musicControlMax.width - 60
        y: musicControlMax.height - 130 + controlMaxLoader.hideHeight
        z: 5
        iconCharacter: "\uf079"
        visible: MusicApi.lyricsTranslate.length !== 0
        width: 36
        height: 36
        radius: 18
        //visible: false
        buttonColor: lyricContent.openTranslate ? "#88ffffff" : "#55e1e1e1"
        hoverColor: "#42000000"
        iconColor: lyricContent.openTranslate ? "#555555" : "#fbfbfb"
        borderColor: "#66ffffff"
        borderWidth: 1
        iconSize: Style.settings.texticon
        shadowEnabled: false
        onClicked: {
            lyricContent.openTranslate = !lyricContent.openTranslate;
        }
        QTip {
            visible: parent.hovered
            text: "翻译"
        }
    }

    Text {
        id: titleMax
        y: mainLayout.height / 1.7 + 20
        x: controlMaxLoader.infoX
        height: musicControlMax.standHeight
        text: window.musicTitle
        font.weight: 600
        width: mainLayout.piclong
        elide: Text.ElideRight
        visible: x !== -400 && !controlMaxLoader.basicCd
        font.pixelSize: musicControlMax.standHeight / 2
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: controlMaxLoader.lyricsType === 1 ? Text.AlignHCenter : Text.AlignLeft

        color: Qt.rgba(1,1,1,1)
        //Behavior on opacity { NumberAnimation { duration: 300 } }
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
        x: titleMax.x
        height: musicControlMax.standHeight / 3
        text: window.musicArtist
        width: mainLayout.piclong
        elide: Text.ElideRight
        horizontalAlignment: controlMaxLoader.lyricsType === 1 ? Text.AlignHCenter : Text.AlignLeft
        font.bold: false
        font.pixelSize: musicControlMax.standHeight / 3.6
        verticalAlignment: Text.AlignVCenter
        visible: x !== -400 && !controlMaxLoader.basicCd
        color: Qt.rgba(1,1,1,0.7)
        //Behavior on opacity { NumberAnimation { duration: 300 } }
    }
    Text {
        id: lyricModeText
        x: parent.width / 2 - width / 2
        y: 80
        height: 50
        width: implicitWidth > parent.width / 3 - 120 ? parent.width / 3 - 120 : implicitWidth
        text: window.musicTitle + "    --" + window.musicArtist
        elide: Text.ElideRight
        font.weight: 600
        font.pixelSize: 16
        verticalAlignment: Text.AlignVCenter
        //horizontalAlignment: Text.AlignRight
        color: Qt.rgba(1,1,1,0.8)
        visible: controlMaxLoader.lyricsType === 2
        opacity: 0.8
    }

    Loader {
        active: controlMaxLoader.basicCd
        visible: controlMaxLoader.basicCd
        sourceComponent: CdAlbum {
            width: mainLayout.piclong
            height: mainLayout.piclong
            y: (musicControlMax.height - height) * 0.5
            x: controlMaxLoader.infoX
            rotation: mainMedia.playing
            source: mainMedia.urlStr || "qrc:/QueMusic/resources/app/musicpic.png"
        }
    }


    // 歌词部分，之后可能会将部分算法移到c++处理。
    Item {
        id: lyricContent
        x: controlMaxLoader.lyricsX
        y: 60
        width: controlMaxLoader.lyricsType === 2 ? musicControlMax.width - 96 : musicControlMax.width * 0.5
        height: parent.height - 120
        visible: controlMaxLoader.lyricsType !== 1
        layer.enabled: true
        layer.effect: ShaderEffect {
            property real fadeTop: 0.15
            property real fadeBottom: 0.3
            property real blurTop: 0.3
            property real blurBottom: 0.5
            property real blurRadius: Style.settings.maskBlur ? 8 : 0
            property vector2d srcSize: Qt.vector2d(lyricContent.width, lyricContent.height)
            fragmentShader: "qrc:/shaders/resources/app/shaders/lyricfade.frag.qsb"
        }

        property int currentPlayTime: 0

        Timer {
            interval: 100
            running: mainMedia.onMedia
            repeat: true
            onTriggered: {
                lyricContent.currentPlayTime = mainMedia.position;
            }
        }
        readonly property int lyricHeight: musicControlMax.standHeight / 2
        property real alignPos: 0.32        // 当前行停在视口高度比例
        property real lineSpacing: musicControlMax.standHeight / 1.6  // 行间距
        //readonly property int overscan: 8
        property int currentLine: 0         // currentIndex
        property int springValue: 0.0
        property bool openTranslate: true   // 是否显示翻译
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
                easing.bezierCurve: [ 0.24, 0.06, lyricContent.springValue, 1.04, 1, 1 ]
            }
            NumberAnimation {
                target: lyricContent
                property: "finalH"
                duration: 460
                to: fixedAnime.to
                easing.type: Easing.BezierSpline
                easing.bezierCurve: [ 0.24, 0.06, lyricContent.springValue, 1.04, 1, 1 ]
            }
        }




        Timer {
            interval: 320
            running: mainMedia.onMedia
            repeat: true
            onTriggered: {
                var data = MusicApi.lyricsData;
                if (!data || data.length === 0) return;
                var pos = mainMedia.position + 320;
                var idx = lyricContent.currentLine;
                while (idx + 1 < data.length && pos >= data[idx + 1].time) idx++;
                while (idx > 0 && pos < data[idx].time) idx--;
                if (lyricContent.isUserScrolling) {
                    lyricContent.currentLine = idx;
                    return;
                }
                if (lyricContent.scrollOffset !== 0) {
                    scrollAnime.running = false;
                    scrollAnime.to = 0;
                    scrollAnime.running = true;
                }
                if (idx !== lyricContent.currentLine)  {
                    if(waitAnimeSection.visible) {
                        waitOpenAnime.running = false;
                        waitOutAnime.running = true;
                    }
                    var animeHeight = lyricContent.prefixSum[idx] - lyricContent.prefixSum[lyricContent.currentLine];
                    var springValue = animeHeight - 400 > 0 ? Math.floor((animeHeight - 400) / 20) / -200 : 0.00;
                        if(springValue < -0.50) {
                        lyricContent.springValue = -0.50;
                    } else {
                        lyricContent.springValue = springValue;
                    }
                    lyricContent.currentLine = idx;
                    fixedAnime.running = false;
                    fixedAnime.to = -animeHeight;
                    for (var i = 0; i < lyricRep.count; i++) {
                        var basicIndexY = lyricContent.prefixSum[lyricContent.currentLine];
                        if (lyricRep.itemAt(i)) lyricRep.itemAt(i).animeTo(Math.floor(lyricContent.prefixSum[i] - basicIndexY + lyricContent.height * lyricContent.alignPos), false);
                    }
                    lyricContent.fixedH = 0;
                    lyricContent.finalH = 0;
                    fixedAnime.running = true;
                }
                if(MusicApi.lyricsData[idx].info) {
                    var lyricLastLineData = MusicApi.lyricsData[idx].info[MusicApi.lyricsData[idx].info.length - 1];
                    if(MusicApi.lyricsData[idx + 1].time - MusicApi.lyricsData[idx].time - lyricLastLineData.offset - lyricLastLineData.duration > 2500 && mainMedia.position > MusicApi.lyricsData[idx].time + lyricLastLineData.offset + lyricLastLineData.duration) {
                        if(!waitAnimeSection.visible) {
                            console.log("开始运行等待动画。");
                            waitOpenAnime.running = false;
                            waitOutAnime.running = false;
                            waitOpenAnime.running = true;
                            for (var i = 0; i <= idx; i++) {
                                var basicIndexY = lyricContent.prefixSum[lyricContent.currentLine + 1];
                                if (lyricRep.itemAt(i)) lyricRep.itemAt(i).animeTo(Math.floor(lyricContent.prefixSum[i] - basicIndexY + lyricContent.height * lyricContent.alignPos), false);
                            }
                            for (var i = idx + 1; i < lyricRep.count; i++) {
                                var basicIndexY = lyricContent.prefixSum[lyricContent.currentLine + 1];
                                if (lyricRep.itemAt(i)) lyricRep.itemAt(i).animeTo(Math.floor(lyricContent.prefixSum[i] - basicIndexY + lyricContent.height * lyricContent.alignPos + musicControlMax.standHeight), false);
                            }
                        }
                    }
                }
            }
        }

        Connections {
            target: MusicApi
            function onLyricsDataChanged() {
                console.log("更换源，重排新歌词");
                lyricContent.heights = [];
                lyricContent.prefixSum = [];
                lyricContent.currentLine = 0;
                lyricContent.scrollOffset = 0;
                Qt.callLater(lyricContent.rebuild);
            }
        }


        function rebuild() {
            var sum = 0, arr = [];
            for (var i = 0; i < lyricRep.count; i++) {
                arr.push(sum);
                var it = lyricRep.itemAt(i);
                var h = it ? it.height : 0;
                heights[i] = h;
                sum += h; // 累加下一行起点
            }
            prefixSum = arr;
            for (var i = 0; i < lyricRep.count; i++) {
                var basicIndexY = prefixSum[currentLine];
                if (lyricRep.itemAt(i)) lyricRep.itemAt(i).standY = Math.floor(prefixSum[i] - basicIndexY + lyricContent.height * lyricContent.alignPos);
            }
        }

        Component.onCompleted: Qt.callLater(rebuild)

        Repeater {
            id: lyricRep
            model: MusicApi.lyricsData ? MusicApi.lyricsData : [{time: 0, text: "纯音乐，请欣赏"}]

            delegate: Item {
                id: lyricItem
                x: 10
                width: lyricContent.width - 20
                height: lyricsText.implicitHeight + lyricTransText.height + lyricContent.lineSpacing
                //visible: Math.abs(index - lyricContent.currentLine) <= lyricContent.overscan

                readonly property bool isCurrent: index === lyricContent.currentLine
                readonly property bool isFlowActive: MusicApi.lyricsData[index].info ? (index == lyricContent.currentLine || index == lyricContent.currentLine - 1) : false
                readonly property int nowPosition: isFlowActive ? lyricContent.currentPlayTime - MusicApi.lyricsData[index].time : 0
                property real opacityAnime: isCurrent && !waitAnimeSection.visible ? 1.0 : 0.0
                Behavior on opacityAnime { NumberAnimation { duration: 320 } }
                property real standY: 0.0
                y: standY + lyricContent.scrollOffset

                //y: displayY
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
                        easing.bezierCurve: [ 0.24, 0.06, lyricContent.springValue, 1.04, 1, 1 ]
                    }
                }

                function animeTo(ty, instant) {
                    lyricAnime.running = false;
                    var d = index - lyricContent.currentLine;
                    var durationMs;
                    lyricItem.standY = lyricItem.standY;
                    // 不使用弹簧动画的外部区域（非index>-3至7)，使用融合动画：绑定至统一的动画值，提升性能喵~
                    if(d < -3) {
                        // 检测防止动画乱跑，检测稳定即融合
                        if(ty - lyricItem.standY - fixedAnime.to == 0) {
                            var nowY = lyricItem.standY;
                            lyricItem.standY = Qt.binding(function() { return (lyricContent.fixedH + nowY) });
                            return;
                        } else {
                            durationMs = 0;
                        }
                    } else if(d > 7) {
                        if(ty - lyricItem.standY - fixedAnime.to == 0) {
                            var nowY = lyricItem.standY;
                            lyricItem.standY = Qt.binding(function() { return (lyricContent.finalH + nowY) });
                            return;
                        } else {
                            //预计算： 12 ** 1.2 * 24
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
                    text: MusicApi.lyricsData[index].text || ""
                    font.weight: Style.settings.textWidth
                    font.pixelSize: lyricContent.lyricHeight
                    color: "#ffffffff"
                    transformOrigin: Item.BottomLeft
                    wrapMode: Text.Wrap
                    scale: lyricItem.isCurrent && !MusicApi.lyricsData[index].info ? 1.02 : 1.00
                    opacity: MusicApi.lyricsData[index].info ? 0.4 : (0.4 + lyricItem.opacityAnime * 0.5)
                    visible: MusicApi.lyricsData[index].info ? !lyricItem.isFlowActive : true
                    horizontalAlignment: controlMaxLoader.lyricsType === 2 ? Text.AlignHCenter : Text.AlignLeft
                    Behavior on scale { NumberAnimation { duration: 640; easing.type: Easing.InOutCubic } }
                }

                Text {
                    id: lyricTransText
                    anchors.top: lyricsText.bottom
                    transformOrigin: Item.TopLeft
                    scale: lyricItem.isCurrent && !waitAnimeSection.visible ? 1.02 : 1.00
                    visible: text !== ""
                    height: visible ? implicitHeight * 1.5 : 0
                    text: MusicApi.lyricsTranslate.length !== 0 && lyricContent.openTranslate ? (MusicApi.lyricsTranslate[index] || "") : ""
                    width: parent.width
                    horizontalAlignment: controlMaxLoader.lyricsType === 2 ? Text.AlignHCenter : Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    font.weight: Style.settings.textWidth
                    color: "#ffffffff"
                    opacity: 0.4 + lyricItem.opacityAnime * 0.2
                    Behavior on scale { NumberAnimation { duration: 640; easing.type: Easing.InOutCubic } }
                    font.pixelSize: lyricContent.lyricHeight / 1.5
                }

                Flow {
                    id: lyricFlow
                    width: lyricItem.width
                    height: lyricItem.height
                    transformOrigin: Item.BottomLeft
                    scale: lyricItem.isCurrent && !waitAnimeSection.visible ? 1.02 : 1.00
                    x: controlMaxLoader.lyricsType === 2 ? (width - implicitWidth) / 2 : 0
                    visible: lyricItem.isFlowActive
                    z: 1
                    Behavior on scale { NumberAnimation { duration: 640; easing.type: Easing.InOutCubic } }
                    Repeater {
                        id: linesText
                        model: lyricItem.isFlowActive ? (MusicApi.lyricsData[index].info || 0) : 0
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
                                color: "#ffffffff"
                                opacity: 0.4
                                //Behavior on y { NumberAnimation { duration: 240 + linesText.model[index].duration * 10; easing.type: Easing.OutExpo } }
                            }
                            LinearGradient {
                                property int countToWidth: lyricItem.nowPosition > linesText.model[index].offset && lyricItem.isFlowActive ? width + 16 : 0
                                Behavior on countToWidth { NumberAnimation { Component.onCompleted: duration = linesText.model[index].duration / mainMedia.playbackRate * (width + 16) / width } }
                                //linesText.model[index].duration !== 0 ? (lyricItem.nowPosition - linesText.model[index].offset) / linesText.model[index].duration * width : (lyricItem.nowPosition - linesText.model[index].offset) * width
                                width: parent.width
                                height: parent.height
                                y: lyricFlowText.y
                                opacity: lyricItem.opacityAnime
                                source: lyricFlowText
                                start: Qt.point(countToWidth - 16, 0)
                                end: Qt.point(countToWidth, 0)
                                gradient: Gradient {
                                    GradientStop { position: 0.0; color: "#ffffffff" }
                                    GradientStop { position: 1.0; color: "#66ffffff" }
                                }
                            }
                        }
                    }
                }
            }
        }

        Item {
            id: waitAnimeSection
            //opacity: waitSectionLoader.opacity
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
            ScriptAction { script: { waitAnimeSection.visible = true; waitAnimeSection.lightState = 0; waitOpenAnime.lightDuration = MusicApi.lyricsData[lyricContent.currentLine + 1].time - MusicApi.lyricsData[lyricContent.currentLine].time - MusicApi.lyricsData[lyricContent.currentLine].info[MusicApi.lyricsData[lyricContent.currentLine].info.length - 1].offset - MusicApi.lyricsData[lyricContent.currentLine].info[MusicApi.lyricsData[lyricContent.currentLine].info.length - 1].duration - 420 } }
            PauseAnimation { duration: 100 }
            ParallelAnimation {
                NumberAnimation { target: waitAnimeSection; property: "opacity"; from: 0; to: 1; duration: 460; easing.type: Easing.OutCubic }
                NumberAnimation { target: waitAnimeSection; property: "scale"; from: 0; to: 1; duration: 460; easing.type: Easing.OutCubic }
            }
            NumberAnimation { target: waitAnimeSection; property: "lightState"; from: 0; to: 3; duration: waitOpenAnime.lightDuration / mainMedia.playbackRate }
            //ScriptAction { script: console.log("动画完成:",waitOpenAnime.lightDuration); }
        }
        SequentialAnimation {
            id: waitOutAnime
            ParallelAnimation {
                NumberAnimation { target: waitAnimeSection; property: "opacity"; from: 1; to: 0; duration: 320 }
                NumberAnimation { target: waitAnimeSection; property: "scale"; from: 1; to: 0; duration: 320 }
            }
            ScriptAction { script: waitAnimeSection.visible = false }
        }

        // 滚动动画区
        WheelHandler {
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            onWheel: (event) => {
                lyricContent.isUserScrolling = true;
                scrollAnime.running = false;
                var to = Math.max(Math.min(scrollAnime.to + event.angleDelta.y * 0.25 * Qt.application.styleHints.wheelScrollLines,lyricContent.prefixSum[lyricContent.currentLine]),lyricContent.prefixSum[lyricContent.currentLine] - lyricContent.prefixSum[lyricRep.count - 1]);
                scrollAnime.to = to;
                scrollAnime.running = true;
                event.accepted = true;
            }
        }

        SequentialAnimation {
            id: scrollAnime
            property int to: 0
            NumberAnimation {
                target: lyricContent
                property: "scrollOffset"
                duration: 640
                to: scrollAnime.to
                easing.type: Easing.OutExpo
            }
            ScriptAction {
                script: lyricContent.isUserScrolling = false;
            }
        }
    }


    QOptionDialog {
        id: maxLyricsDialog
        title: "播放器样式"
        dialogContentHeight: 460
        options: Column {
            width: parent.width
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
                    choice: controlMaxLoader.basicCd ? 1 : 0
                    onTransformed: (choiced) => {
                        controlMaxLoader.basicCd = (choiced === 1);
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
                    choice: mainLayout.maxLyricType
                    onTransformed: (choiced) => {
                        switch(choiced) {
                        case 0:
                            mainLayout.maxLyricType = 0
                            mainLayout.state = "MaxedNormal"
                            break;
                        case 1:
                            mainLayout.maxLyricType = 1
                            mainLayout.state = "MaxedCover"
                            break;
                        case 2:
                            mainLayout.maxLyricType = 2
                            mainLayout.state = "MaxedLyric"
                            break;
                        }
                    }
                }
            }
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
                label: "高级逐行弹簧动画"
                width: parent.width
                QSwitch {
                    height: 36; width: 120
                    anchors.right: parent.right
                    switchTrue: Style.settings.premiumLyricAnime
                    onToggled: Style.settings.premiumLyricAnime = !Style.settings.premiumLyricAnime
                }
            }
            SettingItem {
                label: "显示音波效果"
                width: parent.width
                QSwitch {
                    height: 36; width: 120
                    anchors.right: parent.right
                    switchTrue: Style.settings.waveDisplay
                    onToggled: Style.settings.waveDisplay = !Style.settings.waveDisplay
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
                        controlMaxLoader.hideHeight = 0;
                    }
                }
            }
        }
    }
}