// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
// 底部控制坞：当前曲目 + 传输控制 + 进度 + 音量
import QtQuick
import QueMusic 1.0
import 'qrc:/QueMusic/components'

Rectangle {
    id: root
    height: 80
    radius: 24
    color: "#fafcfdff"
    border.width: 3
    border.color: "#dadbdd"

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

    function fmt(ms) {
        var s = Math.max(0, Math.floor((ms || 0) / 1000))
        return Math.floor(s / 60) + ":" + ("0" + (s % 60)).slice(-2)
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
            color: "#eaebed"
            Behavior on height { NumberAnimation { duration: 140; easing.type: Easing.OutExpo } }
            Rectangle {
                width: parent.width * bar.shown
                height: parent.height
                radius: height / 2
                color: center.c1
            }
            Rectangle {
                x: parent.width * bar.shown - 7
                anchors.verticalCenter: parent.verticalCenter
                width: 14
                height: 14
                radius: 7
                color: "#222222"
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
        function clamp(v) { return Math.max(0, Math.min(1, v)) }
    }

    Row {
        anchors { left: parent.left; leftMargin: 28; verticalCenter: parent.verticalCenter }
        spacing: 14
        QPicture {
            width: 56
            height: 56
            radius: 12
            source: root.cover
            sourceSize: Qt.size(64, 64)
        }
        Column {
            width: 186
            anchors.verticalCenter: parent.verticalCenter
            spacing: 3
            Text {
                width: parent.width
                text: root.title || "未在播放"
                font.pixelSize: 14
                font.weight: Font.DemiBold
                color: "#111111"
                elide: Text.ElideRight
            }
            Text {
                width: parent.width
                text: root.artist || "—"
                font.pixelSize: 12
                color: "#444444"
                elide: Text.ElideRight
            }
        }
    }

    Column {
        anchors { horizontalCenter: parent.horizontalCenter; verticalCenter: parent.verticalCenter }
        spacing: 4
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 6
            SButton {
                iconCharacter: ["\uf118", "\uf115", "\uf0e2", "\uf03b"][Options.settings.cycleIndex]
                width: 46
                height: 46
                radius: 23
                buttonColor: "transparent"
                iconColor: "#222222"
                shadowEnabled: false
                iconSize: 17
                onClicked: Options.settings.cycleIndex = (Options.settings.cycleIndex + 1) % 4
                tipText: "播放模式"
            }
            SButton {
                iconCharacter: "\uf0dc"
                width: 46
                height: 46
                radius: 23
                buttonColor: "transparent"
                iconColor: "#222222"
                iconSize: 18
                shadowEnabled: false
                onClicked: Playback.previous()
                tipText: "上一首"
            }
            SButton {
                iconCharacter: root.playing ? "\uf02f" : "\uf00e"
                width: 46
                height: 46
                radius: 23
                buttonColor: "#f2f5f9"
                iconColor: "#111214"
                shadowEnabled: false
                iconSize: 20
                onClicked: Playback.togglePlay()
                tipText: root.playing ? "暂停" : "播放"
            }
            SButton {
                iconCharacter: "\uf0d9"
                width: 46
                height: 46
                radius: 23
                buttonColor: "transparent"
                iconColor: "#222222"
                iconSize: 18
                shadowEnabled: false
                onClicked: Playback.next(false)
                tipText: "下一首"
            }
            SButton {
                iconCharacter: "\uf0e2"
                width: 46
                height: 46
                radius: 23
                buttonColor: "transparent"
                iconColor: "#222222"
                iconSize: 17
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
                text: Playback.fmt(root.player ? root.player.position : 0)
                //font.family: root.uiFont
                font.pixelSize: 12
                color: "#b9333333"
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
                text: Playback.fmt(root.player ? root.player.duration : 0)
                //font.family: root.uiFont
                font.pixelSize: 12
                color: "#b9333333"
            }
        }
    }

    Row {
        anchors { right: parent.right; rightMargin: 28; verticalCenter: parent.verticalCenter }
        spacing: 10
        SButton {
            iconCharacter: "\uf043"
            width: 40
            height: 40
            radius: 20
            buttonColor: "transparent"
            iconColor: Playback.muted ? "#8a99a8" : "#222222"
            shadowEnabled: false
            onClicked: Playback.toggleMute()
            tipText: "静音"
            WheelHandler {
                onWheel: e => {
                    Playback.stepVolume(e.angleDelta.y > 0 ? 0.05 : -0.05)
                    e.accepted = true
                }
            }
        }
        Bar {
            width: 92
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
            iconColor: "#222222"
            shadowEnabled: false
            onClicked: root.queueClicked()
            tipText: "播放队列"
        }
    }
}
