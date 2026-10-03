// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors

// 插件管理页：歌词界面插件（单选）与功能插件（可多选）共用。
// store 传插件单例：plugins / dir / selectedId / enabledIds / rescan() / install() / remove() / reveal()
import QtQuick
import QtQuick.Dialogs
import QueMusic 1.0

Item {
    id: manager

    property var store: null
    property bool multi: false              // false=同时只用一个，true=可同时启用多个
    property string pluginKind: "插件"      // 删除确认与安装标题的文案

    readonly property var plugins: store ? store.plugins : []
    readonly property bool compact: width < 648

    Row {
        id: toolbar
        y: 12
        anchors.right: parent.right
        spacing: 8

        QButton {
            height: 36
            text: manager.compact ? "" : "重新扫描"
            iconCharacter: "\uf021"
            buttonColor: Style.secondaryColor
            borderColor: Style.sideColor
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
            buttonColor: Style.secondaryColor
            borderColor: Style.sideColor
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
        color: Style.textColor
        font.pixelSize: Style.settings.textTip
        text: "插件目录：" + (manager.store ? manager.store.dir : "")
              + "（每个插件一个文件夹，文件夹名即插件 id，内含 info.json 与入口 QML）。"
              + (manager.multi
                 ? "可同时启用多个，插件界面出现在标题栏、底栏等扩展点上；插件出错会自动停用。"
                 : "安装后点「启用」生效，也可在播放页左上角第二个按钮切换。")
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
            required property string id
            required property string name
            required property bool builtin
            required property string preview
            required property string description
            required property string author
            required property string version
            width: listView.width
            height: 92
            radius: Style.settings.cubeRadius
            color: row.isOn ? Style.containColor : Style.primaryColor
            border.width: 1
            border.color: row.isOn ? Style.themeColor : Style.sideColor

            readonly property bool isOn: manager.store === null ? false
                                        : manager.multi
                                          ? manager.store.enabledIds.indexOf(row.id) >= 0
                                          : manager.store.selectedId === row.id

            QPicture {
                x: 12
                y: 12
                width: 68
                height: 68
                radius: 10
                source: row.preview ? row.preview
                                          : "qrc:/QueMusic/resources/app/musicpic.png"
            }

            Text {
                x: 92
                y: 18
                width: row.width - 300
                text: row.name
                elide: Text.ElideRight
                color: Style.fontColor
                font.pixelSize: Style.settings.textmain
                font.bold: true
            }
            Text {
                x: 92
                y: 40
                width: row.width - 300
                elide: Text.ElideRight
                color: Style.textColor
                font.pixelSize: Style.settings.textTip
                text: (row.builtin ? "内置" : (row.author ? row.author : "未知作者"))
                      + (row.version ? "  v" + row.version : "")
                      + (row.id ? "  " + row.id : "")
            }
            Text {
                x: 92
                y: 62
                width: row.width - 300
                elide: Text.ElideRight
                color: Style.textColor
                font.pixelSize: Style.settings.textTip
                opacity: 0.8
                text: row.description
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
                    onClicked: manager.store.selectedId = row.id
                }
                QSwitch {
                    visible: manager.multi
                    width: 96
                    height: 32
                    switchTrue: row.isOn
                    onToggled: manager.store.setEnabled(row.id, !row.isOn)
                }
                SButton {
                    iconCharacter: "\uf0f5"
                    width: 34
                    height: 34
                    radius: 10
                    buttonColor: "transparent"
                    shadowEnabled: false
                    tipText: "打开插件目录"
                    onClicked: manager.store.reveal(row.id)
                }
                SButton {
                    visible: !row.builtin
                    iconCharacter: "\uf08e"
                    width: 34
                    height: 34
                    radius: 10
                    buttonColor: "transparent"
                    hoverColor: Qt.rgba(1.0, 0.5, 0.5, 0.8)
                    shadowEnabled: false
                    tipText: "删除插件"
                    onClicked: {
                        const pluginId = row.id;
                        Options.dialog.openSimpleDialog("删除插件",
                            "将删除" + manager.pluginKind + "「" + row.name + "」，是否继续？", function() {
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
        title: "选择" + manager.pluginKind + "文件夹"
        onAccepted: {
            const err = manager.store.install(selectedFolder);
            if (err) Style.warned(err, 0);
            else Style.warned(manager.multi ? "插件已安装并启用" : "插件已安装，点「启用」即可使用", 1);
        }
    }
}
