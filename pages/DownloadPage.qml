// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Controls.Basic
import QueMusic 1.0
import QtCore
import Qt.labs.folderlistmodel
import 'qrc:/QueMusic/components'

Item {
    id: downloadPage

    property int downloadTab: 0

    // 接收来自 MusicApi 的下载请求downloader

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
            color: Style.themes.fontColor
        }
    }

    // 标签栏
    QBlurTapBar {
        x: 24
        y: 80
        z: 5
        model: ["正在下载", "已下载", "云端"]
        tabWidth: 100
        width: 304
        rectXy: Qt.rect(0, 12, width, 40)
        blurSource: downloadChildPage
        onTabChange: (index) => {
            downloadChildPage.stack(index);
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
            buttonColor: favouritePage.setMode === 1 ? Style.themes.containColor : Style.themes.fullColor
            onClicked: {
                var path = Options.settings.downloadFolder;
                if (path === "") {
                    path = StandardPaths.writableLocation(StandardPaths.MusicLocation) + "/QueMusic";
                }
                // 转换为 file:// URL
                Qt.openUrlExternally(path);
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
        pageList: [downloadingPage, downloadedPage, cloudPage]

        Item {
            id: downloadingPage
            visible: true
            width: downloadChildPage.width
            height: downloadChildPage.height

            // 空状态提示
            Text {
                anchors.centerIn: parent
                text: "没有下载任务"
                color: Style.themes.textColor
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
                        color: Style.themes.textColor
                        font.pixelSize: Style.settings.text
                    }
                    Text {
                        x: parent.width - 240
                        height: 32
                        verticalAlignment: Text.AlignVCenter
                        text: "进度"
                        color: Style.themes.textColor
                        font.pixelSize: Style.settings.text
                    }
                    Text {
                        x: parent.width - 100
                        height: 32
                        verticalAlignment: Text.AlignVCenter
                        text: "操作"
                        color: Style.themes.textColor
                        font.pixelSize: Style.settings.text
                    }
                    Rectangle {
                        width: parent.width - 16
                        height: 1
                        color: Style.themes.sideColor
                        y: 31
                    }
                }

                delegate: Item {
                    id: activeDel
                    height: 64
                    width: activeList.width - 16
                    visible: model.status === 0 || model.status === 1
                    opacity: model.status === 1 ? 1.0 : 0.6

                    Rectangle {
                        anchors.fill: parent
                        radius: Style.settings.labelRadius
                        color: Style.themes.hoverColor
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
                        color: model.status === 0 ? Style.themes.sideColor
                             : model.status === 1 ? Style.themes.containColor
                             : Style.themes.sideColor

                        Text {
                            anchors.centerIn: parent
                            text: model.status === 0 ? "\ue803"
                                 : model.status === 1 ? "\ue80b"
                                 : "\ue803"
                            font.family: iconFont.name
                            font.pixelSize: 20
                            color: model.status === 1 ? Style.themes.themeColor
                                 : Style.themes.textColor
                        }
                    }

                    // 文件名
                    Text {
                        x: 80
                        y: 12
                        width: parent.width - 340
                        height: 24
                        text: model.fileName
                        color: Style.themes.fontColor
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
                            if (model.status === 0) return "排队中…"
                            if (model.status === 1) return "正在下载…"
                            if (model.status === 3) return "错误: " + model.errorString
                            return ""
                        }
                        color: model.status === 3 ? "#ff4444" : Style.themes.textColor
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
                        visible: model.status === 1

                        Rectangle {
                            id: barBg
                            width: 100
                            height: 6
                            y: 7
                            radius: 3
                            color: Style.themes.sideColor
                        }
                        Rectangle {
                            width: barBg.width * model.progress
                            height: 6
                            y: 7
                            radius: 3
                            color: Style.themes.themeColor
                            Behavior on width { NumberAnimation { duration: 120 } }
                        }
                        Text {
                            x: 104
                            y: 0
                            height: 20
                            verticalAlignment: Text.AlignVCenter
                            text: Math.floor(model.progress * 100) + "%"
                            color: Style.themes.textColor
                            font.pixelSize: Style.settings.text
                        }
                    }

                    // 进度百分比（对排队中的任务显示文字）
                    Text {
                        x: parent.width - 240
                        y: 22
                        height: 20
                        verticalAlignment: Text.AlignVCenter
                        visible: model.status === 0
                        text: "等待中"
                        color: Style.themes.textColor
                        font.pixelSize: Style.settings.text
                    }

                    // 错误状态文字
                    Text {
                        x: parent.width - 240
                        y: 22
                        height: 20
                        verticalAlignment: Text.AlignVCenter
                        visible: model.status === 3
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
                            hoverColor: Style.themes.hoverColor
                            shadowEnabled: false
                            onClicked: MusicApi.downloader.removeTask(model.taskId)
                        }

                        // 重试
                        SButton {
                            iconCharacter: "\ue819"
                            iconSize: 14
                            width: 36
                            height: 36
                            radius: 36
                            buttonColor: "transparent"
                            hoverColor: Style.themes.hoverColor
                            shadowEnabled: false
                            visible: model.status === 3
                            onClicked: MusicApi.downloader.retryTask(model.taskId)
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
                color: Style.themes.textColor
                font.pixelSize: 14
                visible: downloadFileModel.count === 0
            }

            FolderListModel {
                id: downloadFileModel
                nameFilters: ["*.mp3","*.wav","*.aac","*.flac","*.ogg","*.eac3","*.wma","*.ac3","*.alac","*.mkv","*.wmv","*.avi","*.mpeg4"]
                folder: StandardPaths.writableLocation(StandardPaths.MusicLocation) + "/QueMusic"
            }

            QListView {
                id: localFileView
                anchors.fill: parent
                topMargin: 72
                bottomMargin: 24
                model: downloadFileModel
                clip: true
                visible: downloadFileModel.count !== 0
                //reuseItems: true
                headerModel: ["标题","","","菜单"]
                populate: Transition {
                    id: localFileLoadAnime
                    SequentialAnimation {
                        PropertyAction {
                            property: "opacity"
                            value: 0
                        }
                        PauseAnimation {
                            duration: localFileLoadAnime.ViewTransition.index * 80
                        }
                        ParallelAnimation {
                            NumberAnimation {
                                properties: "x"
                                from: 400
                                to: 0
                                duration: 350
                                easing.type: Easing.OutExpo
                            }
                            NumberAnimation {
                                properties: "opacity"
                                from: 0
                                to: 1
                                duration: 350
                                easing.type: Easing.OutExpo
                            }
                        }
                    }
                }
                delegate: Rectangle {
                    id: listLocalFile
                    height: 60
                    width: localFileView.width - 16
                    radius: Style.settings.labelRadius
                    color: mainMedia.source == model.fileUrl ? Style.themes.onPrimaryColor : "transparent"

                    Behavior on color { ColorAnimation { duration: 120 } }

                    Rectangle {
                        y: 8
                        x: 8
                        z: 4
                        width: 44
                        height: 44
                        color: Style.themes.containColor
                        radius: 10
                        Text {
                            anchors.fill: parent
                            text: "\uf044"
                            font.family: iconFont.name
                            font.pixelSize: Style.settings.texticon
                            color: Style.themes.fontColor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    Rectangle {
                        anchors.fill: parent
                        radius: Style.settings.labelRadius
                        color: Qt.rgba(0.5,0.5,0.5,0.2)
                        opacity: localFileArea.containsMouse ? 1 : 0
                        z: 1
                        Behavior on opacity { NumberAnimation { duration: 80 } }
                    }


                    Label {
                        x: 80
                        y: 0
                        z: 3
                        width: 140
                        height: 60
                        text: model.fileName
                        color: Style.themes.fontColor
                        font.bold: true
                        font.pixelSize: Style.settings.textmain
                        verticalAlignment: Text.AlignVCenter
                        visible: true
                        Behavior on color { ColorAnimation { duration: 120 } }
                    }

                    MouseArea {
                        id: localFileArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            mainMedia.urlLocal = true;
                            mainMedia.source = model.fileUrl;
                            mainMedia.noTitle = model.fileName;
                            console.log("url:", model.fileUrl);
                            MusicApi.setLocalLyrics();
                            mainMedia.play();
                            var musicName = model.fileName;
                            var musicPath = model.fileUrl.toString();
                            var listIndex = listLocalFile.findIndexByValue(playListModel, "name", musicName);
                            if (listIndex == -1) {
                                playListModel.append({ name: musicName, path: musicPath, songer: "", source: -1 });
                                playListModel.playListIndex = playListModel.count - 1;
                            }
                        }
                        Row {
                            anchors.right: parent.right
                            anchors.rightMargin: 20
                            spacing: 2
                            y: 12
                            height: 36
                            SButton {
                                iconCharacter: "\uf095"
                                width: 36
                                height: 36
                                radius: 18
                                buttonColor: "transparent"
                                hoverColor: Qt.rgba(0.5,0.5,0.5,0.2)
                                shadowEnabled: false
                                onClicked: {
                                    var musicName = model.fileName;
                                    var musicPath = model.fileUrl.toString();
                                    var listIndex = listLocalFile.findIndexByValue(playListModel, "name", musicName);
                                    if (listIndex == -1) {
                                        playListModel.append({ name: musicName, path: musicPath, songer: "", source: -1 });
                                    }
                                }
                            }
                            SButton {
                                iconCharacter: "\uf107"
                                width: 36
                                height: 36
                                radius: 18
                                buttonColor: "transparent"
                                hoverColor: Qt.rgba(0.5,0.5,0.5,0.2)
                                shadowEnabled: false
                                onClicked: {
                                }
                            }
                            SButton {
                                iconCharacter: "\uf08e"
                                width: 36
                                height: 36
                                radius: 18
                                buttonColor: "transparent"
                                hoverColor: Qt.rgba(1.0,0.5,0.5,0.8)
                                shadowEnabled: false
                                onClicked: {
                                    myfileModel.get(filePage.folderNumber).music.remove(index);
                                }
                            }
                        }
                    }

                    function findIndexByValue(model, key, targetValue) {
                        for (var i = 0; i < model.count; i++) {
                            var element = model.get(i);
                            if (element[key] === targetValue) {
                                return i; // 返回找到的索引
                            }
                        }
                        return -1; // 未找到返回 -1
                    }
                }
            }
        }

        // 云端
        Item {
            id: cloudPage
            visible: false
            width: downloadChildPage.width
            height: downloadChildPage.height
            Text {
                anchors.centerIn: parent
                text: "云端"
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                color: Style.themes.textColor
                font.pixelSize: 14
            }
        }
    }
}
