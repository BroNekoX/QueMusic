// DesktopLyricsWindow.qml
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
import QtQuick
import Qt5Compat.GraphicalEffects
import QueMusic 1.0

Window {
    id: desktopLyricsWindow
    width: desktopLyricsLoader.lyricSize * 32
    height: desktopLyricsLoader.lyricSize * 5 + 52
    x: Screen.width / 2 - width / 2
    y: Screen.height - height - 100
    visible: true
    color: "transparent"
    title: "QueMusic 桌面歌词"
    transientParent: null
    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    property bool active: true   // 外部控制显示/隐藏

    // 歌词数据引用
    property var lyricsData: MusicApi.lyricsData || []
    property int currentIndex: 0
    property int nextIndex: 1

    // 时间格式化（复用）
    function formatTime(ms) {
        if (isNaN(ms) || ms < 0) return "00:00";
        var totalSeconds = Math.floor(ms / 1000);
        var minutes = Math.floor(totalSeconds / 60);
        var seconds = totalSeconds % 60;
        return (minutes < 10 ? "0" : "") + minutes + ":" + (seconds < 10 ? "0" : "") + seconds;
    }

    // 更新歌词索引
    function updateCurrentIndex() {
        var pos = mainMedia.position || 0;
        var data = lyricsData;
        if (!data || data.length === 0) {
            currentIndex = -1;
            nextIndex = -1;
            return;
        }
        var idx = 0;
        while (idx + 1 < data.length && pos >= data[idx + 1].time) idx++;
        currentIndex = idx;
        nextIndex = Math.min(idx + 1, data.length - 1);
    }

    // 定时更新
    Timer {
        interval: 240
        running: mainMedia.onMedia && desktopLyricsWindow.visible
        repeat: true
        onTriggered: updateCurrentIndex()
    }

    // 歌词数据变化时重置
    Connections {
        target: MusicApi
        function onLyricsDataChanged() {
            lyricsData = MusicApi.lyricsData || [];
            updateCurrentIndex();
        }
    }

    // 歌词文本容器（两行）
    Column {
        id: lyricsColumn
        y: 60
        width: parent.width
        height: parent.height - 60
        spacing: 8
        z: 5
        //visible: false
        //layer.enabled: true

        Text {
            id: currentLineText
            width: parent.width
            text: (desktopLyricsWindow.currentIndex >= 0 && desktopLyricsWindow.currentIndex < lyricsData.length)
                  ? lyricsData[desktopLyricsWindow.currentIndex].text || ""
                  : "🎵 纯音乐，请欣赏"
            font.pixelSize: desktopLyricsLoader.lyricSize * 1.2
            font.bold: true
            font.weight: Font.Medium
            color: Style.themes.themeColor
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignHCenter
            //opacity: 0.95
            layer.enabled: true
            layer.effect: DropShadow {
                radius: 12.0
                samples: 16
                //z: -1
                fast: true
                color: "#41000000"
                //source: currentLineText // 阴影绑定到主内容区域
            }
        }

        Text {
            id: nextLineText
            width: parent.width
            text: (desktopLyricsWindow.nextIndex >= 0 && desktopLyricsWindow.nextIndex < lyricsData.length)
                  ? lyricsData[desktopLyricsWindow.nextIndex].text || ""
                  : ""
            font.pixelSize: desktopLyricsLoader.lyricSize
            font.bold: true
            color: "#fcfcfc"
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignHCenter
            //opacity: 0.8
            visible: text !== ""
            layer.enabled: true
            layer.effect: DropShadow {
                radius: 12.0
                samples: 16
                fast: true
                color: "#56000000"
                //source: nextLineText // 阴影绑定到主内容区域
            }
        }
    }


    // 拖动区域
    MouseArea {
        anchors.fill: parent
        z: 2
        property real dragOffsetX: 0
        property real dragOffsetY: 0
        onPressed: (mouse) => {
            dragOffsetX = mouse.x;
            dragOffsetY = mouse.y;
        }
        onPositionChanged: (mouse) => {
            if (pressed) {
                desktopLyricsWindow.x += mouse.x - dragOffsetX;
                desktopLyricsWindow.y += mouse.y - dragOffsetY;
            }
        }
        hoverEnabled: true
        //onEntered: {
        //    controlPanel.opacity = 1;
        //}
        //onExited: hideTimer.start();
        Rectangle {
            opacity: parent.containsMouse ? 1 : 0
            anchors.fill: parent
            color: "#88000000"
            radius: Style.settings.cubeRadius
            Behavior on opacity { NumberAnimation { duration: 120 } }
            Text {
                x: 16
                y: 12
                height: 36
                width: desktopLyricsWindow.width / 2 - 80
                text: window.musicTitle + " - " + window.musicArtist
                verticalAlignment: Text.AlignVCenter
                color: "#fffafafa"
                font.pixelSize: Style.settings.text
            }
            // 控制按钮组
            Row {
                y: 12
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 6

                // 上一首
                SButton {
                    width: 36; height: 36; radius: Style.settings.labelRadius
                    iconCharacter: "\uf0dc"
                    iconSize: 15
                    buttonColor: "transparent"
                    hoverColor: "#66fafafa"
                    iconColor: "#fffdfdfd"
                    shadowEnabled: false
                    onClicked: musicControlMin.lastMedia()
                    QTip { visible: parent.hovered; text: "上一首" }
                }

                // 播放/暂停
                SButton {
                    width: 36; height: 36; radius: Style.settings.labelRadius
                    iconCharacter: mainMedia.playing ? "\uf02f" : "\uf00e"
                    iconSize: 16
                    buttonColor: "transparent"
                    hoverColor: "#66fafafa"
                    iconColor: "#fffdfdfd"
                    shadowEnabled: false
                    onClicked: Playback.togglePlay()
                    QTip { visible: parent.hovered; text: mainMedia.playing ? "暂停" : "播放" }
                }

                // 下一首
                SButton {
                    width: 36; height: 36; radius: Style.settings.labelRadius
                    iconCharacter: "\uf0d9"
                    iconSize: 15
                    buttonColor: "transparent"
                    hoverColor: "#66fafafa"
                    iconColor: "#fffdfdfd"
                    shadowEnabled: false
                    onClicked: musicControlMin.enterMedia()
                    QTip { visible: parent.hovered; text: "下一首" }
                }
            }
            // 进度条小提示（时间）
            Text {
                x: desktopLyricsWindow.width - 60 - width
                y: 12
                height: 36
                text: desktopLyricsWindow.formatTime(mainMedia.position) + " / " + desktopLyricsWindow.formatTime(mainMedia.duration)
                font.pixelSize: 13
                verticalAlignment: Text.AlignVCenter
                color: "#fffdfdfd"
            }
            // 关闭按钮
            SButton {
                id: closeButton
                x: desktopLyricsWindow.width - 48
                y: 12
                width: 36; height: 36; radius: Style.settings.labelRadius
                iconCharacter: "\uf025"
                iconSize: 15
                buttonColor: "transparent"
                hoverColor: "#66fafafa"
                iconColor: "#fffdfdfd"
                shadowEnabled: false
                onClicked: {
                    desktopPlayer.desktopPlayerMode = 0;
                    desktopLyricsLoader.active = false;
                }

                QTip {
                    visible: parent.hovered
                    text: "关闭"
                }
            }
        }
    }

    // 窗口可见性跟随 active
    onActiveChanged: visible = active;

    Component.onCompleted: {
        updateCurrentIndex();
        // 默认显示
        active = true;
    }
}