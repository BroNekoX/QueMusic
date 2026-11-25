// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
import QtQuick
import QtMultimedia
import QtQuick.Effects
import QtQuick.Templates as T
import QueMusic 1.0

Window {
    id: root
    width: 1140
    height: 720
    minimumWidth: 720
    minimumHeight: 480
    visible: true
    color: "#07070b"
    title: MusicApi.mvTitle.length > 0 ? "QueMusic-MV · " + MusicApi.mvTitle : "QueMusic-MV"

    property bool fullScreen: root.visibility === Window.FullScreen
    property bool controlsVisible: true
    property bool muted: false
    property bool musicPaused: false
    property real volume: 0.8

    readonly property bool hasUrl: MusicApi.mvUrl !== ""
    readonly property bool loading: MusicApi.mvLoading && !hasUrl
    readonly property bool playing: player.playbackState === MediaPlayer.PlayingState
    readonly property string errorText: player.errorString.length > 0
                                     ? player.errorString
                                     : MusicApi.mvError

    function togglePlay(): void {
        if (!hasUrl) {
            if (errorText.length > 0)
                MusicApi.retryMv()
            return
        }
        playing ? player.pause() : player.play()
    }
    function toggleFullScreen(): void {
        root.visibility = root.fullScreen ? Window.Windowed : Window.FullScreen
    }
    function showControls(): void {
        controlsVisible = true
        hideTimer.restart()
    }

    onClosing: {
        player.stop()
        Desktop.closeMv()
    }

    Connections {
        target: MusicApi
        function onMvReady(): void {
            root.show()
            root.requestActivate()
        }
    }

    MediaPlayer {
        id: player
        source: MusicApi.mvUrl
        audioOutput: AudioOutput {
            volume: root.muted ? 0 : root.volume
        }
        videoOutput: videoOut
        onSourceChanged: {
            root.musicPaused = false
            if (source !== "")
                play()
        }
        onPlaybackStateChanged: {
            if (playbackState !== MediaPlayer.PlayingState)
                return
            root.showControls()
            if (!root.musicPaused && Playback.inplay) {
                Playback.togglePlay()
                root.musicPaused = true
            }
        }
    }

    Image {
        anchors.fill: parent
        source: MusicApi.mvCover.length > 0
                ? MusicApi.mvCover
                : "qrc:/QueMusic/resources/app/musicpic.png"
        fillMode: Image.PreserveAspectCrop
        sourceSize.width: 640
        opacity: 0.4
        visible: !player.hasVideo
    }
    VideoOutput {
        id: videoOut
        anchors.fill: parent
        fillMode: VideoOutput.PreserveAspectFit
        visible: player.hasVideo
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        hoverEnabled: true
        onClicked: (mouse) => {
            root.showControls()
            if (mouse.button === Qt.LeftButton)
                root.togglePlay()
            else
                root.toggleFullScreen()
        }
        onDoubleClicked: root.toggleFullScreen()
        onPositionChanged: root.showControls()
    }

    Rectangle {
        width: parent.width
        height: 72
        z: 10
        color: "#b3000000"
        visible: root.controlsVisible

        Column {
            x: 24
            y: 17
            width: Math.max(80, tools.x - 58 - 12)
            Text {
                width: parent.width
                text: MusicApi.mvTitle.length > 0 ? MusicApi.mvTitle : "MV"
                color: "#f7f7fb"
                font.pixelSize: 15
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }
            Text {
                width: parent.width
                text: (MusicApi.mvArtist.length > 0 ? MusicApi.mvArtist : "未知歌手")
                      + (MusicApi.mvSourceName.length > 0 ? " · " + MusicApi.mvSourceName : "")
                color: "#b9bac4"
                font.pixelSize: 11
                elide: Text.ElideRight
            }
        }
        Row {
            id: tools
            anchors.right: parent.right
            anchors.rightMargin: 12
            y: 16
            spacing: 8
            QDrop {
                width: 120
                height: 34
                radius: 17
                cardRadius: 12
                color: "#22ffffff"
                textColor: "#f4f4f8"
                icon: "\uf0ac"
                model: MusicApi.sourceNames
                choice: Math.max(0, MusicApi.sourceIds.indexOf(MusicApi.mvSourceId))
                onTransformed: (choiced) => MusicApi.switchMvSource(MusicApi.sourceIdAt(choiced))
            }
            SButton {
                width: 34
                height: 34
                radius: 17
                iconCharacter: root.fullScreen ? "\uf066" : "\uf065"
                buttonColor: "#22ffffff"
                hoverColor: "#38ffffff"
                iconColor: "#f4f4f8"
                onClicked: root.toggleFullScreen()
            }
        }
    }

    Rectangle {
        anchors.centerIn: parent
        z: 8
        width: 76
        height: 76
        radius: 38
        color: "#66000000"
        border.width: 2
        border.color: "#99ffffff"
        visible: root.hasUrl && !root.playing && !root.loading && root.errorText.length === 0
        Text {
            anchors.centerIn: parent
            text: "\uf00e"
            color: "#ffffff"
            font.pixelSize: 30
            font.family: Fonts.icon
        }
        TapHandler {
            onTapped: root.togglePlay()
        }
    }

    Column {
        anchors.centerIn: parent
        z: 9
        spacing: 10
        visible: root.loading || root.errorText.length > 0
        AnimatedImage {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 40
            height: 40
            visible: root.loading
            source: "qrc:/QueMusic/resources/loader.gif"
            playing: root.loading
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: root.loading
            text: "正在解析 MV 地址…"
            color: "#e8e8ef"
            font.pixelSize: 13
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: root.errorText.length > 0
            width: Math.min(460, root.width - 80)
            text: root.errorText
            color: "#cfd0d8"
            font.pixelSize: 12
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
        }
        QButton {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: root.errorText.length > 0
            text: "重试"
            iconCharacter: "\uf021"
            height: 30
            radius: 15
            fontSize: 12
            iconSize: 11
            buttonColor: "#2b63d9"
            textColor: "#ffffff"
            iconColor: "#ffffff"
            shadowEnabled: false
            onClicked: MusicApi.retryMv()
        }
    }

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 110
        z: 10
        color: "#b3000000"
        visible: root.controlsVisible

        T.Slider {
            id: progress
            x: 16
            y: 8
            width: parent.width - 32
            height: 20
            padding: 0
            from: 0
            to: player.duration > 0 ? player.duration : 1
            value: player.position
            live: true
            enabled: root.hasUrl && player.duration > 0
            onMoved: player.setPosition(value)
            background: Rectangle {
                x: 0
                y: (progress.height - 4) / 2
                width: progress.availableWidth
                height: 4
                radius: 2
                color: "#33ffffff"
                Rectangle {
                    width: progress.visualPosition * parent.width
                    height: parent.height
                    radius: 2
                    color: "#ffffff"
                }
            }
            handle: Rectangle {
                x: progress.leftPadding + progress.visualPosition * (progress.availableWidth - width)
                y: progress.topPadding + progress.availableHeight / 2 - height / 2
                implicitWidth: 15
                implicitHeight: 15
                scale: progress.pressed ? 1.3 : 1
                radius: width / 2
                color: "#ffffff"
                Behavior on scale { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }

                RectangularShadow {
                    anchors.fill: parent
                    z: -1
                    offset.y: 1
                    radius: parent.width / 2
                    blur: 6
                    spread: 0
                    color: "#40000000"
                }
            }
        }
        Text {
            x: parent.width - width - 16
            y: 50
            height: 40
            verticalAlignment: Text.AlignVCenter
            text: Playback.fmt(player.position) + " / " + Playback.fmt(player.duration)
            color: "#a8a9b3"
            font.pixelSize: 13
        }
        Row {
            x: 16
            y: 50
            spacing: 8
            SButton {
                width: 40
                height: 40
                radius: 20
                buttonColor: "#f2f2f7"
                hoverColor: "#ffffff"
                iconColor: "#111116"
                iconSize: 15
                iconCharacter: root.playing ? "\uf02f" : "\uf00e"
                onClicked: root.togglePlay()
            }
            SButton {
                width: 40
                height: 40
                radius: 120
                buttonColor: "#22ffffff"
                hoverColor: "#38ffffff"
                iconColor: "#f4f4f8"
                iconCharacter: root.muted || root.volume <= 0.001 ? "\uf040" : "\uf043"
                onClicked: root.muted = !root.muted
            }
            T.Slider {
                id: volumeSlider
                y: 3
                width: 90
                height: 40
                padding: 0
                from: 0
                to: 1
                value: root.muted ? 0 : root.volume
                onMoved: {
                    root.volume = value
                    root.muted = value <= 0.001
                }
                background: Rectangle {
                    x: 0
                    y: (volumeSlider.height - 4) / 2
                    width: volumeSlider.availableWidth
                    height: 4
                    radius: 2
                    color: "#33ffffff"
                    Rectangle {
                        width: volumeSlider.visualPosition * parent.width
                        height: parent.height
                        radius: 2
                        color: "#f2f2f7"
                    }
                }
            }
        }
    }

    Timer {
        id: hideTimer
        interval: 2500
        onTriggered: if (root.playing) root.controlsVisible = false
    }

    Shortcut {
        sequence: "Space"
        onActivated: root.togglePlay()
    }
    Shortcut {
        sequence: "F11"
        onActivated: root.toggleFullScreen()
    }
    Shortcut {
        sequence: "Esc"
        enabled: root.fullScreen
        onActivated: root.toggleFullScreen()
    }

    Component.onCompleted: {
        hideTimer.start()
        show()
        requestActivate()
    }
}
