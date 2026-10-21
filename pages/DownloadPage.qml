// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QueMusic 1.0

Item {
    id: downloadPage

    DownloadedMusicModel {
        id: downloadedModel
    }

    // 扫描下载目录
    function refreshDownloads(): void {
        downloadedModel.downloadDir = MusicApi.downloader.effectiveDownloadDir();
        downloadedModel.reload();
    }

    Component.onCompleted: refreshDownloads()

    function itemAt(index: int): var {
        return downloadedModel.get(index)
    }

    function playRow(index: int): void {
        const item = itemAt(index)
        if (!item || !item.fileUrl)
            return
        Playback.playItem({ name: item.title || item.fileName, path: item.fileUrl,
                            songer: item.artist || "", source: -1 })
    }

    function playNext(index: int): void {
        const item = itemAt(index)
        if (!item || !item.fileUrl)
            return
        Playback.playNext({ name: item.title || item.fileName, path: item.fileUrl,
                            songer: item.artist || "", source: -1 })
        Options.warned("已设为下一首播放", 1)
    }

    function enqueueRow(index: int): void {
        const item = itemAt(index)
        if (!item || !item.fileUrl)
            return
        const added = Playback.enqueue({ name: item.title || item.fileName, path: item.fileUrl,
                                         songer: item.artist || "", source: -1 })
        Options.warned(added ? "已加入播放列表" : "已在播放列表中", added ? 1 : 0)
    }

    function revealRow(index: int): void {
        const item = itemAt(index)
        if (item && item.fileUrl)
            Qt.openUrlExternally(MusicApi.downloader.effectiveDownloadDir())
    }

    function removeRow(index: int): void {
        const item = itemAt(index)
        if (!item || !item.fileUrl)
            return
        Options.dialog.openSimpleDialog("删除", "这将把「" + (item.title || item.fileName) + "」移入回收站，是否继续？",
            function() { downloadedModel.removeFile(item.fileUrl) })
    }

    function runMenu(index: int, choice: int): void {
        const item = itemAt(index)
        if (!item)
            return
        if (choice === 0)
            playRow(index)
        else if (choice === 1)
            playNext(index)
        else if (choice === 2)
            enqueueRow(index)
        else if (choice === 3)
            audioInfoDialog.showInfo(item.fileUrl)
        else if (choice === 4)
            metaDialog.edit(item.fileUrl, 0)
        else if (choice === 5)
            removeRow(index)
    }

    Connections {
        target: MusicApi.downloader
        function onCompletedCountChanged(): void { downloadPage.refreshDownloads() }
    }
    Connections {
        target: MusicApi
        function onDownloadPathChanged(): void { downloadPage.refreshDownloads() }
    }

    // 顶部标题
    Item {
        x: 24
        y: 24
        height: 40
        width: parent.width - 48
        z: 10
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: "下载管理"
            font.weight: Font.DemiBold
            font.pixelSize: Style.pageTitle
            color: Theme.fontColor
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
                downloadPage.refreshDownloads();
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
            buttonColor: Theme.primaryColor
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
                color: Theme.textColor
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
                        color: Theme.textColor
                        font.pixelSize: Style.text
                    }
                    Text {
                        x: parent.width - 240
                        height: 32
                        verticalAlignment: Text.AlignVCenter
                        text: "进度"
                        color: Theme.textColor
                        font.pixelSize: Style.text
                    }
                    Text {
                        x: parent.width - 100
                        height: 32
                        verticalAlignment: Text.AlignVCenter
                        text: "操作"
                        color: Theme.textColor
                        font.pixelSize: Style.text
                    }
                    Rectangle {
                        width: parent.width - 16
                        height: 1
                        color: Theme.sideColor
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
                        radius: Style.labelRadius
                        color: Theme.hoverColor
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
                        color: activeDel.status === 0 ? Theme.sideColor
                             : activeDel.status === 1 ? Theme.containColor
                             : Theme.sideColor

                        Text {
                            anchors.centerIn: parent
                            text: activeDel.status === 0 ? "\ue803"
                                 : activeDel.status === 1 ? "\ue80b"
                                 : "\ue803"
                            font.family: Fonts.icon
                            font.pixelSize: 20
                            color: activeDel.status === 1 ? Theme.themeColor
                                 : Theme.textColor
                        }
                    }

                    // 文件名
                    Text {
                        x: 80
                        y: 12
                        width: parent.width - 340
                        height: 24
                        text: activeDel.fileName
                        color: Theme.fontColor
                        font.pixelSize: Style.textmain
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
                        color: activeDel.status === 3 ? "#ff4444" : Theme.textColor
                        font.pixelSize: Style.text
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
                            color: Theme.sideColor
                        }
                        Rectangle {
                            width: barBg.width * activeDel.progress
                            height: 6
                            y: 7
                            radius: 3
                            color: Theme.themeColor
                            Behavior on width { NumberAnimation { duration: 120 } }
                        }
                        Text {
                            x: 104
                            y: 0
                            height: 20
                            verticalAlignment: Text.AlignVCenter
                            text: Math.floor(activeDel.progress * 100) + "%"
                            color: Theme.textColor
                            font.pixelSize: Style.text
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
                        color: Theme.textColor
                        font.pixelSize: Style.text
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
                        font.pixelSize: Style.text
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
                            hoverColor: Theme.hoverColor
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
                            hoverColor: Theme.hoverColor
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
                color: Theme.textColor
                font.pixelSize: 14
                visible: downloadedModel.count === 0
            }

            QLocalView {
                id: downloadView
                anchors.fill: parent
                topMargin: 72
                bottomMargin: 24
                model: downloadedModel
                clip: true
                visible: downloadedModel.count !== 0

                onClicked: (index) => downloadPage.playRow(index)
                onMenuClicked: (index, choice) => downloadPage.runMenu(index, choice)

                delegate: Rectangle {
                    id: row
                    required property int index
                    required property string title
                    required property string artist
                    required property string cover
                    required property int duration
                    required property string fileName
                    required property string fileUrl

                    height: 60
                    width: downloadView.width - 16
                    radius: Style.labelRadius
                    color: Playback.player.source === row.fileUrl ? Theme.containColor : "transparent"

                    ListView.onReused: row.opacity = 1
                    ListView.onPooled: row.opacity = 1

                    Rectangle {
                        anchors.fill: parent
                        radius: Style.labelRadius
                        color: Theme.hoverColor
                        opacity: rowArea.containsMouse ? 1 : 0
                        Behavior on opacity { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
                    }

                    QPicture {
                        y: 8
                        x: 8
                        width: 44
                        height: 44
                        radius: 10
                        source: row.cover || "qrc:/QueMusic/resources/app/musicpic.png"
                    }

                    Text {
                        x: 72
                        width: downloadView.artistX - 110
                        text: row.title || row.fileName
                        color: Theme.fontColor
                        font.weight: Font.DemiBold
                        font.pixelSize: Style.textmain
                        elide: Text.ElideRight
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Text {
                        x: downloadView.artistX
                        width: downloadView.artistX - 128
                        text: row.artist || "Unknown"
                        color: Theme.textColor
                        font.pixelSize: Style.text
                        elide: Text.ElideRight
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Text {
                        x: downloadView.width - 92
                        width: 60
                        text: row.duration > 0 ? Playback.fmt(row.duration * 1000) : ""
                        color: Theme.textColor
                        font.pixelSize: Style.text
                        elide: Text.ElideRight
                        anchors.verticalCenter: parent.verticalCenter
                        horizontalAlignment: Text.AlignHCenter
                    }

                    MouseArea {
                        id: rowArea
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        onClicked: (mouse) => {
                            if (mouse.button === Qt.RightButton) {
                                downloadView.menu.index = row.index
                                downloadView.menu.popup()
                            } else {
                                downloadPage.playRow(row.index)
                            }
                            forceActiveFocus()
                        }

                        Row {
                            x: downloadView.toolX
                            spacing: 2
                            y: 12
                            height: 36
                            opacity: rowArea.containsMouse ? 1 : 0
                            Behavior on opacity { NumberAnimation { duration: 160 } }
                            SButton {
                                iconCharacter: "\uf095"
                                width: 36
                                height: 36
                                radius: 18
                                buttonColor: "transparent"
                                hoverColor: Theme.hoverColor
                                shadowEnabled: false
                                tipText: "加入播放列表"
                                onClicked: downloadPage.enqueueRow(row.index)
                            }
                            SButton {
                                iconCharacter: "\uf107"
                                width: 36
                                height: 36
                                radius: 18
                                buttonColor: "transparent"
                                hoverColor: Theme.hoverColor
                                shadowEnabled: false
                                tipText: "打开所在文件夹"
                                onClicked: downloadPage.revealRow(row.index)
                            }
                            SButton {
                                iconCharacter: "\uf08e"
                                width: 36
                                height: 36
                                radius: 18
                                buttonColor: "transparent"
                                hoverColor: "#dcff9999"
                                shadowEnabled: false
                                tipText: "删除文件"
                                onClicked: downloadPage.removeRow(row.index)
                            }
                        }
                    }
                }
            }
        }
    }

    AudioInfoDialog {
        id: audioInfoDialog
    }

    MetaEditorDialog {
        id: metaDialog
        onSaved: (filePath, source) => downloadedModel.syncSidecar(filePath)
    }
}
