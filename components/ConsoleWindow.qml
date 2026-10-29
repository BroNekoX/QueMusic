// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QueMusic 1.0

Window {
    id: root
    width: 920
    height: 620
    minimumWidth: 640
    minimumHeight: 420
    color: "#17181c"
    title: "QueMusic Console"
    visible: true

    readonly property Palette consolePalette: Palette {
        window: "#1b1c21"
        windowText: "#e6e8ee"
        base: "#22242a"
        alternateBase: "#272932"
        text: "#e6e8ee"
        brightText: "#9aa0ad"
        button: "#292b33"
        buttonText: "#e6e8ee"
        highlight: Theme.themeColor
        highlightedText: "#ffffff"
        placeholderText: "#7c8190"
        mid: "#343741"
        midlight: "#3d414c"
        dark: "#141519"
        light: "#3d414c"
    }
    readonly property string monoFont: Qt.platform.os === "windows" ? "Consolas"
                                       : Qt.platform.os === "osx" ? "Menlo" : "monospace"

    onClosing: Config.debug = false

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0; color: "#1e2026" }
            GradientStop { position: 1; color: "#17181c" }
        }
    }

    Rectangle {
        id: header
        width: parent.width
        height: 56
        color: "#202228"

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
            color: "#343741"
        }

        Row {
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            spacing: 10
            Text {
                text: "QueMusic Console"
                color: "#e6e8ee"
                font.pixelSize: 14
                font.bold: true
                font.family: Fonts.text
                anchors.verticalCenter: parent.verticalCenter
            }
            Rectangle {
                width: 1
                height: 14
                color: "#343741"
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                text: "v" + Application.version
                color: "#7c8190"
                font.pixelSize: 10
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        QTapBar {
            id: tabs
            anchors.right: parent.right
            anchors.rightMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            width: preferWidth
            height: 32
            model: ["日志", "命令", "信息"]
            textColor: "#9aa0ad"
            activeColor: Theme.themeColor
        }
    }

    StackLayout {
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        currentIndex: tabs.index

        LogPage { consolePalette: root.consolePalette; monoFont: root.monoFont }
        CommandPage { consolePalette: root.consolePalette; monoFont: root.monoFont }
        InfoPage { consolePalette: root.consolePalette; monoFont: root.monoFont }
    }

    component ConsoleButton: Button {
        id: btn
        property bool accent: false

        implicitHeight: 30
        implicitWidth: Math.max(62, label ? label.implicitWidth + 24 : 62)
        padding: 0

        contentItem: Text {
            id: label
            text: btn.text
            color: btn.accent ? "#ffffff" : "#e6e8ee"
            font.pixelSize: 12
            font.weight: Font.Medium
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        background: Rectangle {
            radius: 7
            color: btn.accent
                   ? (btn.pressed ? Qt.darker(Theme.themeColor, 1.15)
                                  : btn.hovered ? Qt.lighter(Theme.themeColor, 1.1) : Theme.themeColor)
                   : (btn.pressed ? "#3b3e48" : btn.hovered ? "#343741" : "#292b33")
            border.width: btn.visualFocus || !btn.accent ? 1 : 0
            border.color: btn.visualFocus ? "#e6e8ee" : "#3d414c"
            Behavior on color { ColorAnimation { duration: 100 } }
        }

        scale: btn.pressed ? 0.97 : 1
        Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutCubic } }
    }

    component LogPage: Pane {
        id: logPage
        required property Palette consolePalette
        required property string monoFont

        padding: 0
        palette: consolePalette
        background: null

        function lineColor(line) {
            if (line.indexOf("[警告]") !== -1) return "#f0c453"
            if (line.indexOf("[错误]") !== -1 || line.indexOf("[致命]") !== -1) return "#ff7373"
            if (line.indexOf("[调试]") !== -1) return "#98a0b3"
            return "#c8cdd8"
        }

        function copyLogs() {
            clipboard.text = LogManager.logPreview
            clipboard.selectAll()
            clipboard.copy()
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 14
            spacing: 10

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Rectangle {
                    Layout.preferredWidth: 7
                    Layout.preferredHeight: 7
                    radius: 3.5
                    color: LogManager.enabled ? "#67d99b" : "#6e7380"
                    Behavior on color { ColorAnimation { duration: 150 } }
                }
                Text {
                    Layout.fillWidth: true
                    text: "日志记录：" + (LogManager.enabled ? "已启用" : "已暂停") + " · 最近 500 行"
                    color: "#c8cdd8"
                    font.pixelSize: 12
                    elide: Text.ElideRight
                }
                ConsoleButton {
                    text: LogManager.enabled ? "暂停" : "启用"
                    onClicked: LogManager.enabled = !LogManager.enabled
                }
                ConsoleButton {
                    text: "打开目录"
                    onClicked: LogManager.openLogFolder()
                }
                ConsoleButton {
                    text: "复制"
                    onClicked: logPage.copyLogs()
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 10
                color: "#22242a"
                border.width: 1
                border.color: "#343741"

                ListView {
                    id: logList
                    anchors.fill: parent
                    anchors.margins: 10
                    clip: true
                    model: LogManager.logPreview ? LogManager.logPreview.split("\n") : []
                    spacing: 2
                    onContentHeightChanged: positionViewAtEnd()
                    ScrollBar.vertical: ScrollBar { }

                    delegate: Text {
                        required property string modelData
                        width: logList.width
                        text: modelData
                        color: logPage.lineColor(modelData)
                        font.family: logPage.monoFont
                        font.pixelSize: 12
                        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    }
                }

                Text {
                    anchors.centerIn: parent
                    visible: logList.count === 0
                    text: LogManager.enabled ? "暂无日志" : "日志记录已暂停"
                    color: "#7c8190"
                    font.pixelSize: 12
                }
            }
        }

        TextEdit { id: clipboard; visible: false }
    }

    component CommandPage: Pane {
        id: commandPage
        required property Palette consolePalette
        required property string monoFont

        padding: 0
        palette: consolePalette
        background: null
        property string output: ""

        function stringify(value) {
            if (value === undefined) return "undefined"
            if (typeof value === "string") return value
            try {
                return JSON.stringify(value)
            } catch (error) {
                return String(value)
            }
        }

        function runCode() {
            const code = editor.text.trim()
            if (code === "")
                return
            commandPage.output += "> " + code + "\n"
            try {
                // qmllint disable eval
                const result = eval(code)
                // qmllint enable eval
                commandPage.output += commandPage.stringify(result) + "\n\n"
            } catch (error) {
                commandPage.output += "错误: " + (error.message || error) + "\n\n"
            }
            Qt.callLater(function() {
                outputFlick.contentY = Math.max(0, outputFlick.contentHeight - outputFlick.height)
            })
        }

        function copyOutput() {
            clipboard.text = commandPage.output
            clipboard.selectAll()
            clipboard.copy()
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 14
            spacing: 10

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    text: "JavaScript"
                    color: "#e6e8ee"
                    font.pixelSize: 13
                    font.bold: true
                }
                Text {
                    Layout.fillWidth: true
                    text: "最后一条表达式的值会显示"
                    color: "#7c8190"
                    font.pixelSize: 10
                    elide: Text.ElideRight
                }
                ConsoleButton {
                    text: "清空"
                    onClicked: commandPage.output = ""
                }
                ConsoleButton {
                    text: "复制输出"
                    onClicked: commandPage.copyOutput()
                }
                ConsoleButton {
                    text: "运行"
                    accent: true
                    onClicked: commandPage.runCode()
                }
            }

            SplitView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                orientation: Qt.Vertical

                handle: Rectangle {
                    implicitHeight: 10
                    color: "transparent"

                    Rectangle {
                        anchors.centerIn: parent
                        width: 44
                        height: 4
                        radius: 2
                        color: "#3d414c"
                    }
                }

                Rectangle {
                    SplitView.preferredHeight: 300
                    SplitView.minimumHeight: 100
                    radius: 10
                    color: "#22242a"
                    border.width: 1
                    border.color: "#343741"

                    Item {
                        id: editorHeader
                        width: parent.width
                        height: 30

                        Text {
                            x: 12
                            anchors.verticalCenter: parent.verticalCenter
                            text: "脚本"
                            color: "#9aa0ad"
                            font.pixelSize: 10
                            font.bold: true
                        }
                        Text {
                            anchors.right: parent.right
                            anchors.rightMargin: 12
                            anchors.verticalCenter: parent.verticalCenter
                            text: "Ctrl+Enter 运行"
                            color: "#6e7380"
                            font.pixelSize: 10
                        }
                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            height: 1
                            color: "#2c2f38"
                        }
                    }

                    ScrollView {
                        anchors.top: editorHeader.bottom
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8
                        anchors.bottomMargin: 8
                        clip: true

                        TextArea {
                            id: editor
                            font.family: commandPage.monoFont
                            font.pixelSize: 13
                            color: "#e6e8ee"
                            placeholderText: "// Playback.togglePlay()\n// Playback.queue.count\n// LogManager.info(\"hi\", \"Console\")"
                            placeholderTextColor: "#7c8190"
                            selectionColor: commandPage.consolePalette.highlight
                            selectedTextColor: "#ffffff"
                            background: null
                            padding: 8
                            selectByMouse: true
                            wrapMode: TextArea.Wrap
                            Keys.onPressed: (event) => {
                                if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter)
                                        && (event.modifiers & Qt.ControlModifier)) {
                                    commandPage.runCode()
                                    event.accepted = true
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    SplitView.fillHeight: true
                    SplitView.minimumHeight: 90
                    radius: 10
                    color: "#22242a"
                    border.width: 1
                    border.color: "#343741"

                    Item {
                        id: outputHeader
                        width: parent.width
                        height: 30

                        Text {
                            x: 12
                            anchors.verticalCenter: parent.verticalCenter
                            text: "输出"
                            color: "#9aa0ad"
                            font.pixelSize: 10
                            font.bold: true
                        }
                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            height: 1
                            color: "#2c2f38"
                        }
                    }

                    Flickable {
                        id: outputFlick
                        anchors.top: outputHeader.bottom
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        anchors.bottomMargin: 10
                        clip: true
                        contentWidth: width
                        contentHeight: outputText.implicitHeight + 8
                        ScrollBar.vertical: ScrollBar { }

                        Text {
                            id: outputText
                            width: outputFlick.width - 14
                            text: commandPage.output
                            color: "#c8cdd8"
                            font.family: commandPage.monoFont
                            font.pixelSize: 12
                            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                        }
                    }

                    Text {
                        anchors.centerIn: parent
                        anchors.verticalCenterOffset: 15
                        visible: commandPage.output === ""
                        text: "运行结果会显示在这里"
                        color: "#6e7380"
                        font.pixelSize: 11
                    }
                }
            }
        }

        TextEdit { id: clipboard; visible: false }
    }

    component InfoPage: Pane {
        id: infoPage
        required property Palette consolePalette
        required property string monoFont
        readonly property AudioEngine player: Playback.player

        padding: 0
        palette: consolePalette
        background: null

        function queueItem() {
            const q = Playback.queue
            if (!q || q.playListIndex < 0 || q.playListIndex >= q.count)
                return null
            return q.get(q.playListIndex)
        }

        function sourceId() {
            const item = infoPage.queueItem()
            return item ? Number(item.source) : Playback.musicSource
        }

        function sourceName(id) {
            if (id < 0) return "本地文件"
            if (id === 0) return "酷狗音乐"
            if (id === 1) return "网易云音乐"
            if (id === 2) return "哔哩哔哩"
            if (id === Playback.kSourceWebDav) return "WebDAV"
            return "插件音源"
        }

        function sourceLabel() {
            return infoPage.player.onMedia ? infoPage.sourceName(infoPage.sourceId()) : "—"
        }

        function title() {
            return infoPage.player.onMedia
                    ? (infoPage.player.noTitle || Playback.musicTitle || "未知曲目")
                    : "未在播放"
        }

        function artist() {
            return infoPage.player.onMedia
                    ? (Playback.musicArtist || infoPage.player.artist || "未知歌手")
                    : ""
        }

        function stateText() {
            return ["已停止", "播放中", "已暂停"][infoPage.player.playbackState] || "未知"
        }

        function address() {
            return infoPage.player.source.toString() || "—"
        }

        function playbackId() {
            if (!infoPage.player.onMedia)
                return "—"
            const source = infoPage.sourceId()
            if (source < 0 || source === Playback.kSourceWebDav)
                return "—"
            return Playback.musicHash || "—"
        }

        function queuePosition() {
            const q = Playback.queue
            return q && q.count > 0 && q.playListIndex >= 0
                    ? (q.playListIndex + 1) + " / " + q.count
                    : "—"
        }

        function value(v, unit) {
            return v > 0 ? v + unit : "—"
        }

        readonly property var rows: [
            { label: "播放地址", value: infoPage.address(), mono: true },
            { label: "播放 ID", value: infoPage.playbackId(), mono: true },
            { label: "专辑", value: infoPage.player.album || "—" },
            { label: "音源", value: infoPage.sourceLabel() },
            { label: "队列位置", value: infoPage.queuePosition() },
            { label: "编码", value: infoPage.player.codecName || "—" },
            { label: "比特率", value: infoPage.value(infoPage.player.bitRate, " kbps") },
            { label: "采样率", value: infoPage.value(infoPage.player.sourceSampleRate, " Hz") },
            { label: "声道", value: infoPage.value(infoPage.player.sourceChannels, "") },
            { label: "输出采样率", value: infoPage.value(infoPage.player.outputSampleRate, " Hz") },
            { label: "缓冲", value: infoPage.value(infoPage.player.bufferMs, " ms") },
            { label: "音量", value: Math.round(Playback.volume * 100) + "%" },
            { label: "倍速", value: infoPage.player.playbackRate.toFixed(2) + "×" },
            { label: "版本", value: "v" + Options.version },
            { label: "日志文件", value: LogManager.currentLogFile || "—", mono: true }
        ]

        function copyInfo() {
            let text = infoPage.title() + " - " + infoPage.artist() + "\n"
            for (let i = 0; i < infoPage.rows.length; ++i)
                text += infoPage.rows[i].label + ": " + infoPage.rows[i].value + "\n"
            clipboard.text = text
            clipboard.selectAll()
            clipboard.copy()
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 14
            spacing: 10

            RowLayout {
                Layout.fillWidth: true

                Text {
                    text: "播放器信息"
                    color: "#e6e8ee"
                    font.pixelSize: 13
                    font.bold: true
                }
                Item { Layout.fillWidth: true }
                ConsoleButton {
                    text: "复制信息"
                    onClicked: infoPage.copyInfo()
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 96
                radius: 10
                color: "#22242a"
                border.width: 1
                border.color: "#343741"

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 14

                    Image {
                        Layout.preferredWidth: 64
                        Layout.preferredHeight: 64
                        source: infoPage.player.urlStr || "qrc:/QueMusic/resources/app/musicpic.png"
                        sourceSize.width: 64
                        sourceSize.height: 64
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 3

                        Text {
                            Layout.fillWidth: true
                            text: infoPage.title()
                            color: "#e6e8ee"
                            font.pixelSize: 18
                            font.bold: true
                            elide: Text.ElideRight
                        }
                        Text {
                            Layout.fillWidth: true
                            text: infoPage.artist()
                            color: "#9aa0ad"
                            font.pixelSize: 12
                            elide: Text.ElideRight
                        }
                        Text {
                            Layout.fillWidth: true
                            text: infoPage.stateText() + " · " + infoPage.sourceLabel()
                            color: "#7c8190"
                            font.pixelSize: 11
                            elide: Text.ElideRight
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 10
                color: "#22242a"
                border.width: 1
                border.color: "#343741"

                ScrollView {
                    id: infoScroll
                    anchors.fill: parent
                    anchors.margins: 10
                    clip: true

                    Column {
                        width: Math.max(0, infoScroll.availableWidth - 8)
                        spacing: 3

                        Repeater {
                            model: infoPage.rows
                            delegate: RowLayout {
                                id: row
                                required property var modelData
                                width: parent.width
                                spacing: 10

                                Text {
                                    Layout.preferredWidth: 88
                                    text: row.modelData.label
                                    color: "#7c8190"
                                    font.pixelSize: 11
                                    elide: Text.ElideRight
                                }
                                Text {
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 0
                                    text: row.modelData.value
                                    color: "#e6e8ee"
                                    font.pixelSize: 11
                                    font.family: row.modelData.mono ? infoPage.monoFont : Fonts.text
                                    elide: Text.ElideMiddle
                                }
                            }
                        }
                    }
                }
            }
        }

        TextEdit { id: clipboard; visible: false }
    }
}
