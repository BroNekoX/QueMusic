// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
// 下载管理页（对应 pages/DownloadPage.qml）。progress 为 0..1
import QtQuick
import QueMusic 1.0
import 'qrc:/QueMusic/components'

Item {
    id: page
    anchors.fill: parent

    property int downloadTab: 0

    DownloadedMusicModel { id: downloaded }

    function refresh() {
        downloaded.downloadDir = MusicApi.downloader.effectiveDownloadDir()
        downloaded.reload()
    }
    Component.onCompleted: page.refresh()
    Connections {
        target: MusicApi.downloader
        function onCompletedCountChanged() { page.refresh() }
    }

    Column {
        anchors.fill: parent
        spacing: 14
        Item {
            width: parent.width
            height: 36
            Row {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 12
                Text {
                    text: "下载管理"
                    font.pixelSize: 16
                    font.weight: Font.DemiBold
                    color: "#f5f7fb"
                    height: 36
                    verticalAlignment: Text.AlignVCenter
                }
                CenterTabs {
                    model: ["正在下载", "已下载"]
                    tabWidth: 76
                    height: 32
                    currentIndex: page.downloadTab
                    onTabClicked: i => {
                        page.downloadTab = i
                        if (i === 1) page.refresh()
                    }
                }
            }
            QButton {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                text: "打开目录"
                iconCharacter: "\uf0b6"
                height: 34
                fontSize: 12
                onClicked: Qt.openUrlExternally("file:///" + MusicApi.downloader.effectiveDownloadDir())
            }
        }

        Item {
            width: parent.width
            height: parent.height - 50

            Text {
                visible: page.downloadTab === 0 && MusicApi.downloader.taskCount === 0
                width: parent.width
                height: 40
                text: "没有下载任务"
                font.pixelSize: 13
                color: "#9fb2c2"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            ListView {
                width: parent.width
                height: parent.height
                visible: page.downloadTab === 0
                model: MusicApi.downloader
                clip: true
                spacing: 6
                delegate: Rectangle {
                    width: ListView.view.width
                    height: 60
                    radius: 14
                    color: "#14ffffff"
                    border.width: 1
                    border.color: "#14ffffff"

                    QPicture {
                        x: 8
                        y: 8
                        width: 44
                        height: 44
                        radius: 10
                        source: center.coverOf(model.cover)
                        sourceSize: Qt.size(128, 128)
                    }
                    Column {
                        x: 60
                        width: parent.width - 60 - 140
                        height: parent.height
                        Text {
                            width: parent.width
                            height: 30
                            text: model.title || model.fileName
                            font.pixelSize: 13
                            font.bold: true
                            color: "#f2f5fa"
                            elide: Text.ElideRight
                            verticalAlignment: Text.AlignVCenter
                        }
                        Text {
                            width: parent.width
                            height: 22
                            text: model.status === 0 ? "排队中…"
                                : model.status === 1 ? "正在下载…"
                                : model.status === 3 ? "错误：" + model.errorString
                                : model.artist || ""
                            font.pixelSize: 11
                            color: model.status === 3 ? "#ff7a7a" : "#9fb2c2"
                            elide: Text.ElideRight
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                    Rectangle {
                        x: parent.width - 132
                        y: 27
                        width: 88
                        height: 6
                        radius: 3
                        color: "#33ffffff"
                        Rectangle {
                            width: 88 * (model.progress || 0)
                            height: 6
                            radius: 3
                            color: "#ffffff"
                            Behavior on width { NumberAnimation { duration: 120 } }
                        }
                    }
                    Text {
                        x: parent.width - 112
                        y: 20
                        width: 40
                        height: 20
                        text: Math.floor((model.progress || 0) * 100) + "%"
                        font.pixelSize: 11
                        color: "#cfdae6"
                        horizontalAlignment: Text.AlignRight
                        verticalAlignment: Text.AlignVCenter
                    }
                    SButton {
                        x: parent.width - 60
                        y: 12
                        width: 36
                        height: 36
                        radius: 18
                        iconCharacter: model.status === 3 ? "\uf0c7" : "\uf025"
                        buttonColor: "transparent"
                        hoverColor: "#1fffffff"
                        iconColor: "#e8ecf3"
                        iconSize: 14
                        shadowEnabled: false
                        tipText: model.status === 3 ? "重试" : "移除"
                        onClicked: model.status === 3 ? MusicApi.downloader.retryTask(model.taskId)
                                                     : MusicApi.downloader.removeTask(model.taskId)
                    }
                }
            }

            Text {
                visible: page.downloadTab === 1 && downloaded.count === 0
                width: parent.width
                height: 40
                text: "没有已下载的文件"
                font.pixelSize: 13
                color: "#9fb2c2"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            QListView {
                width: parent.width
                height: parent.height
                visible: page.downloadTab === 1 && downloaded.count > 0
                model: downloaded
                isEnd: true
                onClicked: i => {
                    var d = downloaded.get(i)
                    center.playLocal(d.fileUrl, d.title || d.fileName)
                }
                onToolClicked: (i, tool) => {
                    if (tool === 0) {
                        var d = downloaded.get(i)
                        center.enqueue({ title: d.title || d.fileName, artist: d.artist, hash: d.fileUrl, source: -1 })
                    }
                }
            }
        }
    }
}
