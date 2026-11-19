// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// 音源插件管理：直接读取 sources 目录下的 .js，支持开关与 .js 导入。
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Dialogs
import QueMusic 1.0

Item {
    id: manager
    property Item blurSource
    readonly property var plugins: MusicApi.sourcePlugins

    ListView {
        id: listView
        x: 0
        y: 60
        width: parent.width + 18
        height: parent.height - y
        rightMargin: 18
        bottomMargin: 24
        model: manager.plugins
        spacing: 8
        clip: true
        property int scrollToY: listView.contentY
        WheelHandler {
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            readonly property real wheelHeightCount: Theme.wheelStep
            readonly property int scrollBottom: listView.contentHeight - listView.height + listView.bottomMargin + listView.originY
            onWheel: (event) => {
                listViewAnime.running = false;
                listView.scrollToY = Math.max(listView.originY - listView.topMargin, Math.min( listView.scrollToY - (event.angleDelta.y * wheelHeightCount), scrollBottom));
                viewBar.active = true;
                event.accepted = true;
                listViewAnime.running = true;
            }
        }

        NumberAnimation {
            id: listViewAnime
            target: listView
            property: "contentY"
            duration: 240
            to: listView.scrollToY
            easing.type: Easing.OutCubic
            onFinished: viewBar.active = false
        }
        QScrollBar.vertical: QScrollBar {
            id: viewBar
            parent: listView
            anchors.top: listView.top
            anchors.right: listView.right
            anchors.bottom: listView.bottom
            onPressedChanged: {
                listView.scrollToY = listView.contentY
            }
        }

        delegate: Rectangle {
            id: card
            required property var modelData
            height: 68
            radius: Style.labelRadius
            color: Theme.primaryColor
            border.color: Theme.sideColor
            border.width: 1
            width: ListView.view.width - 18

            Text {
                x: 20
                y: 14
                text: card.modelData.name || card.modelData.id
                color: Theme.fontColor
                font.pixelSize: Style.textmain
            }
            Text {
                x: 20
                y: 42
                width: parent.width - 200
                text: (card.modelData.description || card.modelData.file || "")
                color: Theme.textColor
                font.pixelSize: Style.textTip
                elide: Text.ElideRight
            }
            Row {
                anchors.right: parent.right
                anchors.rightMargin: 16
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8
                QSwitch {
                    width: 88
                    height: 32
                    switchTrue: card.modelData.enabled === true
                    onToggled: MusicApi.setSourcePluginEnabled(card.modelData.id, !card.modelData.enabled)
                }
                SButton {
                    width: 34
                    height: 34
                    radius: 10
                    iconCharacter: "\uf08e"
                    buttonColor: "transparent"
                    tipText: "删除"
                    onClicked: {
                        const error = MusicApi.removeSourcePlugin(card.modelData.id)
                        if (error)
                            Options.warned(error, 2)
                        else
                            Options.warned("已删除音源插件", 1)
                    }
                }
            }
        }
    }

    Text {
        visible: manager.plugins.length === 0
        anchors.centerIn: parent
        text: "还没有音源插件，点右上角导入 .js"
        color: Theme.textColor
        font.pixelSize: Style.textmain
    }

    Row {
        anchors.right: parent.right
        y: 12
        spacing: 8
        QButton {
            height: 36
            text: "导入 JS"
            iconCharacter: "\uf10d"
            onClicked: jsDialog.open()
        }
        QButton {
            height: 36
            text: "打开目录"
            iconCharacter: "\uf0f5"
            buttonColor: Theme.secondaryColor
            borderColor: Theme.sideColor
            borderWidth: 1
            onClicked: MusicApi.revealSourcePlugins()
        }
    }

    FileDialog {
        id: jsDialog
        nameFilters: ["音源插件 (*.js)"]
        onAccepted: {
            const error = MusicApi.importSourcePlugin(selectedFile)
            if (error)
                Options.warned(error, 2)
            else
                Options.warned("已导入音源插件", 1)
        }
    }

    QButton {
        width: 80
        height: 40
        x: parent.width - 80
        y: parent.height - 64
        radius: 20
        buttonColor: Theme.containColor
        borderWidth: 1
        iconCharacter: "\uf11a"
        text: "提示"
        onClicked: {
            sourceDialog.open();
        }
    }

    QAlertDialog {
        id: sourceDialog
        title: "注意"
        message: "QueMusic音源插件系统使用自研引擎，基于JavaScript语言，提供音源播放地址接口。可以将其他平台例如洛雪音乐的音源插件转换为QueMusic音源插件。建议复制处理文本并附上其他平台插件发送给任意AI Agent处理"
        isInput: false
        blurSource: manager.blurSource
        confirmText: "复制并前往"
        cancelText: "复制"
        TextInput {
            id: agentText
            text: "你好，这是我的音源插件，请你把他转换为QueMusic专用音源插件并交付一个js文件，这是文档地址：https://quemusic.top/pluginsdoc.txt"
            visible: false
        }

        onConfirm: {
            //agentText.selectAll();
            //agentText.copy();
            App.copyText("你好，这是我的音源插件，请你把他转换为QueMusic专用音源插件并交付一个js文件，这是文档地址：https://quemusic.top/pluginsdoc.txt");
            Qt.openUrlExternally("https://yuanbao.tencent.com/");
        }
        onCancel: {
            //agentText.selectAll();
            //agentText.copy();
            App.copyText("你好，这是我的音源插件，请你把他转换为QueMusic专用音源插件并交付一个js文件，这是文档地址：https://quemusic.top/pluginsdoc.txt");
        }
    }
}
