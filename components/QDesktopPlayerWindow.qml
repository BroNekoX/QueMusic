// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024-2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects

// 桌面小窗播放器
// 由 main.qml 中的 desktopPlayerLoader (Loader) 动态加载，模式切换时才创建实例
Window {
    id: desktopPlayerWindow
    width: 340
    height: 148
    x: Screen.width - width - 60
    y: 90
    visible: true
    color: "transparent"
    title: "QueMusic桌面播放器"
    // 关键：置空 transientParent，避免 QML 自动建立依赖关系，
    // 否则主窗口最小化/隐藏时小窗会被系统一起隐藏；同时置顶常驻桌面。
    transientParent: null
    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    property bool topWindow: true

    // 时间格式化
    function formatTime(ms) {
        if (isNaN(ms) || ms < 0) return "00:00";
        var totalSeconds = Math.floor(ms / 1000);
        var minutes = Math.floor(totalSeconds / 60);
        var seconds = totalSeconds % 60;
        return (minutes < 10 ? "0" : "") + minutes + ":" + (seconds < 10 ? "0" : "") + seconds;
    }

    // 卡片主体
    Rectangle {
        id: playerCard
        anchors.fill: parent
        radius: 16
        color: Style.themes.primaryBlurColor
        border.width: 2
        border.color: Style.themes.sideColor

        // 拖动区域（最底层，按钮在上层可正常点击）
        MouseArea {
            anchors.fill: parent
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

        // 封面
        Image {
            x: 16
            y: 16
            width: 64
            height: 64
            source: mainMedia.urlStr || "qrc:/QueMusic/resources/app/musicpic.png"
            sourceSize: Qt.size(128, 128)
            fillMode: Image.PreserveAspectCrop
            cache: false
        }

        // 标题
        Text {
            id: playerTitle
            x: 102
            y: 12
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
            y: 34
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
            y: 55
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

        SButton {
            iconCharacter: ["\uf118","\uf115","\uf0e2","\uf03b"][musicControlMin.cycleIndex]
            x: 66
            y: 95
            width: 36
            height: 36
            radius: 18
            buttonColor: "transparent"
            hoverColor: Style.themes.hoverColor
            iconColor: Style.themes.textColor
            shadowEnabled: false
            iconSize: Style.settings.texticon
            onClicked: {
                if(musicControlMin.cycleIndex < 3) {
                    musicControlMin.cycleIndex += 1;
                } else {
                    musicControlMin.cycleIndex = 0;
                }
            }
            QTip {
                visible: parent.hovered
                text: "播放顺序"
            }
        }

        SButton {
            id: lastButton
            x: 108
            y: 95
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
            QTip {
                visible: parent.hovered
                text: "上一首"
            }
        }
        SButton {
            id: playButton
            x: 150
            y: 93
            width: 40
            height: 40
            radius: 20
            iconCharacter: mainMedia.playing ? "\uf02f" : "\uf00e"
            iconSize: Style.settings.texticonH
            buttonColor: Style.themes.secondaryBlurColor
            hoverColor: Style.themes.hoverColor
            iconColor: Style.themes.textColor
            shadowEnabled: false
            onClicked: {
                if (mainMedia.playing) {
                    mainMedia.pause();
                } else {
                    mainMedia.play();
                }
            }
            QTip {
                visible: parent.hovered
                text: mainMedia.playing ? "暂停" : "播放"
            }
        }
        SButton {
            id: nextButton
            x: 196
            y: 95
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
            QTip {
                visible: parent.hovered
                text: "下一首"
            }
        }
        SButton {
            id: playListButton
            x: 238
            y: 96
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
            QTip {
                visible: parent.hovered
                text: "顶置小窗"
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
            iconSize: 14
            buttonColor: "transparent"
            hoverColor: Qt.rgba(1.0, 0.4, 0.4, 0.4)
            iconColor: Style.themes.textColor
            shadowEnabled: false
            onClicked: {
                desktopSpot.active = false;
                desktopPlayerLoader.active = false;
            }

            QTip {
                visible: parent.hovered
                text: "关闭"
            }
        }
    }
}
