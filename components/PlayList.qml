// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
pragma ComponentBehavior: Bound
import QtQuick
import QueMusic 1.0
import QtQuick.Controls.Basic

// 播放列表
Popup {
    id: playList
    // 列表中： source：-1：本地 0.酷狗 1.网易云 2.哔哩哔哩 3.qq音乐
    property alias model: playListView.model
    property bool edit: false
    property int previewId: -1
    readonly property bool previewing: previewId >= 0
    property string previewName: ""

    padding: 0
    margins: -1
    parent: Overlay.overlay
    width: 360
    height: parent.height - 180
    x: parent.width - 380
    y: 80
    background: QBlurCard {
        y: 0
        x: playList.edit ? -300 : 0
        Behavior on x { NumberAnimation { duration: 460; easing.type: Easing.OutExpo } }
        width: playList.width - x
        height: playList.height
        borderRadius: Style.cubeRadius
        clip: false
        blurSource: Options.mainLayout
        shadowEffect: true
        rectXy: Qt.rect(playList.x + x, playList.y, 360 - x, playList.height)
        Item {
            // 编辑相关ui
            x: 0
            y: 0
            width: 300
            height: playList.height
            visible: playList.edit
            opacity: visible ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 360; easing.type: Easing.OutCubic } }
            Text {
                y: 12
                x: 18
                height: 36
                text: "编辑"
                font.bold: true
                font.pixelSize: Style.textH2
                verticalAlignment: Text.AlignVCenter
                color: Theme.fontColor
            }
            QButton {
                x: 12
                y: parent.height - 48
                z: 2
                height: 36
                width: 150
                radius: Style.labelRadius
                text: playList.previewing ? "加载该保存的列表" : "保存当前播放列表"
                buttonColor: playList.previewing ? Theme.themeColor : Theme.primaryColor
                textColor: playList.previewing ? Theme.primaryColor : Theme.fontColor
                borderWidth: 1
                borderColor: playList.previewing ? Theme.themeColor : Theme.sideColor
                shadowEnabled: false
                enabled: Options.queue ? (playList.previewing ? playList.previewId >= 0
                                                              : Options.queue.count > 0) : false
                opacity: enabled ? 1 : 0.45
                onClicked: {
                    if (playList.previewing) playList.loadSaved(playList.previewId)
                    else savePlaylistDialog.open()
                }
            }
            QButton {
                x: 174
                y: parent.height - 48
                z: 2
                height: 36
                width: 114
                radius: Style.labelRadius
                text: "清空播放列表"
                buttonColor: "#fd8888"
                textColor: Theme.primaryColor
                borderWidth: 1
                borderColor: Theme.sideBlurColor
                shadowEnabled: false
                onClicked: {
                    Options.dialog.openSimpleDialog("删除", "这将移除播放列表其他歌曲，是否继续？",
                        function() {
                            // 空队列 / 无当前曲时下标为 -1：那就只清空，不留一条空条目
                            Options.queue.clear();
                            Options.warned("已清空播放列表",1);
                        }
                    );
                }
            }

            Rectangle {
                x: 12
                y: 52
                z: 3
                width: parent.width - 24
                height: 46
                radius: Style.labelRadius
                color: !playList.previewing ? Theme.containColor
                                            : (currentPlaylistArea.containsMouse ? Theme.hoverColor : "transparent")
                Behavior on color { ColorAnimation { duration: 100 } }

                Text {
                    x: 10
                    anchors.verticalCenter: parent.verticalCenter
                    text: "\uf044"
                    font.family: Fonts.icon
                    font.pixelSize: Style.texticon
                    color: !playList.previewing ? Theme.themeColor : Theme.textColor
                }
                Text {
                    x: 38
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width - 100
                    text: "当前播放列表"
                    color: Theme.fontColor
                    font.pixelSize: Style.textmain
                    elide: Text.ElideRight
                }
                Text {
                    anchors.right: parent.right
                    anchors.rightMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    text: (Options.queue ? Options.queue.count : 0) + " 首"
                    color: Theme.textColor
                    font.pixelSize: Style.textTip
                }
                MouseArea {
                    id: currentPlaylistArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: playList.exitPreview()
                }
            }

            Text {
                x: 14
                y: 104
                width: parent.width - 28
                height: 22
                text: "保存的播放列表"
                color: Theme.textColor
                font.pixelSize: Style.textTip
                verticalAlignment: Text.AlignVCenter
            }

            ListView {
                id: listSaveView
                x: 12
                y: 128
                z: 3
                width: parent.width - 24
                height: parent.height - 188
                clip: true
                spacing: 2
                topMargin: 2
                rightMargin: 12
                bottomMargin: 8
                boundsBehavior: Flickable.StopAtBounds
                reuseItems: true
                model: Options.queue ? Options.queue.savedPlaylists : null
                ScrollBar.vertical: ScrollBar {
                    parent: listSaveView
                    anchors.top: listSaveView.top
                    anchors.right: listSaveView.right
                    anchors.bottom: listSaveView.bottom
                }
                delegate: Rectangle {
                    id: savedListRow
                    required property int playlistId
                    required property string name
                    required property int songCount
                    required property int index
                    width: ListView.view.width
                    height: 56
                    radius: Style.labelRadius
                    color: playList.previewId === savedListRow.playlistId
                           ? Theme.containColor
                           : (savedListArea.containsMouse ? Theme.hoverColor : "transparent")
                    Behavior on color { ColorAnimation { duration: 100 } }

                    Text {
                        x: 12
                        anchors.verticalCenter: parent.verticalCenter
                        text: savedListRow.index
                        font.pixelSize: Style.text
                        color: Theme.textColor
                    }
                    Text {
                        x: 38
                        y: 10
                        width: parent.width - 150
                        height: 20
                        text: savedListRow.name
                        color: Theme.fontColor
                        font.pixelSize: Style.textmain
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                    }
                    Text {
                        x: 38
                        y: 29
                        width: parent.width - 150
                        height: 18
                        text: savedListRow.songCount + " 首"
                        color: Theme.textColor
                        font.pixelSize: Style.textTip
                        verticalAlignment: Text.AlignVCenter
                    }

                    MouseArea {
                        id: savedListArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: playList.previewPlaylist(savedListRow.playlistId, savedListRow.name)
                        Row {
                            anchors.right: parent.right
                            anchors.rightMargin: 8
                            y: 12
                            spacing: 2
                            opacity: savedListArea.containsMouse ? 1 : 0
                            visible: opacity > 0
                            Behavior on opacity { NumberAnimation { duration: 80 } }
                            SButton {
                                width: 32
                                height: 32
                                radius: 16
                                iconSize: 14
                                buttonColor: "transparent"
                                shadowEnabled: false
                                iconCharacter: "\uf0e7"
                                tipText: "加载"
                                onClicked: playList.loadSaved(savedListRow.playlistId)
                            }
                            SButton {
                                width: 32
                                height: 32
                                radius: 16
                                iconSize: 14
                                buttonColor: "transparent"
                                shadowEnabled: false
                                iconCharacter: "\uf005"
                                tipText: "重命名"
                                onClicked: {
                                    renamePlaylistDialog.input = savedListRow.name
                                    renamePlaylistDialog.playlistId = savedListRow.playlistId
                                    renamePlaylistDialog.open()
                                }
                            }
                            SButton {
                                width: 32
                                height: 32
                                radius: 16
                                iconSize: 14
                                buttonColor: "transparent"
                                shadowEnabled: false
                                iconCharacter: "\uf08e"
                                hoverColor: "#cdfd8888"
                                tipText: "删除"
                                onClicked: {
                                    const removeId = savedListRow.playlistId
                                    const removeName = savedListRow.name
                                    Options.dialog.openSimpleDialog("删除播放列表",
                                        "将删除「" + removeName + "」，是否继续？",
                                        function() { Options.queue.deletePlaylist(removeId) })
                                }
                            }
                        }
                    }
                }
            }

            Text {
                x: 12
                y: 180
                width: parent.width - 24
                z: 4
                horizontalAlignment: Text.AlignHCenter
                text: "还没有保存的播放列表"
                color: Theme.textColor
                font.pixelSize: Style.text
                visible: !Options.queue || Options.queue.savedPlaylists.count === 0
            }

            Rectangle {
                x: parent.width - 1
                y: 12
                width: 1
                height: parent.height - 24
                color: Theme.sideColor
                opacity: 0.8
            }

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                propagateComposedEvents: false  // 阻止事件穿透
                acceptedButtons: Qt.AllButtons
            }
        }
    }
    onAboutToHide: {
        playList.edit = false;
        playList.exitPreview();
    }

    Connections {
        target: Options.queue
        function onPlaylistSaved(listId, listName, listCount) {
            Options.warned("已保存播放列表「" + listName + "」", 1);
        }
        function onPlaylistLoaded(listId, listName, listCount) {
            playList.exitPreview();
            Playback.saveQueue();
            Options.warned("已加载播放列表「" + listName + "」，共 " + listCount + " 首", 1);
        }
        function onPlaylistRenamed(listId, listName) {
            if (playList.previewId === listId) playList.previewName = listName;
            Options.warned("已重命名播放列表", 1);
        }
        function onPlaylistRemoved(listId) {
            if (playList.previewId === listId) playList.exitPreview();
            Options.warned("已删除播放列表", 1);
        }
        function onPlaylistError(message) {
            Options.warned(message, 2);
        }
    }

    function locateCurrent(): void {
        if (previewing) return;
        const i = Options.queue.playListIndex;
        if (i < 0) return;
        playListView.positionViewAtIndex(i, ListView.Center);
    }

    function previewPlaylist(id: int, name: string): void {
        previewId = id;
        previewName = name;
        Options.queue.previewPlaylist(id);
        playListView.model = Options.queue.playlistPreview;
        playListView.positionViewAtBeginning();
    }

    function exitPreview(): void {
        if (previewId < 0) return;
        previewId = -1;
        previewName = "";
        Options.queue.playlistPreview.clear();
        playListView.model = Options.queue;
        playListView.positionViewAtBeginning();
    }

    function loadSaved(id: int): void {
        if (id >= 0) Options.queue.loadPlaylist(id);
    }

    contentItem: Item {
        anchors.fill: parent

        Text {
            y: 12
            x: 18
            height: 36
            text: playList.previewing
                  ? "预览：" + playList.previewName
                  : "播放列表"
            font.bold: true
            font.pixelSize: Style.textH2
            verticalAlignment: Text.AlignVCenter
            color: Theme.fontColor
            elide: Text.ElideRight
            Rectangle {
                y: 6
                x: parent.width + 8
                height: 24
                radius: 8
                color: Theme.themeColor
                visible: !playList.previewing
                width: songCountTag.width + 16
                Text {
                    id: songCountTag
                    anchors.centerIn: parent
                    font.bold: true
                    font.pixelSize: Style.textmain
                    text: (Options.queue ? Options.queue.count : 0) + "首"
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
            visible: !playList.previewing
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
            iconCharacter: "\uf098"
            x: parent.width - 90
            y: 12
            width: 36
            height: 36
            radius: 18
            buttonColor: playList.edit ? Theme.containColor : "transparent"
            tipText: "管理播放列表"
            shadowEnabled: false
            onClicked: {
                playList.edit = !playList.edit
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
            opacity: 1
            clip: true
            topMargin: 4
            rightMargin: 12
            bottomMargin: 12
            property int scrollToY: playListView.contentY
            reuseItems: true
            move: Transition { NumberAnimation { properties: "y"; duration: 200; easing.type: Easing.OutCubic } }
            moveDisplaced: Transition { NumberAnimation { properties: "y"; duration: 200; easing.type: Easing.OutCubic } }

            WheelHandler {
                property real scrollMultiplier: Qt.application.styleHints.wheelScrollLines / 4
                onWheel: (event) => {
                    playListView.scrollToY = Math.max(-4, Math.min(playListView.scrollToY - (event.angleDelta.y * scrollMultiplier), playListView.contentHeight - playListView.height + 12))
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
                readonly property bool isCurrent: !playList.previewing && Options.queue.playListIndex === listfile.index
                height: 60
                width: ListView.view.width - 12
                radius: Style.labelRadius
                color: isCurrent ? Theme.containColor : "transparent"

                Text {
                    y: 20
                    x: 18
                    text: listfile.index + 1
                    z: 5
                    width: 20
                    height: 20
                    color: listfile.isCurrent ? Theme.themeColor : Theme.textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                Text {
                    x: 50
                    y: listfile.songArtist ? 10 : 20
                    z: 4
                    width: 180
                    height: 20
                    text: listfile.songName
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
                    text: listfile.songArtist
                    color: Theme.textColor
                    elide: Text.ElideRight
                    font.pixelSize: Style.textTip
                    verticalAlignment: Text.AlignVCenter
                    visible: listfile.songArtist !== ""
                }

                Row {
                    anchors.right: parent.right
                    anchors.rightMargin: 10
                    opacity: playList.previewing ? 1
                             : (playListArea.containsMouse || playDelete.hovered ? 0 : 1)
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
                        if (playList.previewing) return
                        Options.queue.playListIndex = listfile.index
                        Playback.startTrack(listfile.index)
                    }

                    Rectangle {
                        id: listHover
                        anchors.fill: parent
                        radius: Style.labelRadius
                        color: Theme.hoverColor
                        opacity: 0
                        visible: opacity > 0
                        z: 2
                        Behavior on opacity { NumberAnimation { duration: 80 } }
                        SButton {
                            id: playNext
                            visible: !playList.previewing
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
                                // 已在队列里就走移动，moveTo 会收紧越界下标并带着当前播放行一起挪
                                Options.queue.moveTo(listfile.index, cur + 1)
                                Options.warned("已设为下一首", 1)
                            }
                        }
                        SButton {
                            id: playMenu
                            visible: !playList.previewing
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
                                Options.favPicker.show({ id: listfile.path, title: listfile.name,
                                                         artist: listfile.songer, cover: "",
                                                         source: listfile.source, duration: 0 })
                            }
                        }
                        SButton {
                            id: playDelete
                            visible: !playList.previewing
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
                                // 下标由模型自己维护，这里不再手动加减
                                if (Options.queue.playListIndex !== listfile.index)
                                    Options.queue.remove(listfile.index, 1);
                            }
                        }
                    }
                }
            }
        }
    }

    QAlertDialog {
        id: savePlaylistDialog
        title: "保存播放列表"
        message: "给当前播放列表起一个名字："
        isInput: true
        onConfirm: {
            const playlistName = input.trim()
            if (playlistName === "") {
                Options.warned("请输入名称", 0)
                return
            }
            Options.queue.savePlaylist(playlistName)
        }
    }

    QAlertDialog {
        id: renamePlaylistDialog
        title: "重命名播放列表"
        message: "输入新的名称："
        isInput: true
        property int playlistId: -1
        onConfirm: {
            const playlistName = input.trim()
            if (playlistName === "") {
                Options.warned("请输入名称", 0)
                return
            }
            Options.queue.renamePlaylist(playlistId, playlistName)
        }
    }

    enter: Transition {
        NumberAnimation { property: "x"; duration: 450; from: playList.parent.width; to: playList.parent.width - 380; easing.type: Easing.OutExpo }
    }
    exit: Transition {
        NumberAnimation { property: "x"; duration: 240; to: playList.parent.width; easing.type: Easing.OutCubic }
    }
}
