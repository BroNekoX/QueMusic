// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
// 列表详情：歌单 / 榜单 / 歌手歌曲共用（实时模糊玻璃，读取 MusicApi.playlistSong）
// 分页：调用方通过 loadMore 注入"加载下一页"闭包，返回 true 表示已发起请求
import QtQuick
import QueMusic 1.0
import 'qrc:/QueMusic/components'

QBlurCard {
    id: root
    blurMax: 48
    blur: 1.0
    saturation: 1.25
    borderRadius: 24
    cardColor: "#6611131c"
    borderColor: "#1fffffff"
    borderWidth: 1

    property Item blurSource
    rectXy: blurSource ? root.mapToItem(blurSource, 0, 0, root.width, root.height)
                       : Qt.rect(root.x, root.y, root.width, root.height)

    property bool opened: false
    property string title: ""
    property url cover: "qrc:/QueMusic/resources/app/musicpic.png"
    property int pageSize: 50
    property var loadMore: null

    signal closeClicked
    signal picked(int index, var data)

    x: root.opened ? 0 : parent.width + 16
    visible: x < parent.width
    Behavior on x { NumberAnimation { duration: 380; easing.type: Easing.OutExpo } }

    // 头部主色氛围
    Rectangle {
        width: 480
        height: 260
        x: -90
        y: -130
        radius: Math.max(width, height) / 2
        opacity: 0.4
        gradient: Gradient {
            GradientStop { position: 0; color: center.c1 }
            GradientStop { position: 1; color: "#00ffffff" }
        }
    }

    Column {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 14
        Item {
            width: parent.width
            height: 88
            QPicture {
                width: 88
                height: 88
                radius: 18
                source: root.cover
                sourceSize: Qt.size(200, 200)
            }
            Column {
                width: parent.width - 104 - 220
                anchors.verticalCenter: parent.verticalCenter
                spacing: 6
                Text {
                    width: parent.width
                    text: root.title || "歌单"
                    font.pixelSize: 20
                    font.weight: Font.DemiBold
                    color: "#f5f7fb"
                    elide: Text.ElideRight
                }
                Text {
                    text: "共 " + MusicApi.playlistSong.count + " 首"
                    font.pixelSize: 12
                    color: "#8d99aa"
                }
            }
            Row {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                spacing: 10
                SButton {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 36
                    height: 36
                    radius: 18
                    iconCharacter: "\uf0dc"
                    buttonColor: "#14ffffff"
                    hoverColor: "#22ffffff"
                    iconColor: "#e6ecf4"
                    iconSize: 15
                    shadowEnabled: false
                    onClicked: root.closeClicked()
                    tipText: "返回"
                }
                QButton {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "全部加入队列"
                    iconCharacter: "\uf098"
                    height: 38
                    radius: 19
                    fontSize: 12
                    buttonColor: "#24ffffff"
                    textColor: "#f2f5fa"
                    iconColor: "#f2f5fa"
                    shadowEnabled: false
                    onClicked: addAll()
                }
            }
        }

        Item {
            // parent 是已扣除边距的 Column：头部 Row 88 + spacing 14
            width: parent.width
            height: parent.height - 102
            QListView {
                anchors.fill: parent
                model: MusicApi.playlistSong
                isEnd: MusicApi.playlistSong.count > 0
                onClicked: i => root.picked(i, MusicApi.playlistSong.get(i))
                onEnded: {
                    if (!root.loadMore) {
                        isEnd = true
                        return
                    }
                    isEnd = !root.loadMore()
                }
            }
            Column {
                anchors.centerIn: parent
                spacing: 6
                visible: MusicApi.playlistSong.count === 0
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "这里还没有歌曲"
                    font.pixelSize: 14
                    font.weight: Font.DemiBold
                    color: "#93a0b2"
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "内容加载后就会出现在这里"
                    font.pixelSize: 12
                    color: "#6b7686"
                }
            }
        }
    }

    function addAll(): void {
        const m = MusicApi.playlistSong
        let n = 0
        for (let i = 0; i < m.count; i++) {
            const d = m.get(i)
            if (d && d.hash && Playback.indexOfPath(d.hash) === -1) {
                Playback.queue.append({ name: d.title, path: d.hash, songer: d.artist, source: MusicApi.songSource })
                n++
            }
        }
        mainWarn.tiped("已加入 " + n + " 首", 1)
    }
}
