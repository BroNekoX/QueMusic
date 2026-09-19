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
    minimumWidth: 240
    minimumHeight: 128
    visible: true
    color: "transparent"
    title: "QueMusic桌面播放器"
    transientParent: null
    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    property bool topWindow: true
    property int bw: 3

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        z: 1
        cursorShape: {
            const p = Qt.point(mouseX, mouseY);
            const b = bw + 10; // Increase the corner size slightly
            if (p.x < b && p.y < b) return Qt.SizeFDiagCursor;
            if (p.x >= width - b && p.y >= height - b) return Qt.SizeFDiagCursor;
            if (p.x >= width - b && p.y < b) return Qt.SizeBDiagCursor;
            if (p.x < b && p.y >= height - b) return Qt.SizeBDiagCursor;
            if (p.x < b || p.x >= width - b) return Qt.SizeHorCursor;
            if (p.y < b || p.y >= height - b) return Qt.SizeVerCursor;
        }
        acceptedButtons: Qt.NoButton // don't handle actual events
    }

    DragHandler {
        id: resizeHandler
        grabPermissions: TapHandler.TakeOverForbidden
        target: null
        onActiveChanged: if (active) {
                             const p = resizeHandler.centroid.position;
                             const b = bw + 10; // Increase the corner size slightly
                             let e = 0;
                             if (p.x < b) { e |= Qt.LeftEdge }
                             if (p.x >= width - b) { e |= Qt.RightEdge }
                             if (p.y < b) { e |= Qt.TopEdge }
                             if (p.y >= height - b) { e |= Qt.BottomEdge }
                             desktopPlayerWindow.startSystemResize(e);
                         }
    }

    MouseArea {
        anchors.fill: parent
        anchors.margins: 6
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

    // 时间格式化
    function formatTime(ms: real): string {
        if (isNaN(ms) || ms < 0) return "00:00";
        const totalSeconds = Math.floor(ms / 1000);
        const minutes = Math.floor(totalSeconds / 60);
        const seconds = totalSeconds % 60;
        return (minutes < 10 ? "0" : "") + minutes + ":" + (seconds < 10 ? "0" : "") + seconds;
    }

    // 卡片主体
    Rectangle {
        id: playerCard
        anchors.fill: parent
        radius: 12
        z: 1
        color: Style.themes.primaryColor
        border.width: 2
        border.color: Style.themes.sideColor

        // 封面
        Image {
            x: 16
            y: 16
            width: 64
            height: 64
            source: mainMedia.urlStr || "qrc:/QueMusic/resources/app/musicpic.png"
            sourceSize: Qt.size(128, 128)
            fillMode: Image.PreserveAspectCrop
        }

        // 标题
        Text {
            id: playerTitle
            x: 102
            y: 16
            width: playerCard.width - 102 - 48
            height: 22
            text: window.musicTitle
            elide: Text.ElideRight
            font.bold: true
            font.pixelSize: 14
            verticalAlignment: Text.AlignVCenter
            color: Style.themes.fontColor
        }

        // 艺术家
        Text {
            id: playerArtist
            x: 102
            y: 38
            width: playerCard.width - 102 - 48
            height: 18
            text: window.musicArtist
            elide: Text.ElideRight
            font.pixelSize: 12
            verticalAlignment: Text.AlignVCenter
            color: Style.themes.textColor
        }

        // 进度条
        Slider {
            id: seekSlider
            x: 102
            y: 56
            width: playerCard.width - 102 - 14
            height: 16
            from: 0
            to: mainMedia.duration > 0 ? mainMedia.duration : 1
            value: pressed ? null : mainMedia.position
            live: true
            onMoved: mainMedia.position = value
            padding: 0
            background: Rectangle {
                y: (seekSlider.height - 4) / 2
                width: seekSlider.availableWidth
                height: 4
                radius: 2
                color: Style.themes.secondaryColor
                Rectangle {
                    width: seekSlider.visualPosition * parent.width
                    height: parent.height
                    radius: 2
                    color: Style.themes.themeColor
                }
            }
            handle: Rectangle {
                x: seekSlider.leftPadding + seekSlider.visualPosition * (seekSlider.availableWidth - width)
                y: (seekSlider.height - 12) / 2
                width: 12
                height: 12
                radius: 6
                color: Style.themes.primaryColor
                border.width: 2
                border.color: Style.themes.themeColor
                visible: seekSlider.hovered || seekSlider.pressed
            }
        }

        // 时间
        Text {
            id: timeText
            x: 102
            y: 72
            width: playerCard.width - 102 - 14
            height: 16
            text: desktopPlayerWindow.formatTime(mainMedia.position) + " / " + desktopPlayerWindow.formatTime(mainMedia.duration)
            font.pixelSize: 11
            color: Style.themes.textColor
            horizontalAlignment: Text.AlignRight
        }

        Row {
            y: parent.height - 52
            anchors.horizontalCenter: parent.horizontalCenter
            SButton {
                iconCharacter: ["\uf118","\uf115","\uf0e2","\uf03b"][Options.settings.cycleIndex]
                width: 36
                height: 36
                radius: 18
                buttonColor: "transparent"
                hoverColor: Style.themes.hoverColor
                iconColor: Style.themes.textColor
                shadowEnabled: false
                iconSize: Style.settings.texticon
                onClicked: {
                    if(Options.settings.cycleIndex < 3) {
                        Options.settings.cycleIndex += 1;
                    } else {
                        Options.settings.cycleIndex = 0;
                    }
                }
                tipText: "播放顺序"
            }

            SButton {
                id: lastButton
                width: 36
                height: 36
                radius: 18
                iconCharacter: "\uf0dc"
                iconSize: Style.settings.texticon
                buttonColor: "transparent"
                hoverColor: Style.themes.hoverColor
                iconColor: Style.themes.textColor
                shadowEnabled: false
                onClicked: musicControlMin.lastMedia()
                tipText: "上一首"
            }
            SButton {
                id: playButton
                width: 36
                height: 36
                radius: 18
                iconCharacter: mainMedia.playing ? "\uf02f" : "\uf00e"
                iconSize: Style.settings.texticon + 2
                buttonColor: Style.themes.secondaryBlurColor
                hoverColor: Style.themes.hoverColor
                iconColor: Style.themes.textColor
                shadowEnabled: false
                onClicked: Playback.togglePlay()
                tipText: mainMedia.playing ? "暂停" : "播放"
            }
            SButton {
                id: nextButton
                width: 36
                height: 36
                radius: 18
                iconCharacter: "\uf0d9"
                iconSize: Style.settings.texticon
                buttonColor: "transparent"
                hoverColor: Style.themes.hoverColor
                iconColor: Style.themes.textColor
                shadowEnabled: false
                onClicked: musicControlMin.enterMedia()
                tipText: "下一首"
            }
            SButton {
                id: playListButton
                width: 36
                height: 36
                radius: 17
                iconCharacter: desktopPlayerWindow.topWindow ? "\uf003" : "\uf05c"
                iconSize: Style.settings.texticon
                buttonColor: "transparent"
                hoverColor: Style.themes.hoverColor
                iconColor: Style.themes.textColor
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

        // 关闭按钮
        SButton {
            id: closeButton
            x: playerCard.width - 44
            y: 8
            width: 32
            height: 32
            radius: 16
            iconCharacter: "\uf025"
            iconSize: Style.settings.texticon
            buttonColor: "transparent"
            hoverColor: Qt.rgba(1.0, 0.4, 0.4, 0.4)
            iconColor: Style.themes.textColor
            shadowEnabled: false
            onClicked: {
                desktopPlayer.desktopPlayerMode = 0;
                desktopSpot.active = false;
                desktopPlayerLoader.active = false;
            }

            tipText: "关闭"
        }
    }
}
