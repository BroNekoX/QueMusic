// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Controls.Basic
import QueMusic 1.0

Item {
    id: downloadPage

    property int downloadTab: 0

    DownloadedMusicModel {
        id: downloadedModel
    }

    // 扫描下载目录
    function refreshDownloads(): void {
        downloadedModel.downloadDir = MusicApi.downloader.effectiveDownloadDir();
        downloadedModel.reload();
    }

    Component.onCompleted: refreshDownloads()

    Connections {
        target: MusicApi.downloader
        function onCompletedCountChanged(): void { refreshDownloads() }
    }
    Connections {
        target: MusicApi
        function onDownloadPathChanged(): void { refreshDownloads() }
    }

    // 顶部标题
    Item {
        x: 24
        y: 24
        height: 40
        width: parent.width - 48
        z: 10
        Text {
            x: 0
            y: 0
            height: 40
            verticalAlignment: Text.AlignVCenter
            text: "下载管理"
            font.weight: Font.DemiBold
            font.pixelSize: Style.settings.pageTitle
            color: Style.fontColor
        }
    }

    // 标签栏
    QBlurTapBar {
        x: 24
        y: 80
        z: 5
        model: ["正在下载", "已下载"]
        tabWidth: 90
        width: 186
        rectXy: Qt.rect(0, 12, width, 40)
        blurSource: downloadChildPage
        onTabChange: (index) => {
            downloadChildPage.stack(index);
            if (index === 1)
                refreshDownloads();
        }
    }

    // 右侧操作区
    Row {
        x: parent.width - width - 24
        y: 80
        z: 2
        spacing: 8
        QButton {
            height: 38
            text: "文件夹中显示"
            iconCharacter: "\uf0fb"
            buttonColor: favouritePage.setMode === 1 ? Style.containColor : Style.fullColor
            onClicked: {
                Qt.openUrlExternally(MusicApi.downloader.effectiveDownloadDir());
            }
        }
    }

    // 页面容器
    QPages {
        x: 24
        y: 68
        width: parent.width - 32
        height: parent.height - 68
        id: downloadChildPage
        pageList: [downloadingPage, downloadedPage]

        Item {
            id: downloadingPage
            visible: true
            width: downloadChildPage.width
            height: downloadChildPage.height

            // 空状态提示
            Text {
                anchors.centerIn: parent
                text: "没有下载任务"
                color: Style.textColor
                font.pixelSize: 14
                visible: MusicApi.downloader.taskCount === 0
            }

            ListView {
                id: activeList
                anchors.fill: parent
                anchors.topMargin: 72
                anchors.bottomMargin: 24
                model: MusicApi.downloader
                clip: true
                spacing: 4
                reuseItems: true
                ScrollBar.vertical: ScrollBar {
                    parent: activeList
                    anchors.top: activeList.top
                    anchors.right: activeList.right
                    anchors.bottom: activeList.bottom
                }
                // 只显示排队/下载中的任务
                visible: MusicApi.downloader.hasActiveTasks || MusicApi.downloader.taskCount > 0

                header: Item {
                    width: activeList.width
                    height: 32
                    visible: MusicApi.downloader.taskCount > 0
                    Text {
                        x: 80
                        height: 32
                        verticalAlignment: Text.AlignVCenter
                        text: "文件名"
                        color: Style.textColor
                        font.pixelSize: Style.settings.text
                    }
                    Text {
                        x: parent.width - 240
                        height: 32
                        verticalAlignment: Text.AlignVCenter
                        text: "进度"
                        color: Style.textColor
                        font.pixelSize: Style.settings.text
                    }
                    Text {
                        x: parent.width - 100
                        height: 32
                        verticalAlignment: Text.AlignVCenter
                        text: "操作"
                        color: Style.textColor
                        font.pixelSize: Style.settings.text
                    }
                    Rectangle {
                        width: parent.width - 16
                        height: 1
                        color: Style.sideColor
                        y: 31
                    }
                }

                delegate: Item {
                    id: activeDel
                    required property int status
                    required property real progress
                    required property string fileName
                    required property int taskId
                    required property string errorString
                    height: 64
                    width: activeList.width - 16
                    visible: activeDel.status === 0 || activeDel.status === 1
                    opacity: activeDel.status === 1 ? 1.0 : 0.6

                    Rectangle {
                        anchors.fill: parent
                        radius: Style.settings.labelRadius
                        color: Style.hoverColor
                        opacity: area.containsMouse ? 1 : 0
                        Behavior on opacity { NumberAnimation { duration: 80 } }
                    }

                    // 图标
                    Rectangle {
                        x: 8
                        y: 8
                        width: 48
                        height: 48
                        radius: 10
                        color: activeDel.status === 0 ? Style.sideColor
                             : activeDel.status === 1 ? Style.containColor
                             : Style.sideColor

                        Text {
                            anchors.centerIn: parent
                            text: activeDel.status === 0 ? "\ue803"
                                 : activeDel.status === 1 ? "\ue80b"
                                 : "\ue803"
                            font.family: IconFont.name
                            font.pixelSize: 20
                            color: activeDel.status === 1 ? Style.themeColor
                                 : Style.textColor
                        }
                    }

                    // 文件名
                    Text {
                        x: 80
                        y: 12
                        width: parent.width - 340
                        height: 24
                        text: activeDel.fileName
                        color: Style.fontColor
                        font.pixelSize: Style.settings.textmain
                        font.bold: true
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                    }

                    // 状态文字（排队中 / 下载中 / 错误）
                    Text {
                        x: 80
                        y: 36
                        width: parent.width - 340
                        height: 20
                        text: {
                            if (activeDel.status === 0) return "排队中…"
                            if (activeDel.status === 1) return "正在下载…"
                            if (activeDel.status === 3) return "错误: " + activeDel.errorString
                            return ""
                        }
                        color: activeDel.status === 3 ? "#ff4444" : Style.textColor
                        font.pixelSize: Style.settings.text
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                    }

                    // 进度条
                    Item {
                        x: parent.width - 240
                        y: 22
                        width: 120
                        height: 20
                        visible: activeDel.status === 1

                        Rectangle {
                            id: barBg
                            width: 100
                            height: 6
                            y: 7
                            radius: 3
                            color: Style.sideColor
                        }
                        Rectangle {
                            width: barBg.width * activeDel.progress
                            height: 6
                            y: 7
                            radius: 3
                            color: Style.themeColor
                            Behavior on width { NumberAnimation { duration: 120 } }
                        }
                        Text {
                            x: 104
                            y: 0
                            height: 20
                            verticalAlignment: Text.AlignVCenter
                            text: Math.floor(activeDel.progress * 100) + "%"
                            color: Style.textColor
                            font.pixelSize: Style.settings.text
                        }
                    }

                    // 进度百分比（对排队中的任务显示文字）
                    Text {
                        x: parent.width - 240
                        y: 22
                        height: 20
                        verticalAlignment: Text.AlignVCenter
                        visible: activeDel.status === 0
                        text: "等待中"
                        color: Style.textColor
                        font.pixelSize: Style.settings.text
                    }

                    // 错误状态文字
                    Text {
                        x: parent.width - 240
                        y: 22
                        height: 20
                        verticalAlignment: Text.AlignVCenter
                        visible: activeDel.status === 3
                        text: "下载失败"
                        color: "#ff4444"
                        font.pixelSize: Style.settings.text
                    }

                    // 操作按钮
                    Row {
                        anchors.right: parent.right
                        anchors.rightMargin: 8
                        y: 14
                        spacing: 2

                        // 取消/移除
                        SButton {
                            iconCharacter: "\ue804"
                            iconSize: 14
                            width: 36
                            height: 36
                            radius: 36
                            buttonColor: "transparent"
                            hoverColor: Style.hoverColor
                            shadowEnabled: false
                            onClicked: MusicApi.downloader.removeTask(activeDel.taskId)
                        }

                        // 重试
                        SButton {
                            iconCharacter: "\ue819"
                            iconSize: 14
                            width: 36
                            height: 36
                            radius: 36
                            buttonColor: "transparent"
                            hoverColor: Style.hoverColor
                            shadowEnabled: false
                            visible: activeDel.status === 3
                            onClicked: MusicApi.downloader.retryTask(activeDel.taskId)
                        }
                    }

                    MouseArea {
                        id: area
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.NoButton
                    }
                }
            }
        }

        // 已下载
        Item {
            id: downloadedPage
            visible: false
            width: downloadChildPage.width
            height: downloadChildPage.height

            Text {
                anchors.centerIn: parent
                text: "没有已下载的文件,去下载几个音乐喵"
                color: Style.textColor
                font.pixelSize: 14
                visible: downloadedModel.count === 0
            }

            QListView {
                id: localFileView
                anchors.fill: parent
                topMargin: 72
                bottomMargin: 24
                model: downloadedModel
                clip: true
                visible: downloadedModel.count !== 0
                headerModel: ["标题","歌手","时长","操作"]

                onClicked: (index) => {
                    const item = downloadedModel.get(index);
                    if (!item || !item.fileUrl)
                        return;
                    Playback.playLocalSong(item.fileUrl, item.fileName);

                    const listIndex = playListModel.indexOfPath(item.fileUrl);
                    if (listIndex === -1) {
                        playListModel.append({ name: item.title || item.fileName, path: item.fileUrl, songer: item.artist || "", source: -1 });
                        playListModel.playListIndex = playListModel.count - 1;
                    } else {
                        playListModel.playListIndex = listIndex;
                    }
                }

            }
        }
    }
}
