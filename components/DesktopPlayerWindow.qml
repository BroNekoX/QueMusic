// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
import QtQuick
import QueMusic 1.0
import QtQuick.Controls.Basic
import QtQuick.Effects

Window {
    id: desktopPlayerWindow
    width: 340
    height: 148
    minimumWidth: 320
    minimumHeight: 72
    maximumWidth: 480
    maximumHeight: 160
    visible: true
    color: "transparent"
    title: "QueMusic桌面播放器"
    transientParent: null
    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    property bool topWindow: true
    property int bw: 0
    readonly property bool isSmall: height < 132

    // 宿主注入：播放引擎不再靠上下文继承访问宿主的局部 id
    readonly property AudioEngine player: Playback.player
    onClosing: {
        Options.desktop.desktopPlayerMode = 0;
        Options.desktopPlayerLoader.active = false;
    }

    // 右侧缩放
    MouseArea {
        anchors.right: parent.right
        height: parent.height - 10
        width: 10
        z: 1000
        cursorShape: Qt.SizeHorCursor
        onPressed: {
            desktopPlayerWindow.startSystemResize(Qt.RightEdge)
        }
    }

    // 底边缩放
    MouseArea {
        width: parent.width - 10
        anchors.bottom: parent.bottom
        height: 10
        z: 1000
        cursorShape: Qt.SizeVerCursor
        onPressed: {
            desktopPlayerWindow.startSystemResize(Qt.BottomEdge)
        }
    }

    // 右下角斜向缩放
    MouseArea {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        width: 10
        height: 10
        z: 1000
        cursorShape: Qt.SizeFDiagCursor
        onPressed: {
            desktopPlayerWindow.startSystemResize(Qt.RightEdge | Qt.BottomEdge)
        }
    }

    MouseArea {
        anchors.fill: parent
        anchors.margins: 10
        z: 0
        property real dragOffsetX: 0
        property real dragOffsetY: 0
        onPressed: (mouse) => {
            dragOffsetX = mouse.x;
            dragOffsetY = mouse.y;
        }
        onPositionChanged: (mouse) => {
            if (pressed) {
                desktopPlayerWindow.x = desktopPlayerWindow.x + (mouse.x - dragOffsetX);
                desktopPlayerWindow.y = desktopPlayerWindow.y + (mouse.y - dragOffsetY);
            }
        }
    }

    Rectangle {
        id: playerCard
        anchors.fill: parent
        radius: 12
        z: 1
        color: Theme.primaryColor
        border.width: 1
        border.color: Theme.sideColor

        QPicture {
            id: wcover
            x: 16
            y: desktopPlayerWindow.isSmall ? (parent.height - height) * 0.5 : 16
            width: desktopPlayerWindow.isSmall ? 52 : 64
            height: desktopPlayerWindow.isSmall ? 52 : 64
            radius: 8
            source: Playback.player.urlStr || "qrc:/QueMusic/resources/app/musicpic.png"
            sourceSize: Qt.size(64, 64)
        }

        Text {
            id: playerTitle
            x: desktopPlayerWindow.isSmall ? 84 : 96
            y: wcover.y
            width: desktopPlayerWindow.isSmall ? desktopPlayerWindow.width - 128 - dpControlRow.width : desktopPlayerWindow.width - 112
            height: 18
            text: Playback.musicTitle
            elide: Text.ElideRight
            font.bold: true
            font.pixelSize: 14
            verticalAlignment: Text.AlignVCenter
            color: Theme.fontColor
        }

        Text {
            id: playerArtist
            x: desktopPlayerWindow.isSmall ? 84 : 96
            y: wcover.y + 18
            width: desktopPlayerWindow.isSmall ? desktopPlayerWindow.width - 128 - dpControlRow.width : desktopPlayerWindow.width - 112
            height: 16
            text: Playback.musicArtist
            elide: Text.ElideRight
            font.pixelSize: 12
            verticalAlignment: Text.AlignVCenter
            color: Theme.textColor
        }

        Slider {
            id: seekSlider
            x: desktopPlayerWindow.isSmall ? timeText.width + 100 : 96
            y: wcover.y + 35
            width: desktopPlayerWindow.isSmall ? playerCard.width - 128 - timeText.width - timeFullText.width : playerCard.width - 112//96 - 16
            height: 16
            from: 0
            to: Playback.player.duration > 0 ? Playback.player.duration : 1
            value: pressed ? null : Playback.player.position
            live: true
            onMoved: Playback.player.position = value
            padding: 0
            background: Rectangle {
                y: (seekSlider.height - 4) / 2
                width: seekSlider.availableWidth
                height: 4
                radius: 2
                color: Theme.secondaryColor
                Rectangle {
                    width: seekSlider.visualPosition * parent.width
                    height: parent.height
                    radius: 2
                    color: Theme.themeColor
                }
            }
            handle: Rectangle {
                x: seekSlider.leftPadding + seekSlider.visualPosition * (seekSlider.availableWidth - width)
                y: (seekSlider.height - 12) / 2
                width: 12
                height: 12
                radius: 6
                color: Theme.themeColor
                border.width: 3
                border.color: Theme.primaryColor
                RectangularShadow {
                    anchors.fill: parent
                    z: -1
                    radius: 10
                    blur: 5
                    spread: 0
                    color: "#33000000"
                }
            }
        }

        Text {
            id: timeText
            x: wcover.width + 32
            y: desktopPlayerWindow.isSmall ? wcover.y + 35 : 70
            height: 16
            // position 20Hz 刷新：量化到秒
            readonly property int posSec: Math.floor(Playback.player.position / 1000)
            text: Playback.fmt(posSec * 1000)
            font.pixelSize: 11
            color: Theme.textColor
        }
        Text {
            id: timeFullText
            x: desktopPlayerWindow.width - width - 16
            y: desktopPlayerWindow.isSmall ? wcover.y + 35 : 70
            height: 16
            font.pixelSize: 11
            color: Theme.textColor
            text: Playback.fmt(Playback.player.duration)
        }

        Row {
            id: dpControlRow
            y: desktopPlayerWindow.isSmall ? wcover.y + 2 : parent.height - 52
            x: desktopPlayerWindow.isSmall ? desktopPlayerWindow.width - width - 44 : (desktopPlayerWindow.width - width) / 2
            height: desktopPlayerWindow.isSmall ? 28 : 36
            SButton {
                iconCharacter: Playback.cycleIcon
                width: parent.height
                height: parent.height
                radius: 18
                buttonColor: "transparent"
                hoverColor: Theme.hoverColor
                iconColor: Theme.textColor
                shadowEnabled: false
                iconSize: Style.texticon - 2
                onClicked: {
                    if(Config.cycleIndex < 3) {
                        Config.cycleIndex += 1;
                    } else {
                        Config.cycleIndex = 0;
                    }
                }
                tipText: "播放顺序"
            }

            SButton {
                id: lastButton
                width: parent.height
                height: parent.height
                radius: 18
                iconCharacter: "\uf0dc"
                iconSize: Style.texticon
                buttonColor: "transparent"
                hoverColor: Theme.hoverColor
                iconColor: Theme.textColor
                shadowEnabled: false
                onClicked: Playback.previous()
                tipText: "上一首"
            }
            SButton {
                id: playButton
                width: parent.height
                height: parent.height
                radius: 18
                iconCharacter: Playback.player.playing ? "\uf02f" : "\uf00e"
                iconSize: Style.texticon + 2
                buttonColor: Theme.secondaryBlurColor
                hoverColor: Theme.hoverColor
                iconColor: Theme.textColor
                shadowEnabled: false
                onClicked: Playback.togglePlay()
                tipText: Playback.player.playing ? "暂停" : "播放"
            }
            SButton {
                id: nextButton
                width: parent.height
                height: parent.height
                radius: 18
                iconCharacter: "\uf0d9"
                iconSize: Style.texticon
                buttonColor: "transparent"
                hoverColor: Theme.hoverColor
                iconColor: Theme.textColor
                shadowEnabled: false
                onClicked: Playback.next(false)
                tipText: "下一首"
            }
            SButton {
                id: playListButton
                width: parent.height
                height: parent.height
                radius: 17
                iconCharacter: desktopPlayerWindow.topWindow ? "\uf003" : "\uf05c"
                iconSize: Style.texticon - 2
                buttonColor: "transparent"
                hoverColor: Theme.hoverColor
                iconColor: Theme.textColor
                shadowEnabled: false
                onClicked: {
                    if(desktopPlayerWindow.topWindow) {
                        desktopPlayerWindow.flags = Qt.Window | Qt.FramelessWindowHint;
                        desktopPlayerWindow.topWindow = false;
                    } else {
                        desktopPlayerWindow.flags = Qt.Window | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint;
                        desktopPlayerWindow.topWindow = true;
                    }
                }
                tipText: "顶置小窗"
            }
        }

        SButton {
            id: closeButton
            x: playerCard.width - 44
            y: desktopPlayerWindow.isSmall ? wcover.y + 2 : 12
            width: desktopPlayerWindow.isSmall ? 28 : 32
            height: width
            radius: 16
            iconCharacter: "\uf025"
            iconSize: Style.texticon
            buttonColor: "transparent"
            hoverColor: Qt.rgba(1.0, 0.4, 0.4, 0.4)
            iconColor: Theme.textColor
            shadowEnabled: false
            onClicked: {
                Options.desktop.desktopPlayerMode = 0;
                Options.desktopPlayerLoader.active = false;
            }

            tipText: "关闭"
        }
    }
}