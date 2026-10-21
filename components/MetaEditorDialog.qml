// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QueMusic 1.0

QOptionDialog {
    id: root
    title: "编辑元数据"
    confirmText: "保存"
    cancelText: "取消"

    signal saved(string filePath, int source)

    property string filePath: ""
    property int source: 0
    property var values: ({})
    property string coverPick: ""
    property string embeddedCover: ""
    property string lyrics: ""

    readonly property string coverPreview: {
        if (coverPick !== "")
            return coverPick
        if (embeddedCover !== "")
            return embeddedCover
        return "qrc:/QueMusic/resources/app/musicpic.png"
    }

    function edit(path: string, src: int): void {
        const info = MusicApi.readLocalAudioInfo(path)
        filePath = path
        source = src
        coverPick = ""
        embeddedCover = path !== "" ? (Options.coverHelper.findEmbeddedCover(path) || "") : ""
        values = {
            title: info.title || info.fileName.replace(/\.[^.]+$/, ""),
            artist: info.artist || "",
            album: info.album || "",
            genre: info.genre || "",
            year: info.year > 0 ? String(info.year) : "",
            track: info.track > 0 ? String(info.track) : ""
        }
        root.open()
    }

    onConfirm: {
        root.values.lyrics = root.lyrics
        root.values.cover = root.coverPick
        MusicApi.writeLocalMetadata(root.filePath, root.values)
    }

    options: Column {
        width: parent.width
        spacing: 12
        Row {
            width: parent.width
            height: 96
            spacing: 12
            QPicture {
                width: 96
                height: 96
                source: root.coverPreview
                radius1: 10
                radius2: 10
                radius3: 10
                radius4: 10
            }
            Column {
                width: parent.width - 108
                spacing: 8
                Text {
                    text: "封面"
                    color: Theme.textColor
                    font.pixelSize: Style.textmain
                    opacity: 0.65
                }
                QButton {
                    width: 108
                    height: 32
                    radius: 16
                    text: "选择图片"
                    onClicked: coverDialog.open()
                }
            }
        }
        Repeater {
            model: [
                { label: "标题", key: "title" },
                { label: "歌手", key: "artist" },
                { label: "专辑", key: "album" },
                { label: "流派", key: "genre" },
                { label: "年份", key: "year" },
                { label: "音轨号", key: "track" }
            ]
            delegate: Row {
                id: metaRow
                required property string key
                required property string label
                width: parent.width
                height: 36
                spacing: 12
                Text {
                    width: 56
                    height: parent.height
                    text: metaRow.label
                    color: Theme.textColor
                    font.pixelSize: Style.textmain
                    verticalAlignment: Text.AlignVCenter
                    opacity: 0.65
                }
                QInput {
                    width: parent.width - 68
                    height: 36
                    inputText: root.values[metaRow.key] !== undefined
                               ? String(root.values[metaRow.key]) : ""
                    onInputTextChanged: root.values[metaRow.key] = inputText
                }
            }
        }
        Text {
            text: "歌词（可带 [mm:ss.xx] 时间轴）"
            color: Theme.textColor
            font.pixelSize: Style.textmain
            opacity: 0.65
        }
        TextArea {
            id: lyricsArea
            width: parent.width
            color: Theme.fontColor
            font.pixelSize: Style.textmain
            wrapMode: TextEdit.Wrap
            selectByMouse: true
            onTextChanged: root.lyrics = text
            background: Rectangle {
                radius: Style.labelRadius
                color: Theme.fullColor
                border.width: 1
                border.color: Theme.sideColor
            }
        }
        Connections {
            target: root
            function onOpened(): void {
                lyricsArea.text = MusicApi.readLocalLyricsText(root.filePath)
            }
        }
    }

    FileDialog {
        id: coverDialog
        title: "选择封面图片"
        nameFilters: ["图片文件 (*.jpg *.jpeg *.png *.bmp *.gif)"]
        onAccepted: root.coverPick = selectedFile.toString()
    }

    Connections {
        target: MusicApi
        function onLocalMetadataSaved(filePath: string, ok: bool): void {
            if (filePath !== root.filePath)
                return
            if (!ok) {
                Options.warned("元数据保存失败，文件可能只读或格式不支持", 0)
                return
            }
            Options.warned("元数据已保存", 1)
            root.saved(filePath, root.source)
        }
    }
}
