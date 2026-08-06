// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024-2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Shapes
import Qt5Compat.GraphicalEffects
import QueMusic 1.0
import GetWave 1.0
import MeshGradientItem 1.0
import 'qrc:/QueMusic/components'

Item {
    id: musicControlMax
    //color: "black"
    //layer.enabled: true
    readonly property int standHeight: Style.settings.lyricSize + mainLayout.height / 32 + mainLayout.width / 56
    readonly property int infoWidth: lyricModeText.width / 2
    property color mainColor: "#00ee66"
    property color secondColor: "#00b1ee"
    property color thirdColor: "#9d4edd"
    property real springValue: 0.00
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
        lyricLayout.scrollTimeLyric();
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
        interval: 2400
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
        width: 40
        height: 40
        radius: 10
        //visible: false
        buttonColor: "transparent"
        hoverColor: Qt.rgba(0,0,0,0.2)
        iconColor: lyricsView.openTranslate ? Style.themes.containColor : "#eeeeee"
        iconSize: Style.settings.texticon
        shadowEnabled: false
        onClicked: {
            lyricsView.openTranslate = !lyricsView.openTranslate;
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
        visible: x !== -400
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
        visible: x !== -400
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
        opacity: visible ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 300 } }
    }



    // 歌词部分
    Item {
        id: lyricLayout
        x: controlMaxLoader.lyricsX
        y: 60
        width: controlMaxLoader.lyricsType === 2 ? musicControlMax.width - 96 : musicControlMax.width * 0.54
        height: parent.height - 120
        clip: false
        visible: controlMaxLoader.lyricsType !== 1
        Label {
            anchors.fill: parent
            text: "加载中"
            visible: false
            font.bold: true
            font.pixelSize: 18
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            color: Qt.rgba(1,1,1,0.8)
            //Behavior on opacity { NumberAnimation { duration: 300 } }
        }
        Timer {
            interval: 240
            running: mainMedia.onMedia
            repeat: true
            onTriggered: {
                if(!lyricsView.moving) {
                    var idx = 0;
                    if(mainMedia.position + 240 > MusicApi.lyricsData[lyricsView.currentIndex].time) {
                        for (var i = lyricsView.currentIndex; i < MusicApi.lyricsData.length; i++) {
                            if (mainMedia.position + 240 > MusicApi.lyricsData[i].time)
                                idx = i;
                            else break;
                        }
                    } else {
                        for (var i = lyricsView.currentIndex; i >= 0; i--) {
                            if (mainMedia.position + 240 < MusicApi.lyricsData[i].time)
                                idx = i;
                            else break;
                        }
                    }
                    var notSame = (lyricsView.currentIndex !== idx);

                    lyricsView.currentIndex = idx;
                    //lyricsView.positionViewAtIndex(idx, ListView.Center);
                    if(!lyricHandleAnime.running && notSame) {
                        lyricLayout.scrollTimeLyric();
                        if(notSame) {
                            var animeHeight = lyricsView.currentItem.y - lyricsView.height * 0.32 - lyricsView.contentY;
                            console.log("lastHeight: ",lyricsView.lastHeight);
                            lyricLastAnime.running = false;
                            lyricsView.lastHeight = 0;
                            lyricLastAnime.to = animeHeight;
                            lyricLastAnime.running = true;
                        }
                    }
                    if(MusicApi.lyricsData[idx].info) {
                        var lyricLastLineData = MusicApi.lyricsData[idx].info[MusicApi.lyricsData[idx].info.length - 1];
                        if(MusicApi.lyricsData[idx + 1].time - MusicApi.lyricsData[idx].time - lyricLastLineData.offset - lyricLastLineData.duration > 2500 && mainMedia.position > MusicApi.lyricsData[idx].time + lyricLastLineData.offset + lyricLastLineData.duration) {
                            if(lyricsView.currentItem.heightScale === 0.0) {
                                console.log("开始运行等待动画。");
                                lyricListAnime.running = false;
                                lyricListAnime.to = lyricsView.currentItem.y - lyricsView.height * 0.32 + musicControlMax.standHeight;
                                lyricsView.currentItem.waitOpenAnime.running = true;
                                lyricListAnime.running = true;
                            }
                        }
                    }
                }
            }
        }
        function scrollTimeLyric() {
            lyricListAnime.running = false;
            var animeHeight = lyricsView.currentItem.y - lyricsView.height * 0.32 - lyricsView.contentY;
            //if(lyricsView.currentItem.heightScale > 0) lyricListAnime.to = toY - lyricsView.height * 0.36 + musicControlMax.standHeight;
            //lyricsView.contentY = toY - lyricsView.height * 0.4
            var springValue = animeHeight - 400 > 0 ? Math.floor((animeHeight - 400) / 20) / -200 : 0.00;
            if(springValue < -0.50) {
                musicControlMax.springValue = -0.50;
            } else {
                musicControlMax.springValue = springValue;
            }
            if(lyricsView.isWaitOut) {
                lyricListAnime.to = lyricsView.currentItem.y - lyricsView.height * 0.32 - musicControlMax.standHeight;
                lyricsView.isWaitOut = false;
            } else {
                lyricListAnime.to = lyricsView.currentItem.y - lyricsView.height * 0.32;
            }

            lyricListAnime.running = true;
            lyricsView.scrollToY = lyricListAnime.to;
        }

        ListView {
            id: lyricsView
            anchors.fill: parent
            spacing: musicControlMax.standHeight / 1.6
            opacity: 0
            model: MusicApi.lyricsData || [{time: 0,text: "纯音乐，请欣赏"}]
            //property int lyricHeight: musicControlMax.standHeight / 2
            topMargin: 360
            bottomMargin: 420
            leftMargin: 10
            //reuseItems: true
            reuseItems: true
            currentIndex: 1
            cacheBuffer: contentHeight//height * 2
            maximumFlickVelocity: 10000
            synchronousDrag: true
            readonly property int lyricHeight: musicControlMax.standHeight / 2
            property int scrollToY: lyricsView.contentY
            property bool openTranslate: true
            property real lastHeight: 0
            property bool isWaitOut: false

            // 驱动歌词滚动动画
            NumberAnimation {
                id: lyricListAnime
                target: lyricsView
                property: "contentY"
                duration: 450
                easing.type: Easing.BezierSpline
                easing.bezierCurve: [ 0.24, 0.06, musicControlMax.springValue, 1.04, 1, 1 ]
            }

            //驱动弹簧抵消动画
            NumberAnimation {
                id: lyricLastAnime
                target: lyricsView
                property: "lastHeight"
                from: 0
                duration: 450
                easing.type: Easing.BezierSpline
                easing.bezierCurve: [ 0.24, 0.06, musicControlMax.springValue, 1.04, 1, 1 ]
            }

            WheelHandler {
                property real scrollMultiplier: Qt.application.styleHints.wheelScrollLines

                onWheel: (event) => {

                             lyricsView.scrollToY = Math.max( -360, Math.min( lyricsView.scrollToY - (event.angleDelta.y / 4 * scrollMultiplier), lyricsView.contentHeight - lyricsView.height + 420))
                             lyricHandleAnime.running = false
                             lyricHandleAnime.running = true

                             event.accepted = true
                         }
            }
            // 驱动滚轮动画
            SequentialAnimation {
                id: lyricHandleAnime
                NumberAnimation {
                    target: lyricsView
                    property: "contentY"
                    duration: 360
                    to: lyricsView.scrollToY
                    easing.type: Easing.OutCubic
                }
                //ScriptAction {
                    //script: lyricsView.moving = false
                //}
            }

            onCurrentIndexChanged: {
                if (lyricHandleAnime.running) return;
                var toIndex = currentIndex;
                for (var i = toIndex - 4; i < toIndex + 8; i++) {
                    if (i < 0 || i >= model.count) continue;
                    var item = itemAtIndex(i);
                    if (item) item.handleIndexChanged(toIndex);
                }
            }

            //Behavior on contentY { NumberAnimation { duration: 440; easing.type: Easing.Bezier; easing.bezierCurve: [ 0.24, 0.06, 0.00, 1.00, 1, 1 ] } }
            //signal turnlyriced(int toIndex)

            delegate: Item {
                //color: "transparent"
                //border.color: "green"
                //border.width: 1
                id: lyricItem
                width: lyricsView.width - 10
                height: lyricsText.implicitHeight + lyricTransText.height + heightScale
                readonly property int heightScale: musicControlMax.standHeight * waitSectionLoader.scale
                readonly property int nowPosition: mainMedia.position - MusicApi.lyricsData[index].time
                readonly property int isFlowLyric: (index === lyricsView.currentIndex) || (index === lyricsView.currentIndex - 1)
                readonly property int isCurrentIndex: index === lyricsView.currentIndex
                //Behavior on height { NumberAnimation { duration: 460; easing.type: Easing.Bezier; easing.bezierCurve: [ 0.24, 0.06, 0.00, 1.00, 1, 1 ] } }
                property alias waitOpenAnime: waitOpenAnime
                Text {
                    z: 0
                    id: lyricsText
                    //width: lyricsView.width
                    width: lyricItem.width - lyricsView.lyricHeight / 4
                    //height: musicControlMax.standHeight / 2
                    text: MusicApi.lyricsData[index].text || model.text
                    font.weight: Style.settings.textWidth
                    font.pixelSize: lyricsView.lyricHeight
                    color: "#ffffffff"
                    wrapMode: Text.Wrap
                    //opacity: 0.4
                    opacity: MusicApi.lyricsData[index].info ? 0.4 : (lyricItem.isCurrentIndex ? 0.9 : 0.4)
                    visible: MusicApi.lyricsData[index].info ? !lyricItem.isFlowLyric : true

                    Behavior on opacity { NumberAnimation { duration: 320 } }
                    horizontalAlignment: controlMaxLoader.lyricsType === 2 ? Text.AlignHCenter : Text.AlignLeft

                }
                Text {
                    id: lyricTransText
                    anchors.top: lyricsText.bottom
                    visible: text !== ""
                    height: visible ? implicitHeight * 2 : 0
                    text: MusicApi.lyricsTranslate.length !== 0 && lyricsView.openTranslate ? (MusicApi.lyricsTranslate[index] || "") : ""
                    width: parent.width
                    horizontalAlignment: controlMaxLoader.lyricsType === 2 ? Text.AlignHCenter : Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    font.weight: Style.settings.textWidth
                    color: "#ffffffff"
                    opacity: lyricItem.isCurrentIndex && lyricItem.heightScale === 0 ? 0.6 : 0.4

                    Behavior on opacity { NumberAnimation { duration: 320 } }
                    font.pixelSize: lyricsView.lyricHeight / 1.5
                }

                Flow {
                    id: lyricFlow
                    width: lyricItem.width
                    height: lyricItem.height
                    y: lyricsText.y
                    x: controlMaxLoader.lyricsType === 2 ? (width - implicitWidth) / 2 : 0
                    //spacing: 10
                    //opacity: index === lyricsView.currentIndex ? 1 : 0
                    visible: MusicApi.lyricsData[index].info ? lyricItem.isFlowLyric : false
                    z: 1
                    //Behavior on opacity { NumberAnimation { duration: 320 } }
                    Repeater {
                        id: linesText
                        model: lyricItem.isFlowLyric || index === lyricsView.currentIndex + 1 ? (MusicApi.lyricsData[index].info || 0) : 0
                        delegate: Item {
                            width: lyricFlowText.width
                            height: lyricFlowText.height
                            Text {
                                id: lyricFlowText
                                text: linesText.model[index].text
                                y: lyricItem.nowPosition > linesText.model[index].offset ? -3 : 0
                                font.weight: Style.settings.textWidth
                                font.pixelSize: lyricsView.lyricHeight
                                color: "#ffffffff"
                                opacity: 0.4
                                //visible: true
                                Behavior on y { enabled: lyricItem.isFlowLyric; NumberAnimation { duration: 240 + linesText.model[index].duration * 10; easing.type: Easing.OutExpo } }
                            }
                            LinearGradient {
                                property int countToWidth: linesText.model[index].duration !== 0 ? (lyricItem.nowPosition - linesText.model[index].offset) / linesText.model[index].duration * width : (lyricItem.nowPosition - linesText.model[index].offset) * width
                                width: parent.width
                                height: parent.height
                                y: lyricFlowText.y
                                opacity: lyricItem.isCurrentIndex && lyricItem.heightScale === 0 ? 1.0 : 0.0
                                Behavior on opacity { NumberAnimation { duration: 320 } }
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

                // 等待伴奏动画
                Loader {
                    id: waitSectionLoader
                    opacity: 0
                    property real scale: 0
                    property int lightState: 0
                    active: false
                    visible: active
                    sourceComponent: Item {
                        id: waitAnimeSection
                        x: 20
                        //opacity: waitSectionLoader.opacity
                        scale: waitSectionLoader.scale
                        transformOrigin: Popup.TopLeft
                        y: lyricsText.implicitHeight + lyricTransText.height + lyricsView.lyricHeight
                        width: lyricsView.lyricHeight * 4
                        height: lyricsView.lyricHeight
                        Rectangle {
                            width: waitAnimeSection.height
                            height: waitAnimeSection.height
                            radius: waitAnimeSection.height / 2
                            color: waitSectionLoader.lightState > 0 ? "#ffffffff" : "#99ffffff"
                            Behavior on color { ColorAnimation { duration: 320; easing.type: Easing.OutCubic } }
                        }
                        Rectangle {
                            x: waitAnimeSection.height * 1.5
                            width: waitAnimeSection.height
                            height: waitAnimeSection.height
                            radius: waitAnimeSection.height / 2
                            color: waitSectionLoader.lightState > 1 ? "#ffffffff" : "#99ffffff"
                            Behavior on color { ColorAnimation { duration: 320; easing.type: Easing.OutCubic } }
                        }
                        Rectangle {
                            x: waitAnimeSection.height * 3
                            width: waitAnimeSection.height
                            height: waitAnimeSection.height
                            radius: waitAnimeSection.height / 2
                            color: waitSectionLoader.lightState > 2 ? "#ffffffff" : "#99ffffff"
                            Behavior on color { ColorAnimation { duration: 320; easing.type: Easing.OutCubic } }
                        }
                    }
                }

                SequentialAnimation {
                    id: waitOpenAnime
                    property int lightDuration: 1000
                    ScriptAction { script: { waitSectionLoader.active = true; waitSectionLoader.lightState = 0; waitOpenAnime.lightDuration = MusicApi.lyricsData[index + 1].time - MusicApi.lyricsData[index].time - MusicApi.lyricsData[index].info[MusicApi.lyricsData[index].info.length - 1].offset - MusicApi.lyricsData[index].info[MusicApi.lyricsData[index].info.length - 1].duration - 420 } }
                    ParallelAnimation {
                        NumberAnimation { target: waitSectionLoader; property: "opacity"; from: 0; to: 1; duration: 420; easing.type: Easing.OutCubic }
                        NumberAnimation { target: waitSectionLoader; property: "scale"; from: 0; to: 1; duration: 420; easing.type: Easing.OutCubic }
                    }
                    NumberAnimation { target: waitSectionLoader; property: "lightState"; from: 0; to: 3; duration: waitOpenAnime.lightDuration }
                    //ScriptAction { script: console.log("动画完成:",waitOpenAnime.lightDuration); }
                }
                SequentialAnimation {
                    id: waitOutAnime
                    ParallelAnimation {
                        NumberAnimation { target: waitSectionLoader; property: "opacity"; from: 1; to: 0; duration: 320 }
                        NumberAnimation { target: waitSectionLoader; property: "scale"; from: 1; to: 0; duration: 320 }
                    }
                    ScriptAction { script: waitSectionLoader.active = false }
                }

                function handleIndexChanged(toIndex) {
                    if (index > toIndex - 4 && index < toIndex + 7) {
                        var startY = -lyricsView.contentY;
                        lyricAnime.byteY = 0;
                        lyricsText.y = Qt.binding(function() { return (lyricsView.lastHeight + lyricAnime.byteY) });
                        lyricYAnime.duration = 450 + (index - toIndex + 4) * 32;
                        lyricAnime.duration = (index - toIndex + 4) ** 1.2 * 24;
                        lyricYAnime.to = lyricsView.contentY + lyricsView.height * 0.32 - lyricsView.currentItem.y;//-startY - lyricsView.currentItem.y + lyricsView.height * 0.36;
                        //console.log("弹簧动画运行");
                        lyricAnime.running = false;
                        lyricAnime.running = true;
                        if(waitSectionLoader.active) {
                            lyricsView.isWaitOut = true;
                            waitOpenAnime.running = false;
                            waitOutAnime.running = true;
                        }
                    }
                }

                SequentialAnimation {
                    id: lyricAnime
                    property int duration: 0
                    property real byteY: 0
                    NumberAnimation {
                        duration: lyricAnime.duration
                    }
                    NumberAnimation {
                        id: lyricYAnime
                        target: lyricAnime
                        property: "byteY"
                        duration: 450
                        easing.type: Easing.Bezier
                        easing.bezierCurve: [ 0.24, 0.06, musicControlMax.springValue, 1.04, 1, 1 ]
                    }
                    ScriptAction {
                        script: {
                            lyricsText.y = 0;
                            //lyricItem.border.color = "green";
                        }
                    }
                }
            }
        }
        LinearGradient {
            id: lyricsGradient
            anchors.fill: lyricsView
            //source: lyricLayout
            visible: false
            gradient: Gradient {
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 0.2; color: "white" }
                GradientStop { position: 0.6; color: "white" }
                GradientStop { position: 1.0; color: "transparent" }
            }
        }
        LinearGradient {
            id: lyricsBlur
            anchors.fill: lyricsView
            //source: lyricLayout
            visible: false
            gradient: Gradient {
                GradientStop { position: 0.0; color: "white" }
                GradientStop { position: 0.32; color: "transparent" }
                GradientStop { position: 0.45; color: "transparent" }
                GradientStop { position: 1.0; color: "white" }
            }
        }
        MaskedBlur {
            id: lyricBlurSource
            anchors.fill: lyricsView
            source: lyricsView
            maskSource: lyricsBlur
            radius: Style.settings.maskBlur ? 12 : 0
            samples: Style.settings.maskBlur ? 18 : 0
            visible: false
        }
        OpacityMask {
            anchors.fill: lyricsView
            source: lyricBlurSource
            maskSource: lyricsGradient
        }
    }

    QOptionDialog {
        id: maxLyricsDialog
        title: "播放器样式"
        dialogContentHeight: 300
        options: Column {
            width: parent.width
            spacing: 16
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
                    onToggled: Style.settings.lyricHideGui = !Style.settings.lyricHideGui
                }
            }
        }
    }
}