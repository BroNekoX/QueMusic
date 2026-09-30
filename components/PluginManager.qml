// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// 插件管理页：歌词界面插件（单选）与功能插件（可多选）共用一套列表 UI。
// store 传插件单例，两者接口一致：plugins / dir / rescan() / install() / remove() / reveal()；
// 单选模式额外用 selectedId，多选模式用 enabledIds + setEnabled()。
import QtQuick
import QtQuick.Dialogs
import QueMusic 1.0

Item {
    id: manager

    property var store: null
    property bool multi: false            // false=同时只启用一个，true=可同时启用多个
    property bool compact: false          // 窄宽度下按钮只留图标
    property string hint: ""
    property string installTitle: "选择插件文件夹"
    property string pluginKind: "插件"    // 删除确认文案用

    readonly property var plugins: store ? store.plugins : []

    Row {
        id: toolbar
        y: 12
        anchors.right: parent.right
        spacing: 8

        QButton {
            height: 36
            text: manager.compact ? "" : "重新扫描"
            iconCharacter: "\uf021"
            buttonColor: Style.themes.secondaryColor
            borderColor: Style.themes.sideColor
            borderWidth: 1
            onClicked: manager.store.rescan()
        }
        QButton {
            height: 36
            text: manager.compact ? "" : "安装插件"
            iconCharacter: "\uf067"
            onClicked: pluginFolderDialog.open()
        }
        QButton {
            height: 36
            text: manager.compact ? "" : "打开插件目录"
            iconCharacter: "\uf0f5"
            buttonColor: Style.themes.secondaryColor
            borderColor: Style.themes.sideColor
            borderWidth: 1
            onClicked: manager.store.reveal("")
        }
    }

    Text {
        id: hintText
        x: 24
        y: 62
        width: manager.width - 48
        wrapMode: Text.Wrap
        color: Style.themes.textColor
        font.pixelSize: Style.settings.textTip
        text: manager.hint
    }

    ListView {
        id: listView
        anchors.top: hintText.bottom
        anchors.topMargin: 12
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 24
        clip: true
        spacing: 8
        model: manager.plugins

        delegate: Rectangle {
            id: row
            width: listView.width
            height: 92
            radius: Style.settings.cubeRadius
            color: row.isOn ? Style.themes.containColor : Style.themes.primaryColor
            border.width: 1
            border.color: row.isOn ? Style.themes.themeColor : Style.themes.sideColor

            // 启用状态：多选看 enabledIds、单选看 selectedId（都是带通知的属性，状态变了会重新求值）
            readonly property bool isOn: manager.store === null ? false
                                        : manager.multi
                                          ? manager.store.enabledIds.indexOf(modelData.id) >= 0
                                          : manager.store.selectedId === modelData.id

            QPicture {
                x: 12
                y: 12
                width: 68
                height: 68
                radius: 10
                source: modelData.preview ? modelData.preview
                                          : "qrc:/QueMusic/resources/app/musicpic.png"
            }

            Text {
                x: 92
                y: 18
                width: row.width - 300
                text: modelData.name
                elide: Text.ElideRight
                color: Style.themes.fontColor
                font.pixelSize: Style.settings.textmain
                font.bold: true
            }
            Text {
                x: 92
                y: 40
                width: row.width - 300
                elide: Text.ElideRight
                color: Style.themes.textColor
                font.pixelSize: Style.settings.textTip
                text: (modelData.builtin ? "内置" : (modelData.author ? modelData.author : "未知作者"))
                      + (modelData.version ? "  v" + modelData.version : "")
                      + (modelData.id ? "  " + modelData.id : "")
            }
            Text {
                x: 92
                y: 62
                width: row.width - 300
                elide: Text.ElideRight
                color: Style.themes.textColor
                font.pixelSize: Style.settings.textTip
                opacity: 0.8
                text: modelData.description
            }

            Row {
                anchors.right: parent.right
                anchors.rightMargin: 16
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8

                QButton {
                    visible: !manager.multi
                    width: 96
                    height: 34
                    text: row.isOn ? "使用中" : "启用"
                    enabled: !row.isOn
                    iconCharacter: "\uf0e7"
                    onClicked: manager.store.selectedId = modelData.id
                }
                QSwitch {
                    visible: manager.multi
                    width: 64
                    height: 32
                    switchTrue: row.isOn
                    onToggled: manager.store.setEnabled(modelData.id, !row.isOn)
                }
                SButton {
                    iconCharacter: "\uf0f5"
                    width: 34
                    height: 34
                    radius: 10
                    buttonColor: "transparent"
                    shadowEnabled: false
                    tipText: "打开插件目录"
                    onClicked: manager.store.reveal(modelData.id)
                }
                SButton {
                    visible: !modelData.builtin
                    iconCharacter: "\uf08e"
                    width: 34
                    height: 34
                    radius: 10
                    buttonColor: "transparent"
                    hoverColor: Qt.rgba(1.0, 0.5, 0.5, 0.8)
                    shadowEnabled: false
                    tipText: "删除插件"
                    onClicked: {
                        const pluginId = modelData.id;
                        globalDialog.openSimpleDialog("删除插件",
                            "将删除" + manager.pluginKind + "「" + modelData.name + "」，是否继续？", function() {
                            const err = manager.store.remove(pluginId);
                            if (err) Style.warned(err, 0);
                            else Style.warned("已删除插件", 1);
                        });
                    }
                }
            }
        }
    }

    FolderDialog {
        id: pluginFolderDialog
        title: manager.installTitle
        onAccepted: {
            const err = manager.store.install(selectedFolder);
            if (err) Style.warned(err, 0);
            else Style.warned(manager.multi ? "插件已安装并启用" : "插件已安装，点「启用」即可使用", 1);
        }
    }
}
