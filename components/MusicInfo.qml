// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
import QtQuick
import QueMusic 1.0
import QtQuick.Controls.Basic

// 音乐详情侧栏：封面与标题固定在顶部，下方按分页切换（本地音乐没有评论页）
Popup {
    id: root
    padding: 0
    margins: -1
    parent: Overlay.overlay
    width: 360
    height: parent.height - 180
    x: parent.width - 380
    y: 80

    // 宿主注入：播放引擎不再靠上下文继承访问宿主的局部 id
    readonly property AudioEngine player: Playback.player
    // 本地音乐没有在线评论，直接不显示分页
    readonly property bool isLocal: !String(Playback.player.source).startsWith("http")
    readonly property real bodyY: isLocal ? 176 : 216

    function loadComments(): void {
        MusicApi.getComments(Playback.musicHash, 1, 30, Playback.musicSource)
    }
    // 只在真正看得见评论页时请求，换歌也会跟着重来
    function refreshComments(): void {
        if (root.visible && tabs.index === 1)
            MusicApi.comments.clear();
            root.loadComments();
    }

    onOpened: {
        tabs.index = 0
        MusicApi.comments.clear()
    }

    Connections {
        target: Playback
        function onMusicHashChanged(): void { root.refreshComments() }
    }

    background: QBlurCard {
        anchors.fill: parent
        borderRadius: Style.cubeRadius
        clip: false
        blurSource: Options.mainLayout
        shadowEffect: true
        rectXy: Qt.rect(root.x, root.y, 360, root.height)
    }

    contentItem: Item {
        anchors.fill: parent

        Text {
            y: 12
            x: 18
            height: 36
            text: "音乐详情"
            font.bold: true
            font.pixelSize: Style.textH2
            verticalAlignment: Text.AlignVCenter
            color: Theme.fontColor
        }
        SButton {
            iconCharacter: "\uf025"
            x: parent.width - 50
            y: 12
            width: 36
            height: 36
            radius: 36
            iconSize: Style.texticon + 2
            buttonColor: "transparent"
            shadowEnabled: false
            onClicked: root.close()
        }

        // 封面 + 标题 + 作者：固定不滚动
        Item {
            id: head
            x: 16
            y: 56
            width: root.width - 32
            height: 114
            QPicture {
                source: Playback.player.urlStr
                radius: 12
                width: 112
                height: 112
                MouseArea {
                    anchors.fill: parent
                    onClicked: Options.picWatch.dialog(Playback.player.urlStr || "qrc:/QueMusic/resources/app/musicpic.png",
                                               Playback.musicTitle)
                }
            }
            Text {
                x: 128
                y: 8
                width: parent.width - 128
                text: Playback.musicTitle
                font.pixelSize: 18
                font.bold: true
                elide: Text.ElideRight
                color: Theme.fontColor
            }
            Text {
                x: 128
                y: 42
                width: parent.width - 128
                text: Playback.musicArtist
                font.pixelSize: 16
                elide: Text.ElideRight
                color: Theme.textColor
            }
        }

        QTapBar {
            id: tabs
            x: 16
            y: 176
            height: 32
            model: ["歌曲信息", "评论"]
            visible: !root.isLocal
            onIndexChanged: root.refreshComments();
        }

        // 内容区：只创建当前分页，评论列表不切过去就不建
        Loader {
            id: infoPane
            x: 16
            y: root.bodyY
            width: root.width - 22
            height: root.height - root.bodyY - 12
            active: tabs.index === 0
            sourceComponent: infoPaneComponent
        }
        Loader {
            id: commentPane
            x: 16
            y: root.bodyY
            width: root.width - 22
            height: root.height - root.bodyY - 12
            active: tabs.index === 1 && !root.isLocal
            sourceComponent: commentPaneComponent
        }
    }

    Component {
        id: infoPaneComponent

        Flickable {
            id: view
            contentHeight: infoColumn.height
            contentWidth: width - 12
            boundsBehavior: Flickable.StopAtBounds
            clip: true
            synchronousDrag: true
            ScrollBar.vertical: ScrollBar {
                anchors.right: view.right
                anchors.top: view.top
                anchors.bottom: view.bottom
            }
            Column {
                id: infoColumn
                width: view.width - 10
                spacing: 16
                SettingItem {
                    label: "文件名："
                    controlWidth: 120
                    width: parent.width
                    TextInput {
                        height: 36
                        anchors.right: parent.right
                        font.pixelSize: Style.textmain
                        text: Playback.player.noTitle
                        color: Theme.textColor
                        verticalAlignment: Text.AlignVCenter
                        readOnly: true
                        selectByMouse: true
                        selectionColor: Theme.themeColor
                    }
                }
                SettingItem {
                    label: "歌曲名："
                    controlWidth: 120
                    width: parent.width
                    TextInput {
                        height: 36
                        anchors.right: parent.right
                        font.pixelSize: Style.textmain
                        text: Playback.musicTitle
                        color: Theme.textColor
                        readOnly: true
                        selectByMouse: true
                        selectionColor: Theme.themeColor
                        verticalAlignment: Text.AlignVCenter
                    }
                }
                SettingItem {
                    label: "艺术家："
                    controlWidth: 120
                    width: parent.width
                    TextInput {
                        height: 36
                        anchors.right: parent.right
                        font.pixelSize: Style.textmain
                        text: Playback.musicArtist
                        color: Theme.textColor
                        readOnly: true
                        selectByMouse: true
                        selectionColor: Theme.themeColor
                        verticalAlignment: Text.AlignVCenter
                    }
                }
                SettingItem {
                    label: "专辑："
                    controlWidth: 120
                    width: parent.width
                    TextInput {
                        height: 36
                        anchors.right: parent.right
                        font.pixelSize: Style.textmain
                        text: Playback.player.album
                        color: Theme.textColor
                        readOnly: true
                        selectByMouse: true
                        selectionColor: Theme.themeColor
                        verticalAlignment: Text.AlignVCenter
                    }
                }
                SettingItem {
                    label: "实际音质："
                    controlWidth: 120
                    width: parent.width
                    TextInput {
                        height: 36
                        anchors.right: parent.right
                        font.pixelSize: Style.textmain
                        text: Math.round(Playback.player.bitRate / 1000) + " kbps"
                        color: Theme.textColor
                        readOnly: true
                        selectByMouse: true
                        selectionColor: Theme.themeColor
                        verticalAlignment: Text.AlignVCenter
                    }
                }
                SettingItem {
                    label: "音频长度："
                    controlWidth: 120
                    width: parent.width
                    TextInput {
                        height: 36
                        anchors.right: parent.right
                        font.pixelSize: Style.textmain
                        text: Playback.player.duration.toString()
                        color: Theme.textColor
                        readOnly: true
                        selectByMouse: true
                        selectionColor: Theme.themeColor
                        verticalAlignment: Text.AlignVCenter
                    }
                }
                SettingItem {
                    label: "日期："
                    controlWidth: 120
                    width: parent.width
                    TextInput {
                        height: 36
                        anchors.right: parent.right
                        font.pixelSize: Style.textmain
                        text: Playback.player.mediaDate
                        color: Theme.textColor
                        readOnly: true
                        selectByMouse: true
                        selectionColor: Theme.themeColor
                        verticalAlignment: Text.AlignVCenter
                    }
                }
                SettingItem {
                    label: "音频格式："
                    controlWidth: 120
                    width: parent.width
                    TextInput {
                        height: 36
                        anchors.right: parent.right
                        font.pixelSize: Style.textmain
                        text: Playback.player.mediaType
                        color: Theme.textColor
                        readOnly: true
                        selectByMouse: true
                        selectionColor: Theme.themeColor
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }
    }

    Component {
        id: commentPaneComponent

        ListView {
            id: commentView
            clip: true
            model: MusicApi.comments
            spacing: 12
            topMargin: 4
            bottomMargin: 12
            reuseItems: true
            boundsBehavior: Flickable.StopAtBounds
            property int scrollToY: commentView.contentY
            property bool isEnd: false
            ScrollBar.vertical: ScrollBar {
                id: viewBar
                anchors.right: commentView.right
                anchors.top: commentView.top
                anchors.bottom: commentView.bottom
                onPressedChanged: {
                    commentView.scrollToY = commentView.contentY
                }
            }
            onAtYEndChanged: {
                if (atYEnd && !MusicApi.loadState && count !== 0) {
                    if(count % 30 === 0) {
                        MusicApi.getComments(Playback.musicHash, count / 30 + 1, 30, Playback.musicSource);
                    } else {
                        isEnd = true;
                    }
                }
            }
            WheelHandler {
                acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                readonly property real wheelHeightCount: Qt.application.styleHints.wheelScrollLines * 0.25
                readonly property int scrollBottom: commentView.contentHeight - commentView.height + commentView.bottomMargin + commentView.originY
                onWheel: (event) => {
                    listViewAnime.running = false;
                    commentView.scrollToY = Math.max(commentView.originY - commentView.topMargin, Math.min( commentView.scrollToY - (event.angleDelta.y * wheelHeightCount), scrollBottom));
                    viewBar.active = true;
                    event.accepted = true;
                    listViewAnime.running = true;
                }
            }

            NumberAnimation {
                id: listViewAnime
                target: commentView
                property: "contentY"
                duration: 240
                to: commentView.scrollToY
                easing.type: Easing.OutCubic
                onFinished: viewBar.active = false
            }
            delegate: Item {
                id: commentItem
                required property string user
                required property string avatar
                required property string content
                required property int liked
                width: ListView.view.width
                height: Math.max(34, body.height + 6)
                QPicture {
                    width: 30
                    height: 30
                    radius: 15
                    source: commentItem.avatar || "qrc:/QueMusic/resources/app/musicpic.png"
                }
                Column {
                    id: body
                    x: 40
                    width: commentItem.width - 40
                    spacing: 3
                    Text {
                        width: parent.width
                        text: commentItem.user
                              + (commentItem.liked > 0 ? "  ·  " + commentItem.liked + " 赞" : "")
                        color: Theme.textColor
                        font.pixelSize: Style.textmain - 2
                        elide: Text.ElideRight
                    }
                    Text {
                        width: parent.width
                        text: commentItem.content
                        color: Theme.fontColor
                        font.pixelSize: Style.textmain
                        wrapMode: Text.Wrap
                    }
                }
            }
            footer: Item {
                width: ListView.view.width
                height: 32
                visible: commentView.isEnd
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottom: parent.bottom
                    text: "没有更多了~"
                    color: Theme.textColor
                    font.pixelSize: Style.text
                }
            }
            Text {
                anchors.centerIn: parent
                width: commentView.width - 20
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                visible: commentView.count === 0
                // B 站/酷狗要两三次往返才拿到结果，这期间别说成"暂无评论"
                text: MusicApi.loadState ? "评论加载中…" : "暂无评论"
                color: Theme.textColor
                font.pixelSize: Style.text
            }
        }
    }

    enter: Transition {
        NumberAnimation { property: "x"; duration: 450; from: root.parent.width; to: root.parent.width - 380; easing.type: Easing.OutExpo }
    }
    exit: Transition {
        NumberAnimation { property: "x"; duration: 240; to: root.parent.width; easing.type: Easing.OutCubic }
    }
}
