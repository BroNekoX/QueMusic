// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// 插件市场：推荐 / 最新 / 歌词界面 / 功能 / 音乐源
pragma ComponentBehavior: Bound
import QtQuick
import QueMusic 1.0

Item {
    id: page

    readonly property var chips: ["推荐", "最新", "歌词界面", "功能", "音乐源"]
    readonly property bool compact: width < 748
    readonly property string message: PluginMarket.error.length > 0 ? PluginMarket.error
                                      : PluginMarket.busy ? PluginMarket.status
                                      : (PluginMarket.status.length > 0 ? PluginMarket.status
                                                                        : "点右上角刷新获取插件列表")

    Component.onCompleted: {
        PluginMarket.reloadConfig();
        PluginMarket.refresh();
    }

    Connections {
        target: PluginMarket
        function onInstalled(id) { Options.warned("插件已安装", 1); }
        function onFailed(id, text) { Options.warned(text, 0); }
    }

    Row {
        id: toolbar
        y: 12
        anchors.right: parent.right
        spacing: 8

        QButton {
            height: 36
            text: page.compact ? "" : "刷新"
            iconCharacter: "\uf10c"
            enabled: !PluginMarket.busy
            buttonColor: Theme.secondaryColor
            borderColor: Theme.sideColor
            borderWidth: 1
            onClicked: PluginMarket.refresh();
        }
        QButton {
            height: 36
            text: page.compact ? "" : "市场设置"
            iconCharacter: "\uf0f7"
            buttonColor: Theme.secondaryColor
            borderColor: Theme.sideColor
            borderWidth: 1
            onClicked: serverDialog.open();
        }
    }

    Row {
        id: chipsRow
        x: 0
        y: 72
        spacing: 6

        Repeater {
            model: page.chips

            delegate: Rectangle {
                id: chip
                required property string modelData
                required property int index
                readonly property bool active: PluginMarket.category === chip.index

                width: Math.max(64, chipLabel.implicitWidth + 24)
                height: 30
                radius: 15
                color: chip.active ? Theme.themeColor : Theme.primaryColor
                border.width: 1
                border.color: Theme.sideColor

                Text {
                    id: chipLabel
                    anchors.centerIn: parent
                    text: chip.modelData
                    color: chip.active ? Theme.fullColor : Theme.textColor
                    font.pixelSize: Style.text
                }
                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: PluginMarket.category = chip.index
                }
            }
        }
    }

    Text {
        id: statusText
        x: page.width - width - 24
        y: 72
        height: 30
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        color: PluginMarket.error.length > 0 ? "#ff7b7b" : Theme.textColor
        font.pixelSize: Style.textTip
        text: page.message
    }

    ListView {
        id: listView
        anchors.top: chipsRow.bottom
        anchors.topMargin: 12
        width: parent.width + 18
        rightMargin: 18
        bottomMargin: 24
        anchors.bottom: parent.bottom
        clip: true
        spacing: 8
        model: PluginMarket.list
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
            width: ListView.view.width - 18
            required property var modelData

            readonly property bool installed: modelData.installed
            readonly property bool updatable: modelData.updatable
            readonly property string meta: (modelData.author ? modelData.author : "未知作者")
                                           + (modelData.version ? "  v" + modelData.version : "")
                                           + "  ·  " + card.typeLabel()
            readonly property string action: PluginMarket.installingId === modelData.id ? PluginMarket.status
                                             : !installed ? "安装"
                                             : updatable ? "更新" : "已安装"

            function typeLabel(): string {
                return modelData.type === "lyrics" ? "歌词界面" : modelData.type === "source" ? "音乐源" : "功能";
            }

            height: 92
            radius: Style.cubeRadius
            color: installed ? Theme.containColor : Theme.primaryColor
            border.width: 1
            border.color: installed ? Theme.themeColor : Theme.sideColor

            QPicture {
                x: 12
                y: 12
                width: 68
                height: 68
                radius: 10
                image: card.modelData.preview ? card.modelData.preview : ""
            }

            Column {
                x: 92
                y: 14
                width: card.width - 220
                spacing: 4

                Text {
                    width: parent.width
                    text: card.modelData.name
                    elide: Text.ElideRight
                    color: Theme.fontColor
                    font.pixelSize: Style.textmain
                    font.bold: true
                }
                Text {
                    width: parent.width
                    text: card.meta
                    elide: Text.ElideRight
                    color: Theme.textColor
                    font.pixelSize: Style.textTip
                }
                Text {
                    width: parent.width
                    text: card.modelData.description
                    elide: Text.ElideRight
                    color: Theme.textColor
                    opacity: 0.8
                    font.pixelSize: Style.textTip
                }
            }

            QButton {
                anchors.right: parent.right
                anchors.rightMargin: 16
                anchors.verticalCenter: parent.verticalCenter
                width: 96
                height: 34
                text: card.action
                enabled: !PluginMarket.busy && !(card.installed && !card.updatable)
                iconCharacter: card.installed ? "\uf099" : "\uf00f"
                onClicked: PluginMarket.install(card.modelData.id)
            }
        }
    }

    Text {
        anchors.centerIn: listView
        visible: !PluginMarket.busy && listView.count === 0 && PluginMarket.error.length === 0
        text: "这个分类下还没有插件"
        color: Theme.textColor
        opacity: 0.7
        font.pixelSize: Style.text
    }
}