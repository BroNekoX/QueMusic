// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
import QtQuick
import QueMusic 1.0
import QtQuick.Controls.Basic

// 播放列表
Popup {
    id: playList
    // 列表中： source：-1：本地 0.酷狗 1.网易云 2.qq音乐
    property alias model: playListView.model
    property string filter: ""
    property int dragIndex: -1
    property real dragOrigin: 0
    property real dragOffset: 0

    padding: 0
    margins: -1
    parent: Overlay.overlay
    width: 360
    height: parent.height - 180
    x: parent.width - 380
    y: 80
    background: QBlurCard {
        anchors.fill: parent
        borderRadius: Style.settings.cubeRadius
        clip: false
        blurSource: mainLayout
        shadowEffect: true
        rectXy: Qt.rect(playList.x, playList.y, 360, playList.height)
        //color: Style.themes.primaryBlurColor
    }

    // 拖动排序：跨过半行即交换，同步修正当前播放下标
    function moveItem(from, to) {
        if (from === to || from < 0 || to < 0 || to >= playListModel.count) return
        var cur = playListModel.playListIndex
        playListModel.move(from, to, 1)
        if (cur === from) cur = to
        else if (from < cur && to >= cur) cur--
        else if (from > cur && to <= cur) cur++
        playListModel.playListIndex = cur
    }

    function locateCurrent() {
        var i = playListModel.playListIndex
        if (i < 0) return
        playListView.positionViewAtIndex(i, ListView.Center)
    }

    contentItem: Item {
        anchors.fill: parent

        Label {
            y: 10
            x: 18
            height: 40
            text: "播放列表 " + playListModel.count + "首"
            font.bold: true
            font.pixelSize: Style.settings.textH2
            verticalAlignment: Text.AlignVCenter
            color: Style.themes.fontColor
        }

        TextField {
            id: filterInput
            x: 18
            y: 60
            width: parent.width - 36
            height: 32
            leftPadding: 12
            rightPadding: 30
            placeholderText: "过滤当前列表"
            placeholderTextColor: Style.themes.textColor
            color: Style.themes.textColor
            font.pixelSize: Style.settings.text
            verticalAlignment: Text.AlignVCenter
            selectionColor: Style.themes.containColor
            onTextChanged: playList.filter = text.trim().toLowerCase()
            background: Rectangle {
                radius: 16
                color: Style.themes.primaryColor
                border.width: 2
                border.color: filterInput.focus ? Style.themes.themeColor : Style.themes.sideColor
            }
            SButton {
                visible: playList.filter !== ""
                x: parent.width - 32
                width: 28
                height: 28
                radius: 14
                iconCharacter: "\uf025"
                iconSize: 12
                buttonColor: "transparent"
                shadowEnabled: false
                onClicked: filterInput.text = ""
            }
        }

        SButton {
            iconCharacter: "\uf050"
            x: parent.width - 130
            y: 12
            width: 36
            height: 36
            radius: 18
            //iconSize: Style.settings.texticon + 2
            buttonColor: "transparent"
            tipText: "定位当前"
            shadowEnabled: false
            onClicked: playList.locateCurrent();
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
                globalDialog.openSimpleDialog("删除", "这将移除播放列表其他歌曲，是否继续？",
                    function() {
                        var title = playListModel.get(playListModel.playListIndex).name;
                        var hash = playListModel.get(playListModel.playListIndex).path;
                        var artist = playListModel.get(playListModel.playListIndex).songer;
                        var source = playListModel.get(playListModel.playListIndex).source;
                        playListModel.remove( 0, playListModel.count );
                        //playListModel.append(indexData);
                        playListModel.append({ name: title, path: hash, songer: artist, source: source });
                        playListModel.playListIndex = 0;
                        Style.warned("已清空播放列表",1);
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
            iconSize: Style.settings.texticon + 2
            buttonColor: "transparent"
            shadowEnabled: false
            onClicked: {
                playList.close();
            }
        }

        ListView {
            id: playListView
            x: 12
            y: 112
            z: 2
            width: 348
            height: parent.height - 112
            model: playListModel
            spacing: 0
            orientation: Qt.Vertical
            clip: true
            topMargin: 4
            rightMargin: 12
            bottomMargin: 12
            property int scrollToY: playListView.contentY
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
                property real scrollMultiplier: Qt.application.styleHints.wheelScrollLines
                onWheel: (event) => {
                    playListView.scrollToY = Math.max(-10, Math.min(playListView.scrollToY - (event.angleDelta.y / 4 * scrollMultiplier), playListView.contentHeight - playListView.height + 10))
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
                readonly property string songName: model.name || ""
                readonly property string songArtist: model.songer || ""
                readonly property bool matched: playList.filter === ""
                        || songName.toLowerCase().indexOf(playList.filter) !== -1
                        || songArtist.toLowerCase().indexOf(playList.filter) !== -1
                readonly property bool isCurrent: playListModel.playListIndex === index
                height: matched ? 60 : 0
                visible: matched
                width: parent.width
                radius: Style.settings.labelRadius
                color: isCurrent ? Style.themes.containColor : "transparent"
                // 拖拽时整行跟随手指，不改动 ListView 管理的 y
                transform: Translate { y: playList.dragIndex === index ? playList.dragOffset : 0 }
                Behavior on color { ColorAnimation { duration: 120 } }

                Text {
                    y: 10
                    x: 8
                    text: index + 1
                    z: 5
                    width: 40
                    height: 40
                    color: isCurrent ? Style.themes.themeColor : Style.themes.textColor
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
                    color: Style.themes.fontColor
                    font.pixelSize: Style.settings.text
                    verticalAlignment: Text.AlignVCenter
                    Behavior on color { ColorAnimation { duration: 120 } }
                }
                Text {
                    x: 50
                    y: 30
                    z: 4
                    width: 200
                    height: 20
                    text: songArtist
                    color: Style.themes.textColor
                    elide: Text.ElideRight
                    font.pixelSize: Style.settings.textTip
                    verticalAlignment: Text.AlignVCenter
                    visible: songArtist !== ""
                    Behavior on color { ColorAnimation { duration: 120 } }
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
                        color: Style.themes.textColor
                        text: model.source == -1 ? "本地" : "在线"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: Style.settings.text
                    }
                }

                MouseArea {
                    z: 1
                    id: playListArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: listHover.opacity = 1
                    onExited: listHover.opacity = 0
                    // 拖拽排序：过程中只做位移，松手时一次性提交，避免频繁改动模型
                    /*onPressed: {
                        playList.dragIndex = index
                        playList.dragOffset = 0
                        playList.dragOrigin = mapToItem(playListView, mouseX, mouseY).y
                    }
                    onPositionChanged: {
                        if (pressed && playList.dragIndex === index)
                            playList.dragOffset = mapToItem(playListView, mouseX, mouseY).y - playList.dragOrigin
                    }
                    onReleased: {
                        var dy = mapToItem(playListView, mouseX, mouseY).y - playList.dragOrigin
                        var from = playList.dragIndex
                        playList.dragIndex = -1
                        playList.dragOffset = 0
                        var step = Math.round(dy / 60)
                        if (from >= 0 && step !== 0)
                            playList.moveItem(from, Math.max(0, Math.min(playListModel.count - 1, from + step)))
                    }*/
                    onClicked: {
                        if (model.source == -1) {
                            playListModel.playListIndex = index
                            window.playLocalSong(model.path, model.name)
                        } else {
                            mainMedia.urlLocal = false
                            playListModel.playListIndex = index
                            MusicApi.getMusicInfo(model.path, 0, model.source)
                        }
                    }

                    Rectangle {
                        id: listHover
                        anchors.fill: parent
                        radius: Style.settings.labelRadius
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
                                var cur = playListModel.playListIndex
                                if (index === cur) return
                                playListModel.move(index, cur + 1, 1)
                                if (index < cur) playListModel.playListIndex = cur - 1
                                mainWarn.tiped("已设为下一首", 1)
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
                                if (model.source === -1) { mainWarn.tiped("本地歌曲请使用本地收藏", 0); return }
                                if (favoritesSong.isFavorite(model.path, "song")) {
                                    favoritesSong.removeFavorite(model.path, "song")
                                    mainWarn.tiped("取消收藏", 0)
                                } else {
                                    favoritesSong.addFavorite(model.path, model.name, model.songer, "", model.source, 0, "song")
                                    mainWarn.tiped("成功收藏", 1)
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
                                if (playListModel.playListIndex !== index) {
                                    if (playListModel.playListIndex > index) playListModel.playListIndex -= 1
                                    playListModel.remove(index, 1)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    enter: Transition {
        NumberAnimation { property: "x"; duration: 420; from: playList.parent.width; to: playList.parent.width - 380; easing.type: Easing.OutExpo }
    }
    exit: Transition {
        NumberAnimation { property: "x"; duration: 210; to: playList.parent.width; easing.type: Easing.InCubic }
    }
}
