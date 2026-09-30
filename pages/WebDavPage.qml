// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// WebDAV 分页：服务器列表 + 目录浏览（视觉与「我的文件夹 / 本地文件夹」一致）
import QtQuick
import QtQuick.Controls.Basic
import QueMusic 1.0
import 'qrc:/QueMusic/components'

Item {
    id: root

    // 宿主注入：与 pages/FilePage.qml 一致，播放引擎不靠上下文继承
    readonly property AudioEngine player: Playback.player

    property bool browsing: false        // false = 服务器列表，true = 目录浏览
    property string serverName: ""
    property string editingId: ""        // 非空表示编辑既有服务器

    WebDavModel { id: browser }

    function openServer(server: var): void {
        root.serverName = server.name || server.url
        root.browsing = true
        browser.openServer(server.id, WebDav.authHeader(server.id), server.url)
    }

    function openForm(server: var): void {
        root.editingId = server ? server.id : ""
        fieldName.inputText = server ? (server.name || "") : ""
        fieldUrl.inputText = server ? (server.url || "") : ""
        fieldUser.inputText = server ? WebDav.userOf(server.id) : ""
        fieldPass.inputText = ""
        serverDialog.open()
    }

    function submitForm(): void {
        const url = fieldUrl.inputText.trim()
        if (!url) {
            Style.warned("请填写服务器地址", 0)
            serverDialog.open();         // 校验不过就留在弹窗里
            return
        }
        if (root.editingId)
            WebDav.updateServer(root.editingId, fieldName.inputText, url,
                                fieldUser.inputText, fieldPass.inputText)
        else
            WebDav.addServer(fieldName.inputText, url, fieldUser.inputText, fieldPass.inputText)
    }

    function deleteServer(server: var): void {
        globalDialog.openSimpleDialog("删除", "将移除该 WebDAV 服务器，是否删除？", function() {
            WebDav.removeServer(server.id)
            Style.warned("已移除服务器", 1)
        })
    }

    function playRow(index: int): void {
        const row = browser.at(index)
        if (!row.url || row.isDir)
            return
        WebDav.rememberSidecars(row.url, row.lyricsUrl, row.coverUrl)
        Playback.playWebDav(row.url, row.title)
    }

    // 右侧操作区
    Row {
        x: parent.width - width - 16
        y: 11
        z: 2
        spacing: 8
        QButton {
            height: 38
            text: root.browsing ? "返回" : "新建服务器"
            iconCharacter: root.browsing ? "\uf112" : "\uf067"
            onClicked: {
                if (root.browsing)
                    root.browsing = false;
                else
                    root.openForm(null);
            }
        }
        QButton {
            height: 38
            visible: root.browsing
            enabled: browser.canGoUp
            text: "上一级"
            iconCharacter: "\uf062"
            onClicked: browser.goUp()
        }
        QButton {
            height: 38
            visible: root.browsing
            text: "刷新"
            iconCharacter: "\uf021"
            onClicked: browser.refresh()
        }
    }

    Connections {
        target: window
        function onExit(): void {
            browser.goUp();
        }
    }

    // 服务器列表
    QLocalView {
        id: serverView
        anchors.fill: parent
        topMargin: 60
        clip: true
        visible: !root.browsing
        model: WebDav.servers
        headerModel: ["名称","地址","","菜单"]
        menuModel: ["打开","编辑","删除"]

        rebound: Transition {
            NumberAnimation {
                properties: "y"
                duration: 480
                easing.type: Easing.Bezier
                easing.bezierCurve: [ 0.32, 0.12, 0.00, 1.00, 1, 1 ]
            }
        }

        onMenuClicked: (index, choice) => {
            const server = WebDav.servers[index]
            if (!server) return
            if (choice === 0)
                root.openServer(server)
            else if (choice === 1)
                root.openForm(server)
            else if (choice === 2)
                root.deleteServer(server)
        }

        delegate: Rectangle {
            id: listServer
            height: 64
            width: serverView.width - 16
            radius: Style.settings.labelRadius
            color: "#00000000"

            readonly property var server: modelData
            // 没填名称时用主机名占位，避免与地址列重复
            readonly property string title: {
                if (listServer.server.name && listServer.server.name !== listServer.server.url)
                    return listServer.server.name;
                return listServer.server.url.replace(/^https?:\/\//, "").replace(/\/$/, "");
            }

            Rectangle {
                anchors.fill: parent
                radius: Style.settings.labelRadius
                color: Style.themes.hoverColor
                opacity: serverArea.containsMouse ? 1 : 0
                z: 1
                Behavior on opacity { NumberAnimation { duration: 80 } }
            }

            Rectangle {
                y: 8
                x: 8
                z: 4
                width: 48
                height: 48
                color: Style.themes.containColor
                radius: 10
                Text {
                    anchors.fill: parent
                    text: "\uf0c2"
                    font.family: iconFont.name
                    font.pixelSize: Style.settings.texticon
                    color: Style.themes.fontColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }

            Label {
                x: 80
                z: 3
                width: parent.width / 2 - 108
                height: 64
                text: listServer.title
                color: Style.themes.fontColor
                font.bold: true
                elide: Text.ElideRight
                font.pixelSize: Style.settings.textmain
                verticalAlignment: Text.AlignVCenter
            }
            Label {
                x: parent.width / 2 - 24
                z: 2
                width: parent.width / 2 - 120
                height: 60
                text: listServer.server.url
                color: Style.themes.textColor
                font.pixelSize: Style.settings.textTip
                elide: Text.ElideMiddle
                verticalAlignment: Text.AlignVCenter
            }

            MouseArea {
                id: serverArea
                anchors.fill: parent
                hoverEnabled: true
                onClicked: root.openServer(listServer.server)

                Row {
                    anchors.right: parent.right
                    anchors.rightMargin: 20
                    spacing: 2
                    z: 2
                    y: 12
                    height: 36
                    SButton {
                        iconCharacter: "\uf005"
                        width: 36
                        height: 36
                        radius: 18
                        buttonColor: "transparent"
                        hoverColor: Qt.rgba(0.5,0.5,0.5,0.2)
                        shadowEnabled: false
                        tipText: "编辑服务器"
                        onClicked: root.openForm(listServer.server)
                    }
                    SButton {
                        iconCharacter: "\uf08e"
                        width: 36
                        height: 36
                        radius: 18
                        buttonColor: "transparent"
                        hoverColor: Qt.rgba(1.0,0.5,0.5,0.8)
                        shadowEnabled: false
                        tipText: "删除服务器"
                        onClicked: root.deleteServer(listServer.server)
                    }
                }
            }
        }

        Label {
            anchors.centerIn: parent
            visible: WebDav.servers.length === 0
            text: "还没有 WebDAV 服务器，点右上角「新建服务器」接入网盘音乐目录"
            color: Style.themes.textColor
            font.pixelSize: Style.settings.textmain
            opacity: 0.65
        }
    }

    // 目录浏览
    QLocalView {
        id: dirView
        anchors.fill: parent
        topMargin: 60
        clip: true
        visible: root.browsing
        reuseItems: false
        model: browser
        headerModel: ["名称","路径","","菜单"]
        menuModel: ["立即播放","下一首播放","添加到播放列表"]

        onMenuClicked: (index, choice) => {
            const row = browser.at(index)
            if (!row.url || row.isDir) return
            WebDav.rememberSidecars(row.url, row.lyricsUrl, row.coverUrl)
            const item = { name: row.title, path: row.url, songer: "", source: 3 }
            if (choice === 0) {
                Playback.playItem(item)
            } else if (choice === 1) {
                Playback.playNext(item)
                Style.warned("已设为下一首播放", 1)
            } else if (choice === 2) {
                const added = Playback.enqueue(item)
                Style.warned(added ? "已加入播放列表" : "已在播放列表中", added ? 1 : 0)
            }
        }

        delegate: Rectangle {
            id: listDir
            height: 60
            width: dirView.width - 16
            radius: Style.settings.labelRadius
            color: !model.isDir && player.source == model.fileUrl ? Style.themes.containColor : "transparent"
            Behavior on color { ColorAnimation { duration: 120 } }

            QPicture {
                y: 8
                x: 8
                z: 4
                width: 44
                height: 44
                visible: !model.isDir
                source: "qrc:/QueMusic/resources/app/musicpic.png"
                radius1: 10
                radius2: 10
                radius3: 10
                radius4: 10
            }

            Rectangle {
                y: 8
                x: 8
                z: 4
                width: 44
                height: 44
                visible: model.isDir
                color: Style.themes.containColor
                radius: 10
                Text {
                    anchors.fill: parent
                    text: "\uf0f5"
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
                color: Style.themes.hoverColor
                opacity: dirArea.containsMouse ? 1 : 0
                z: 1
                Behavior on opacity { NumberAnimation { duration: 80 } }
            }

            Label {
                height: 60
                x: 80
                z: 3
                width: parent.width / 2 - 108
                text: model.title || model.fileName
                color: Style.themes.fontColor
                font.bold: true
                font.pixelSize: Style.settings.textmain
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }
            Label {
                x: parent.width / 2 - 24
                height: 60
                z: 2
                width: parent.width / 2 - 120
                text: model.isDir ? "文件夹" : (model.fileSize > 0 ? (model.fileSize / 1048576).toFixed(1) + " MB" : "")
                color: Style.themes.textColor
                font.pixelSize: Style.settings.textTip
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }

            MouseArea {
                id: dirArea
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                onClicked: (mouse) => {
                    if (mouse.button === Qt.LeftButton) {
                        if (model.isDir)
                            browser.enter(index);
                        else
                            root.playRow(index);
                    } else {
                        dirView.menu.index = index;
                        dirView.menu.popup();
                    }
                }

                Row {
                    anchors.right: parent.right
                    anchors.rightMargin: 20
                    spacing: 2
                    z: 2
                    y: 12
                    height: 36
                    SButton {
                        iconCharacter: "\uf095"
                        width: 36
                        height: 36
                        radius: 18
                        visible: !model.isDir
                        buttonColor: "transparent"
                        hoverColor: Qt.rgba(0.5,0.5,0.5,0.2)
                        shadowEnabled: false
                        tipText: "加入播放列表"
                        onClicked: {
                            const row = browser.at(index)
                            if (!row.url) return
                            WebDav.rememberSidecars(row.url, row.lyricsUrl, row.coverUrl)
                            const added = Playback.enqueue({ name: row.title, path: row.url, songer: "", source: 3 })
                            Style.warned(added ? "已加入播放列表" : "已在播放列表中", added ? 1 : 0)
                        }
                    }
                }
            }
        }

        Label {
            anchors.centerIn: parent
            visible: !browser.busy && browser.count === 0
            text: browser.error !== "" ? browser.error : "该目录下没有音频"
            color: Style.themes.textColor
            font.pixelSize: Style.settings.textmain
            opacity: 0.65
        }
    }

    // 服务器表单
    QOptionDialog {
        id: serverDialog
        title: root.editingId ? "编辑服务器" : "新建服务器"
        cancelText: "取消"
        confirmText: "保存"
        onConfirm: root.submitForm()

        options: Column {
            width: parent.width
            spacing: 10

            Text {
                text: "名称（可留空）"
                color: Style.themes.textColor
                font.pixelSize: Style.settings.textmain
                opacity: 0.65
            }
            QInput {
                id: fieldName
                width: parent.width
                height: 36
            }

            Text {
                text: "服务器地址"
                color: Style.themes.textColor
                font.pixelSize: Style.settings.textmain
                opacity: 0.65
            }
            QInput {
                id: fieldUrl
                width: parent.width
                height: 36
            }

            Text {
                text: "账号"
                color: Style.themes.textColor
                font.pixelSize: Style.settings.textmain
                opacity: 0.65
            }
            QInput {
                id: fieldUser
                width: parent.width
                height: 36
            }

            Text {
                text: root.editingId ? "密码（留空表示不改）" : "密码"
                color: Style.themes.textColor
                font.pixelSize: Style.settings.textmain
                opacity: 0.65
            }
            QInput {
                id: fieldPass
                width: parent.width
                height: 36
                echoMode: TextInput.Password
                SButton {
                    width: 36
                    height: 36
                    radius: parent.radius
                    anchors.right: parent.right
                    buttonColor: "transparent"
                    iconCharacter: parent.echoMode === TextInput.Password ? "\uf001" : "\uf11f"
                    onClicked: {
                        if(parent.echoMode === TextInput.Password) {
                            parent.echoMode = TextInput.Normal;
                        } else {
                            parent.echoMode = TextInput.Password;
                        }
                    }
                }
            }
        }
    }
}
