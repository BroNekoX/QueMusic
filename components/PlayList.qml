// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
import QtQuick
import QueMusic 1.0
import QtQuick.Controls.Basic

// 播放列表
Popup {
    id: playList
    // 列表中： source：-1：本地 0.酷狗 1.网易云 2.哔哩哔哩 3.qq音乐
    property alias model: playListView.model
    property string filter: ""

    padding: 0
    margins: -1
    parent: Overlay.overlay
    width: 360
    height: parent.height - 180
    x: parent.width - 380
    y: 80
    background: QBlurCard {
        anchors.fill: parent
        borderRadius: Style.cubeRadius
        clip: false
        blurSource: Options.mainLayout
        shadowEffect: true
        rectXy: Qt.rect(playList.x, playList.y, 360, playList.height)
    }

    function locateCurrent(): void {
        const i = Options.queue.playListIndex;
        if (i < 0) return;
        playListView.positionViewAtIndex(i, ListView.Center);
    }

    contentItem: Item {
        anchors.fill: parent

        Text {
            y: 12
            x: 18
            height: 36
            text: "播放列表"
            font.bold: true
            font.pixelSize: Style.textH2
            verticalAlignment: Text.AlignVCenter
            color: Theme.fontColor
            Rectangle {
                y: 6
                x: parent.width + 8
                height: 24
                radius: 8
                color: Theme.themeColor
                width: songCountTag.width + 16
                Text {
                    id: songCountTag
                    anchors.centerIn: parent
                    font.bold: true
                    font.pixelSize: Style.textmain
                    text: Options.queue.count + "首"
                    color: Theme.primaryColor
                }
            }
        }

        SButton {
            iconCharacter: "\uf092"
            x: parent.width - 130
            y: 12
            width: 36
            height: 36
            radius: 18
            buttonColor: "transparent"
            tipText: "刷新与定位"
            shadowEnabled: false
            onClicked: {
                playListView.model = [];
                playListView.model = Options.queue;
                playList.locateCurrent();
                playListView.scrollToY = playListView.contentY;
            }
        }

        SButton {
            iconCharacter: "\uf08e"
            x: parent.width - 90
            y: 12
            width: 36
            height: 36
            radius: 18
            buttonColor: "transparent"
            tipText: "清空"
            hoverColor: Qt.rgba(1.0,0.5,0.5,0.5)
            shadowEnabled: false
            onClicked: {
                Options.dialog.openSimpleDialog("删除", "这将移除播放列表其他歌曲，是否继续？",
                    function() {
                        // 空队列 / 无当前曲时下标为 -1：那就只清空，不留一条空条目
                        const i = Options.queue.playListIndex;
                        const cur = (i >= 0 && i < Options.queue.count) ? Options.queue.get(i) : null;
                        Options.queue.remove( 0, Options.queue.count );
                        if (cur) {
                            Options.queue.append({ name: cur.name, path: cur.path, songer: cur.songer, source: cur.source });
                            Options.queue.playListIndex = 0;
                        } else {
                            Options.queue.playListIndex = -1;
                        }
                        Options.warned("已清空播放列表",1);
                    }
                );
            }
        }
        SButton {
            iconCharacter: "\uf025"
            x: parent.width - 50
            y: 12
            width: 36
            height: 36
            radius: 18
            iconSize: Style.texticon + 2
            buttonColor: "transparent"
            shadowEnabled: false
            onClicked: {
                playList.close();
            }
        }

        ListView {
            id: playListView
            x: 12
            y: 60
            z: 2
            width: 348
            height: parent.height - 60
            model: Options.queue
            spacing: 0
            orientation: Qt.Vertical
            clip: true
            topMargin: 4
            rightMargin: 12
            bottomMargin: 12
            property int scrollToY: playListView.contentY
            reuseItems: true
            move: Transition { NumberAnimation { properties: "y"; duration: 200; easing.type: Easing.OutCubic } }
            moveDisplaced: Transition { NumberAnimation { properties: "y"; duration: 200; easing.type: Easing.OutCubic } }

            ScrollBar.vertical: ScrollBar {
                parent: playListView
                anchors.top: playListView.top
                anchors.left: playListView.right
                anchors.bottom: playListView.bottom
                onPressedChanged: playListView.scrollToY = playListView.contentY
            }
            WheelHandler {
                property real scrollMultiplier: Qt.application.styleHints.wheelScrollLines / 4
                onWheel: (event) => {
                    playListView.scrollToY = Math.max(-10, Math.min(playListView.scrollToY - (event.angleDelta.y * scrollMultiplier), playListView.contentHeight - playListView.height + 10))
                    listViewAnime.running = false
                    listViewAnime.running = true
                    event.accepted = true
                }
            }
            NumberAnimation {
                id: listViewAnime
                target: playListView
                property: "contentY"
                duration: 240
                to: playListView.scrollToY
                easing.type: Easing.OutCubic
            }

            delegate: Rectangle {
                id: listfile
                required property int index
                required property string name
                required property string songer
                required property string path
                required property int source
                // 复用时清掉上一行残留的悬停态
                readonly property string songName: listfile.name || ""
                readonly property string songArtist: listfile.songer || ""
                readonly property bool isCurrent: Options.queue.playListIndex === listfile.index
                height: 60
                width: ListView.view.width - 12
                radius: Style.labelRadius
                color: isCurrent ? Theme.containColor : "transparent"

                Text {
                    y: 10
                    x: 8
                    text: listfile.index + 1
                    z: 5
                    width: 40
                    height: 40
                    color: isCurrent ? Theme.themeColor : Theme.textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                Text {
                    x: 50
                    y: songArtist ? 10 : 20
                    z: 4
                    width: 200
                    height: 20
                    text: songName
                    elide: Text.ElideRight
                    color: Theme.fontColor
                    font.pixelSize: Style.text
                    verticalAlignment: Text.AlignVCenter
                }
                Text {
                    x: 50
                    y: 30
                    z: 4
                    width: 200
                    height: 20
                    text: songArtist
                    color: Theme.textColor
                    elide: Text.ElideRight
                    font.pixelSize: Style.textTip
                    verticalAlignment: Text.AlignVCenter
                    visible: songArtist !== ""
                }

                Row {
                    anchors.right: parent.right
                    anchors.rightMargin: 10
                    opacity: playListArea.containsMouse || playDelete.hovered ? 0 : 1
                    Behavior on opacity { NumberAnimation { duration: 80 } }
                    spacing: 5
                    z: 3
                    y: 10
                    height: 40
                    Text {
                        width: 56
                        height: 40
                        color: Theme.textColor
                        text: Playback.sourceText(listfile.source)
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: Style.text
                    }
                }

                MouseArea {
                    z: 1
                    id: playListArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: listHover.opacity = 1
                    onExited: listHover.opacity = 0
                    // 统一走 startTrack：本地/在线/WebDAV 只在这里判定一次
                    onClicked: {
                        Options.queue.playListIndex = listfile.index
                        Playback.startTrack(listfile.index)
                    }

                    Rectangle {
                        id: listHover
                        anchors.fill: parent
                        radius: Style.labelRadius
                        color: Qt.rgba(0.5, 0.5, 0.5, 0.2)
                        opacity: 0
                        visible: opacity > 0
                        z: 2
                        Behavior on opacity { NumberAnimation { duration: 80 } }
                        SButton {
                            id: playNext
                            iconCharacter: "\uf0d9"
                            x: parent.width - 132
                            y: 12
                            width: 36
                            height: 36
                            radius: 36
                            buttonColor: "transparent"
                            hoverColor: Qt.rgba(0.5, 0.5, 0.5, 0.5)
                            shadowEnabled: false
                            tipText: "下一首播放"
                            onClicked: {
                                const cur = Options.queue.playListIndex
                                if (listfile.index === cur) return
                                Options.queue.move(listfile.index, cur + 1, 1)
                                if (listfile.index < cur) Options.queue.playListIndex = cur - 1
                                Options.warned("已设为下一首", 1)
                            }
                        }
                        SButton {
                            id: playMenu
                            iconCharacter: "\uf0c8"
                            x: parent.width - 94
                            y: 12
                            width: 36
                            height: 36
                            radius: 36
                            buttonColor: "transparent"
                            hoverColor: Qt.rgba(0.5, 0.5, 0.5, 0.5)
                            shadowEnabled: false
                            tipText: "收藏"
                            onClicked: {
                                if (listfile.source === -1) { Options.warned("本地歌曲请使用本地收藏", 0); return }
                                if (FavoriteSongs.isFavorite(listfile.path, "song")) {
                                    FavoriteSongs.removeFavorite(listfile.path, "song")
                                    Options.warned("已取消收藏", 0)
                                } else {
                                    FavoriteSongs.addFavorite(listfile.path, listfile.name, listfile.songer, "", listfile.source, 0, "song")
                                    Options.warned("已收藏", 1)
                                }
                            }
                        }
                        SButton {
                            id: playDelete
                            iconCharacter: "\uf08e"
                            x: parent.width - 56
                            y: 12
                            width: 36
                            height: 36
                            radius: 36
                            buttonColor: "transparent"
                            hoverColor: Qt.rgba(1.0, 0.5, 0.5, 0.8)
                            shadowEnabled: false
                            tipText: "移除"
                            onClicked: {
                                if (Options.queue.playListIndex !== listfile.index) {
                                    if (Options.queue.playListIndex > listfile.index) Options.queue.playListIndex -= 1;
                                    Options.queue.remove(listfile.index, 1);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    enter: Transition {
        NumberAnimation { property: "x"; duration: 450; from: playList.parent.width; to: playList.parent.width - 380; easing.type: Easing.OutExpo }
    }
    exit: Transition {
        NumberAnimation { property: "x"; duration: 240; to: playList.parent.width; easing.type: Easing.OutCubic }
    }
}
