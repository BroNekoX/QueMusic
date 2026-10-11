// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// 歌曲 / 歌单信息弹窗：全应用只此一份（原先每个 QListView 各带一个 QOptionDialog）。
// 字段名各平台不统一，归一化在各平台的 C++ 层完成，这里只做展示。
pragma ComponentBehavior: Bound
import QtQuick
import QueMusic 1.0

QOptionDialog {
    id: root
    property bool isList: false
    property var info

    readonly property string sourceName: {
        const names = ["酷狗音乐", "网易云音乐", "哔哩哔哩", "QQ音乐", "自定义源"]
        return (info && names[info.source] !== undefined) ? names[info.source] : "未知来源"
    }
    readonly property list<var> rows: {
        if (!info) return []
        if (isList)
            return [["名称", info.title || info.name || ""], ["创建者", info.artist || info.singer || ""],
                    ["曲目", info.duration > 0 ? info.duration + " 首" : "—"], ["来源", sourceName]]
        const sec = info.duration || 0
        return [["歌手", info.artist || info.singer || ""], ["专辑", info.album || "—"],
                ["时长", Math.floor(sec / 60) + ":" + ("0" + Math.floor(sec % 60)).slice(-2)],
                ["音质", ["标准", "无损", "Hi-Res"][Config.soundQuality] || "标准"], ["来源", sourceName]]
    }

    title: isList ? "歌单信息" : "歌曲信息"
    confirmText: "关闭"

    function show(item: var, list: bool): void {
        info = item
        isList = list
        open()
    }

    options: Column {
        width: parent.width
        spacing: 14
        Row {
            width: parent.width
            spacing: 14
            QPicture {
                width: 72
                height: 72
                radius: 14
                source: root.info && root.info.cover ? String(root.info.cover).replace("{size}", "160")
                                                     : "qrc:/QueMusic/resources/app/musicpic.png"
                sourceSize: Qt.size(160, 160)
            }
            Column {
                width: parent.width - 86
                anchors.verticalCenter: parent.verticalCenter
                spacing: 6
                Text {
                    width: parent.width
                    text: root.info ? (root.info.title || root.info.name || "") : ""
                    font.pixelSize: 16
                    font.weight: Font.DemiBold
                    color: Theme.fontColor
                    elide: Text.ElideRight
                }
                Text {
                    width: parent.width
                    text: root.info ? (root.info.artist || root.info.singer || "") : ""
                    font.pixelSize: 12
                    color: Theme.textColor
                    elide: Text.ElideRight
                }
            }
        }
        Repeater {
            model: root.rows
            delegate: Row {
                id: infoRow
                required property var modelData
                width: root.width - 40
                height: 22
                Text {
                    width: 72
                    height: 22
                    text: infoRow.modelData[0]
                    font.pixelSize: 12
                    color: Theme.textColor
                    verticalAlignment: Text.AlignVCenter
                }
                Text {
                    width: parent.width - 72
                    height: 22
                    text: infoRow.modelData[1]
                    font.pixelSize: 12
                    color: Theme.fontColor
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }
}
