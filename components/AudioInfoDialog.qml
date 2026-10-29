// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
pragma ComponentBehavior: Bound
import QtQuick
import QueMusic 1.0

QOptionDialog {
    id: root
    title: "音频详情"
    cancelText: ""
    confirmText: "关闭"
    property var rows: []

    function showInfo(path: string): void {
        const info = MusicApi.readLocalAudioInfo(path)
        const rows = []
        const add = (label, value) => {
            const text = value === undefined || value === null ? "" : String(value)
            if (text !== "" && text !== "0") rows.push({ label: label, value: text })
        }
        add("标题", info.title)
        add("歌手", info.artist)
        add("专辑", info.album)
        add("流派", info.genre)
        add("年份", info.year)
        add("音轨号", info.track)
        add("时长", info.duration > 0
            ? Math.floor(info.duration / 60) + ":" + String(info.duration % 60).padStart(2, "0") : "")
        add("格式", info.format)
        add("比特率", info.bitrate > 0 ? info.bitrate + " kbps" : "")
        add("采样率", info.sampleRate > 0 ? info.sampleRate + " Hz" : "")
        add("声道", info.channels)
        add("大小", info.size > 0 ? (info.size / 1048576).toFixed(2) + " MB" : "")
        add("文件名", info.fileName)
        root.rows = rows
        root.open()
    }

    options: Column {
        width: parent.width
        spacing: 8
        Repeater {
            model: root.rows
            delegate: Row {
                id: infoRow
                required property string label
                required property string value
                width: parent.width
                height: 24
                spacing: 12
                Text {
                    width: 68
                    height: parent.height
                    text: infoRow.label
                    color: Theme.textColor
                    font.pixelSize: Style.textmain
                    verticalAlignment: Text.AlignVCenter
                    opacity: 0.65
                }
                TextInput {
                    width: parent.width - 80
                    height: parent.height
                    text: infoRow.value
                    readOnly: true
                    selectByMouse: true
                    selectionColor: Theme.themeColor
                    color: Theme.fontColor
                    font.pixelSize: Style.textmain
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }
}
