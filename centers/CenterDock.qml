// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
// 底部控制坞：当前曲目 + 传输控制 + 进度 + 音量（实时模糊玻璃）喵~
import QtQuick
import QtQuick.Effects
import QueMusic 1.0
import 'qrc:/QueMusic/components'

QBlurCard {
    id: root
    height: 84
    blurMax: 64
    blur: 1.0
    saturation: 1.25
    borderRadius: 28
    shadowEffect: true
    cardColor: "#5210131b"
    borderColor: "#24ffffff"
    borderWidth: 1

    property Item blurSource
    rectXy: blurSource ? root.mapToItem(blurSource, 0, 0, root.width, root.height)
                       : Qt.rect(root.x, root.y, root.width, root.height)

    property url cover: "qrc:/QueMusic/resources/app/musicpic.png"
    property string title: ""
    property string artist: ""
    property bool playing: false
    property int position: 0
    property int duration: 0
    property real volume: 0.6
    property bool muted: false
    property int cycleIndex: 0

    signal queueClicked
    signal seek(real ratio)
    signal volumeMoved(real v)

    function fmt(ms: real): string {
        const s = Math.max(0, Math.floor((ms || 0) / 1000))
        return Math.floor(s / 60) + ":" + ("0" + (s % 60)).slice(-2)
    }

    // 顶部高光缝：玻璃的上沿反光
    Rectangle {
        x: parent.width * 0.04
        y: 1
        width: parent.width * 0.92
        height: 1
        radius: 1
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0; color: "#00ffffff" }
            GradientStop { position: 0.5; color: "#4dffffff" }
            GradientStop { position: 1; color: "#00ffffff" }
        }
    }

    // 细长条：进度 / 音量
    component Bar: Item {
        id: bar
        property real value: 0
        signal moved(real v)
        property bool seeking: false
        property real seekValue: 0
        readonly property real shown: seeking ? seekValue : value

        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width
            height: barArea.containsMouse || bar.seeking ? 8 : 5
            radius: height / 2
            color: "#20ffffff"
            Behavior on height { NumberAnimation { duration: 140; easing.type: Easing.OutExpo } }
            Rectangle {
                width: parent.width * bar.shown
                height: parent.height
                radius: height / 2
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0; color: center.c1 }
                    GradientStop { position: 1; color: Qt.lighter(center.c2, 1.1) }
                }
            }
            Rectangle {
                x: parent.width * bar.shown - 7
                anchors.verticalCenter: parent.verticalCenter
                width: 14
                height: 14
                radius: 7
                color: "#ffffff"
                scale: barArea.containsMouse || bar.seeking ? 1 : 0
                Behavior on scale { NumberAnimation { duration: 140; easing.type: Easing.OutExpo } }
            }
        }
        MouseArea {
            id: barArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onPressed: m => { bar.seeking = true; bar.seekValue = clamp(m.x / width) }
            onPositionChanged: m => { if (bar.seeking) bar.seekValue = clamp(m.x / width) }
            onReleased: m => { bar.seeking = false; bar.moved(clamp(m.x / width)) }
            onCanceled: bar.seeking = false
        }
        function clamp(v: real): real { return Math.max(0, Math.min(1, v)) }
    }

    Row {
        anchors { left: parent.left; leftMargin: 26; verticalCenter: parent.verticalCenter }
        spacing: 14
        Item {
            width: 58
            height: 58
            Rectangle {
                id: coverWrap
                width: 58
                height: 58
                radius: 15
                color: "#ffffff"
                QPicture {
                    anchors.fill: parent
                    anchors.margins: 1
                    radius: 14
                    source: root.cover
                    sourceSize: Qt.size(128, 128)
                }
            }
            // 播放中角标：跳动音条
            Rectangle {
                x: parent.width - 13
                y: -5
                width: 22
                height: 22
                radius: 11
                color: Qt.rgba(center.c1.r, center.c1.g, center.c1.b, 0.95)
                border.width: 2
                border.color: "#30ffffff"
                visible: root.playing
                scale: root.playing ? 1 : 0
                Behavior on scale { NumberAnimation { duration: 180; easing.type: Easing.OutBack } }
                Row {
                    anchors.centerIn: parent
                    spacing: 2
                    Repeater {
                        model: 3
                        Rectangle {
                            required property int index
                            width: 2.5
                            radius: 1.25
                            color: "#ffffff"
                            anchors.verticalCenter: parent.verticalCenter
                            SequentialAnimation on height {
                                running: root.playing && center.visible
                                loops: Animation.Infinite
                                NumberAnimation { to: 4; duration: 300 + index * 110; easing.type: Easing.InOutSine }
                                NumberAnimation { to: 11; duration: 300 + index * 110; easing.type: Easing.InOutSine }
                            }
                        }
                    }
                }
            }
        }
        Column {
            width: 190
            anchors.verticalCenter: parent.verticalCenter
            spacing: 3
            Text {
                width: parent.width
                text: root.title || "未在播放"
                font.pixelSize: 14
                font.weight: Font.DemiBold
                color: "#f2f5fa"
                elide: Text.ElideRight
            }
            Text {
                width: parent.width
                text: root.artist || "—"
                font.pixelSize: 12
                color: "#93a0b2"
                elide: Text.ElideRight
            }
        }
    }

    Column {
        anchors { horizontalCenter: parent.horizontalCenter; verticalCenter: parent.verticalCenter }
        spacing: 5
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 6
            SButton {
                iconCharacter: ["\uf118", "\uf115", "\uf0e2", "\uf03b"][Options.settings.cycleIndex]
                width: 44
                height: 44
                radius: 22
                buttonColor: "transparent"
                hoverColor: "#1affffff"
                iconColor: "#c3cddb"
                shadowEnabled: false
                iconSize: 16
                onClicked: Options.settings.cycleIndex = (Options.settings.cycleIndex + 1) % 4
                tipText: "播放模式"
            }
            SButton {
                iconCharacter: "\uf0dc"
                width: 44
                height: 44
                radius: 22
                buttonColor: "transparent"
                hoverColor: "#1affffff"
                iconColor: "#e6ecf4"
                iconSize: 17
                shadowEnabled: false
                onClicked: Playback.previous()
                tipText: "上一首"
            }
            Item {
                width: 50
                height: 50
                anchors.verticalCenter: parent.verticalCenter
                // 播放中呼吸光晕
                RectangularShadow {
                    anchors.fill: parent
                    anchors.margins: -8
                    radius: 30
                    blur: 20
                    color: Qt.rgba(center.c1.r, center.c1.g, center.c1.b, 0.85)
                    opacity: 0.8
                    scale: root.playing ? 1 : 0.4
                    visible: root.playing
                    Behavior on scale { NumberAnimation { duration: 320; easing.type: Easing.OutCubic } }
                    SequentialAnimation on opacity {
                        running: root.playing && center.visible
                        loops: Animation.Infinite
                        alwaysRunToEnd: true
                        NumberAnimation { to: 0.35; duration: 1400; easing.type: Easing.InOutSine }
                        NumberAnimation { to: 0.9; duration: 1400; easing.type: Easing.InOutSine }
                    }
                }
                Rectangle {
                    anchors.fill: parent
                    radius: 25
                    gradient: Gradient {
                        GradientStop { position: 0; color: Qt.lighter(center.c1, 1.2) }
                        GradientStop { position: 1; color: center.c2 }
                    }
                }
                SButton {
                    anchors.fill: parent
                    iconCharacter: root.playing ? "\uf02f" : "\uf00e"
                    buttonColor: "transparent"
                    hoverColor: "#2fffffff"
                    iconColor: "#ffffff"
                    shadowEnabled: false
                    iconSize: 20
                    onClicked: Playback.togglePlay()
                    tipText: root.playing ? "暂停" : "播放"
                }
            }
            SButton {
                iconCharacter: "\uf0d9"
                width: 44
                height: 44
                radius: 22
                buttonColor: "transparent"
                hoverColor: "#1affffff"
                iconColor: "#e6ecf4"
                iconSize: 17
                shadowEnabled: false
                onClicked: Playback.next(false)
                tipText: "下一首"
            }
            SButton {
                iconCharacter: "\uf0e2"
                width: 44
                height: 44
                radius: 22
                buttonColor: "transparent"
                hoverColor: "#1affffff"
                iconColor: "#c3cddb"
                iconSize: 16
                shadowEnabled: false
                onClicked: Playback.next(true)
                tipText: "随机播放"
            }
        }
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 10
            Text {
                width: 44
                text: root.fmt(root.position)
                font.pixelSize: 12
                color: "#8d99aa"
                horizontalAlignment: Text.AlignRight
            }
            Bar {
                width: 300
                height: 16
                value: root.duration > 0 ? root.position / root.duration : 0
                onMoved: v => root.seek(v)
            }
            Text {
                width: 44
                text: root.fmt(root.duration)
                font.pixelSize: 12
                color: "#8d99aa"
            }
        }
    }

    Row {
        anchors { right: parent.right; rightMargin: 26; verticalCenter: parent.verticalCenter }
        spacing: 10
        SButton {
            iconCharacter: "\uf043"
            width: 40
            height: 40
            radius: 20
            buttonColor: "transparent"
            hoverColor: "#1affffff"
            iconColor: root.muted ? "#5f6b7c" : "#c3cddb"
            shadowEnabled: false
            onClicked: Playback.toggleMute()
            tipText: "静音"
            WheelHandler {
                onWheel: e => {
                    Playback.stepVolume(e.angleDelta.y > 0 ? Playback.volumeStep : -Playback.volumeStep)
                    e.accepted = true
                }
            }
        }
        Bar {
            width: 96
            height: 16
            anchors.verticalCenter: parent.verticalCenter
            value: Options.settings.musicVolume
            onMoved: v => Playback.setVolume(v)
        }
        SButton {
            iconCharacter: "\uf098"
            width: 40
            height: 40
            radius: 20
            buttonColor: "transparent"
            hoverColor: "#1affffff"
            iconColor: "#c3cddb"
            iconSize: 17
            shadowEnabled: false
            onClicked: root.queueClicked()
            tipText: "播放队列"
        }
    }
}
