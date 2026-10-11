// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
import QtQuick
import Qt5Compat.GraphicalEffects
import QueMusic 1.0

Window {
    id: desktopLyricsWindow
    width: 640
    height: 160
    minimumWidth: 480
    minimumHeight: 120
    visible: true
    color: "transparent"
    title: "QueMusic 桌面歌词"
    transientParent: null
    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    property bool active: true   // 外部控制显示/隐藏
    property int lyricSize: height / 4 - 18

    // 宿主注入：播放引擎不再靠上下文继承访问宿主的局部 id
    readonly property AudioEngine player: Playback.player

    // 歌词数据引用
    property var lyricsData: MusicApi.lyricsData || []
    property int currentIndex: 0
    property int nextIndex: 1

    // 播放时下标只会前进，从上次位置增量推进，避免每 tick 全表扫描
    function updateCurrentIndex(): void {
        const pos = Playback.player.position || 0;
        const data = lyricsData;
        if (!data || data.length === 0) {
            currentIndex = -1;
            nextIndex = -1;
            return;
        }
        let idx = currentIndex;
        if (idx < 0 || idx >= data.length || data[idx].time > pos)
            idx = 0;   // 换歌或往回跳转才从头扫
        while (idx + 1 < data.length && pos >= data[idx + 1].time) idx++;
        currentIndex = idx;
        nextIndex = Math.min(idx + 1, data.length - 1);
    }

    Timer {
        interval: 240
        running: Playback.player.onMedia && desktopLyricsWindow.visible
        repeat: true
        onTriggered: desktopLyricsWindow.updateCurrentIndex()
    }

    Connections {
        target: MusicApi
        function onLyricsDataChanged(): void {
            desktopLyricsWindow.lyricsData = MusicApi.lyricsData || [];
            desktopLyricsWindow.updateCurrentIndex();
        }
    }

    Column {
        id: lyricsColumn
        y: 60
        width: parent.width
        height: parent.height - 60
        spacing: 8
        z: 5

        Text {
            id: currentLineText
            width: parent.width
            text: (desktopLyricsWindow.currentIndex >= 0 && desktopLyricsWindow.currentIndex < desktopLyricsWindow.lyricsData.length)
                  ? desktopLyricsWindow.lyricsData[desktopLyricsWindow.currentIndex].text || ""
                  : "🎵 纯音乐，请欣赏"
            font.pixelSize: desktopLyricsWindow.lyricSize * 1.2
            font.bold: true
            font.weight: Font.Medium
            color: Theme.themeColor
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignHCenter
            layer.enabled: true
            layer.effect: DropShadow {
                radius: 12.0
                samples: 16
                fast: true
                color: "#41000000"
            }
        }

        Text {
            id: nextLineText
            width: parent.width
            text: (desktopLyricsWindow.nextIndex >= 0 && desktopLyricsWindow.nextIndex < desktopLyricsWindow.lyricsData.length)
                  ? desktopLyricsWindow.lyricsData[desktopLyricsWindow.nextIndex].text || ""
                  : ""
            font.pixelSize: desktopLyricsWindow.lyricSize
            font.bold: true
            color: "#fcfcfc"
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignHCenter
            visible: text !== ""
            layer.enabled: true
            layer.effect: DropShadow {
                radius: 12.0
                samples: 16
                fast: true
                color: "#56000000"
            }
        }
    }

    property int bw: 3

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        z: 1
        cursorShape: {
            const p = Qt.point(mouseX, mouseY);
            const b = desktopLyricsWindow.bw + 10; // Increase the corner size slightly
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
                             const b = desktopLyricsWindow.bw + 10; // Increase the corner size slightly
                             let e = 0;
                             if (p.x < b) { e |= Qt.LeftEdge }
                             if (p.x >= desktopLyricsWindow.width - b) { e |= Qt.RightEdge }
                             if (p.y < b) { e |= Qt.TopEdge }
                             if (p.y >= desktopLyricsWindow.height - b) { e |= Qt.BottomEdge }
                             desktopLyricsWindow.startSystemResize(e);
                         }
    }


    Rectangle {
        opacity: desktopLyricArea.containsMouse ? 1 : 0
        anchors.fill: parent
        color: "#88000000"
        radius: Style.cubeRadius
        z: 2
        Behavior on opacity { NumberAnimation { duration: 120 } }
        Text {
            x: 20
            y: 12
            height: 36
            width: desktopLyricsWindow.width / 2 - 80
            elide: Text.ElideRight
            text: Playback.musicTitle + " - " + Playback.musicArtist
            verticalAlignment: Text.AlignVCenter
            color: "#fffafafa"
            font.pixelSize: Style.text
        }
        MouseArea {
            id: desktopLyricArea
            anchors.fill: parent
            anchors.margins: 6
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

            Row {
                y: 6
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 6

                SButton {
                    width: 36; height: 36; radius: Style.labelRadius
                    iconCharacter: "\uf0dc"
                    iconSize: 15
                    buttonColor: "transparent"
                    hoverColor: "#66fafafa"
                    iconColor: "#fffdfdfd"
                    shadowEnabled: false
                    onClicked: Playback.previous()
                    tipText: "上一首"
                }

                SButton {
                    width: 36; height: 36; radius: Style.labelRadius
                    iconCharacter: Playback.player.playing ? "\uf02f" : "\uf00e"
                    iconSize: 16
                    buttonColor: "transparent"
                    hoverColor: "#66fafafa"
                    iconColor: "#fffdfdfd"
                    shadowEnabled: false
                    onClicked: Playback.togglePlay()
                    tipText: Playback.player.playing ? "暂停" : "播放"
                }

                SButton {
                    width: 36; height: 36; radius: Style.labelRadius
                    iconCharacter: "\uf0d9"
                    iconSize: 15
                    buttonColor: "transparent"
                    hoverColor: "#66fafafa"
                    iconColor: "#fffdfdfd"
                    shadowEnabled: false
                    onClicked: Playback.next(false)
                    tipText: "下一首"
                }
            }
            Text {
                x: desktopLyricsWindow.width - 68 - width
                y: 6
                height: 36
                text: Playback.fmt(Playback.player.position) + " / " + Playback.fmt(Playback.player.duration)
                font.pixelSize: 13
                verticalAlignment: Text.AlignVCenter
                color: "#fffdfdfd"
            }
            SButton {
                id: closeButton
                x: desktopLyricsWindow.width - 54
                y: 6
                width: 36; height: 36; radius: Style.labelRadius
                iconCharacter: "\uf025"
                iconSize: 15
                buttonColor: "transparent"
                hoverColor: "#66fafafa"
                iconColor: "#fffdfdfd"
                shadowEnabled: false
                onClicked: {
                    Options.desktop.desktopPlayerMode = 0;
                    Options.desktopLyricsLoader.active = false;
                }
                tipText: "关闭"
            }
        }
    }

    // 窗口可见性跟随 active
    onActiveChanged: visible = active;

    Component.onCompleted: {
        updateCurrentIndex();
        active = true;
    }
}
