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
    contentItem: Item {
        anchors.fill: parent
        Label {
            y: 10
            x: 18
            height: 40
            text: "播放列表"
            font.bold: true
            font.pixelSize: Style.settings.textH2
            verticalAlignment: Text.AlignVCenter
            color: Style.themes.fontColor
        }
        SButton {
            iconCharacter: "\uf08e"
            x: parent.width - 90
            y: 12
            width: 36
            height: 36
            radius: 18
            buttonColor: "transparent"
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
            y: 60
            z: 2
            width: 348
            height: parent.height - 60
            model: playListModel
            spacing: 0
            orientation: Qt.Vertical
            clip: true
            topMargin: 12
            rightMargin: 12
            bottomMargin: 12
            property int scrollToY: playListView.contentY
            ScrollBar.vertical: ScrollBar {
                parent: playListView
                anchors.top: playListView.top
                anchors.left: playListView.right
                //anchors.leftMargin: 8
                anchors.bottom: playListView.bottom
                onPressedChanged: {
                    playListView.scrollToY = playListView.contentY
                }
            }
            WheelHandler {
                property real scrollMultiplier: Qt.application.styleHints.wheelScrollLines

                onWheel: (event) => {
                             playListView.scrollToY = Math.max( -10, Math.min( playListView.scrollToY - (event.angleDelta.y / 4 * scrollMultiplier), playListView.contentHeight - playListView.height + 10))
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
                height: 60
                width: parent.width
                radius: Style.settings.labelRadius
                color: playListModel.playListIndex === index ? Style.themes.containColor : "transparent"

                Behavior on color { ColorAnimation { duration: 120 } }
                Text {
                    y: 10
                    x: 8
                    text: index + 1
                    z: 5
                    width: 40
                    height: 40
                    color: Style.themes.textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }


                Text {
                    x: 50
                    y: model.songer ? 10 : 20
                    z: 4
                    width: 200
                    height: 20
                    text: model.name
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
                    text: model.songer
                    color: Style.themes.textColor
                    elide: Text.ElideRight
                    font.pixelSize: Style.settings.textTip
                    verticalAlignment: Text.AlignVCenter
                    visible: model.songer ? true : false
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
                    onClicked: {
                        if(model.source == -1) {
                            playListModel.playListIndex = index;
                            window.playLocalSong(model.path, model.name);
                        } else {
                            mainMedia.urlLocal = false;
                            playListModel.playListIndex = index;
                            MusicApi.getMusicInfo(model.path,0,model.source);
                        }
                        console.log("name:",model.name," path:",model.path," source:",model.source," artist:",model.songer)
                    }
                    Rectangle {
                        id: listHover
                        anchors.fill: parent
                        radius: Style.settings.labelRadius
                        color: Qt.rgba(0.5,0.5,0.5,0.2)
                        opacity: 0
                        visible: opacity > 0
                        //opacity: playListArea.containsMouse ? 1 : 0
                        z: 2
                        Behavior on opacity { NumberAnimation { duration: 80 } }
                        SButton {
                            id: playMenu
                            iconCharacter: "\uf050"
                            x: parent.width - 96
                            y: 12
                            width: 36
                            height: 36
                            radius: 36
                            buttonColor: "transparent"
                            hoverColor: Qt.rgba(0.5,0.5,0.5,0.5)
                            shadowEnabled: false
                            onClicked: {
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
                            hoverColor: Qt.rgba(1.0,0.5,0.5,0.8)
                            shadowEnabled: false
                            onClicked: {
                                if(playListModel.playListIndex !== index) {
                                    if(playListModel.playListIndex > index) playListModel.playListIndex -= 1;
                                    playListModel.remove( index, 1 );
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