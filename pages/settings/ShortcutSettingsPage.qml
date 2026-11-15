// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//

pragma ComponentBehavior: Bound
import QtQuick

import QueMusic 1.0

QScrollView {
    id: page
    property int standWidth: 0
    property int containWidth: 0
    property int containX: 0
    property color cardColor: Theme.primaryColor
    property string recordingAction: ""
    property bool isRecording: false
    property bool oldShortCutState: false

    y: 60
    width: parent ? parent.width : 0
    height: parent ? parent.height - 60 : 0
    visible: false

    // 根据 action 名拼出 Config 里的"该功能是否全局生效"属性名
    function globalPropertyName(actionName: string): string {
        return "globalShortcut" + actionName.charAt(0).toUpperCase() + actionName.slice(1)
    }

    // 按键转字符串（辅助函数）
    function keyToString(key: int): string {
        if (key >= Qt.Key_F1 && key <= Qt.Key_F35)
            return "F" + (key - Qt.Key_F1 + 1)
        switch (key) {
            case Qt.Key_Escape: return "Esc"
            case Qt.Key_Return: return "Enter"
            case Qt.Key_Backspace: return "Backspace"
            case Qt.Key_Tab: return "Tab"
            case Qt.Key_Space: return "Space"
            case Qt.Key_Left: return "Left"
            case Qt.Key_Right: return "Right"
            case Qt.Key_Up: return "Up"
            case Qt.Key_Down: return "Down"
            case Qt.Key_Insert: return "Insert"
            case Qt.Key_Delete: return "Delete"
            case Qt.Key_Home: return "Home"
            case Qt.Key_End: return "End"
            case Qt.Key_PageUp: return "PageUp"
            case Qt.Key_PageDown: return "PageDown"
            default:
                if (key >= Qt.Key_A && key <= Qt.Key_Z)
                    return String.fromCharCode(key)
                else if (key >= Qt.Key_0 && key <= Qt.Key_9)
                    return String.fromCharCode(key)
                else
                    return "" // 不支持的键
        }
    }

    function keyEventToSequence(event: var): string {
        const modifiers = []
        if (event.modifiers & Qt.ControlModifier) modifiers.push("Ctrl")
        if (event.modifiers & Qt.AltModifier) modifiers.push("Alt")
        if (event.modifiers & Qt.ShiftModifier) modifiers.push("Shift")
        const key = event.key
        // 忽略单独的修饰键
        if (key === Qt.Key_Control || key === Qt.Key_Alt || key === Qt.Key_Shift || key === Qt.Key_Meta)
            return ""
        const keyName = keyToString(key)
        if (!keyName) return ""
        let seq = modifiers.join("+")
        if (seq && keyName) seq += "+"
        seq += keyName
        return seq
    }

    // 开始录制
    function startRecording(action: string): void {
        if (isRecording) return
        recordingAction = action
        isRecording = true
        oldShortCutState = Config.openShortCut
        Config.openShortCut = false   // 关闭总开关（兼容旧逻辑）
        Options.recordingShortCut = true   // 录制期间屏蔽所有全局快捷键
        keyCapture.forceActiveFocus()
        keyCapture.focus = true;
        Options.warned("按下新的快捷键... (按 Esc 取消)", 0)
    }

    // 停止录制（完成或取消）
    function stopRecording(success: bool, sequence: string): void {
        isRecording = false
        Config.openShortCut = oldShortCutState
        Options.recordingShortCut = false
        if (success && sequence) {
            ShortCut[recordingAction] = sequence
            Options.warned("已设置快捷键: " + sequence, 1)
        } else {
            Options.warned("已取消录制", 1)
        }
        recordingAction = ""
        keyCapture.focus = false
    }

    // 动作定义（名称、显示描述、默认键位）
    ListModel {
        id: actionDefs
        ListElement { name: "play"; desc: "播放/暂停"; defau: "Space" }
        ListElement { name: "back"; desc: "上一首"; defau: "Left" }
        ListElement { name: "forward"; desc: "下一首"; defau: "Right" }
        ListElement { name: "playList"; desc: "打开/关闭播放列表"; defau: "Alt" }
        ListElement { name: "musicControl"; desc: "音乐控制面板"; defau: "Up" }
        ListElement { name: "volumeUp"; desc: "音量增大"; defau: "Ctrl+Up" }
        ListElement { name: "volumeDown"; desc: "音量减小"; defau: "Ctrl+Down" }
        ListElement { name: "seekBack"; desc: "快退"; defau: "Ctrl+Left" }
        ListElement { name: "seekForward"; desc: "快进"; defau: "Ctrl+Right" }
        ListElement { name: "mute"; desc: "静音切换"; defau: "Ctrl+M" }
        ListElement { name: "abLoop"; desc: "A-B 片段循环"; defau: "Ctrl+B" }
        ListElement { name: "favorite"; desc: "收藏当前曲目"; defau: "Ctrl+D" }
        ListElement { name: "playerOptions"; desc: "播放器选项"; defau: "Ctrl+T" }
    }

    // 内容
    Column {
        id: shortcutContent
        spacing: 16
        padding: 24
        width: page.containWidth
        x: page.containX

        Text {
            height: 40
            color: Theme.fontColor
            verticalAlignment: Text.AlignVCenter
            text: "快捷键"
            font.pixelSize: Style.pageTitle
            font.weight: Font.DemiBold
        }

        // 全局开关
        QHead { text: "全局快捷键" }

        Rectangle {
            width: page.standWidth
            color: page.cardColor
            radius: Style.cubeRadius
            Column {
                width: parent.width
                padding: 0
                Component.onCompleted: parent.height = height

                SettingItemCard {
                    label: "启用全局快捷键"
                    tip: "开启后可在其他窗口用快捷键控制播放"
                    QSwitch {
                        letRight: true
                        switchTrue: Config.openShortCut
                        onToggled: Config.openShortCut = !Config.openShortCut
                    }
                    bottomLine: false
                }
            }
        }

        // 按键捕获器（隐藏）
        Rectangle {
            id: keyCapture
            height: page.isRecording ? 60 : 0
            width: page.standWidth
            color: Theme.fontColor
            border.width: 1
            border.color: Theme.sideColor
            radius: 16
            focus: false
            visible: true          // 必须可见才能获得焦点
            opacity: page.isRecording ? 1 : 0 // 透明，不干扰界面
            enabled: page.isRecording   // 仅在录制时启用
            Behavior on opacity { NumberAnimation { duration: 120 } }
            Behavior on height { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }
            Keys.onPressed: (event) => {
                if (!page.isRecording) return;
                // 按 Esc 取消
                if (event.key === Qt.Key_Escape) {
                    page.stopRecording(false)
                    event.accepted = true
                    Options.warned("已取消", 0)
                    return
                }
                const seq = page.keyEventToSequence(event)
                if (seq) {
                    page.stopRecording(true, seq)
                    Options.warned("设置已保存", 1)
                    event.accepted = true
                }
                // 如果是无效键（如单独的修饰键），不处理，等待有效组合
            }
            Row {
                anchors.centerIn: parent
                spacing: 8
                QButton {
                    width: 100
                    height: 34
                    text: "取消[Esc]"
                    buttonColor: Theme.primaryColor
                    borderWidth: 1
                    onClicked: {
                        page.stopRecording(false);
                        Options.warned("已取消", 0);
                    }
                }
                Text {
                    id: keyCaptureText
                    anchors.verticalCenter: parent.verticalCenter
                    font.pixelSize: Style.textmain
                    color: Theme.secondaryColor
                    text: "正在键位录制状态，请输入一个键来设置" + page.recordingAction + "功能的快捷键"
                }
            }
        }

        // 快捷键列表
        QHead { text: "快捷键列表" }
        Rectangle {
            width: page.standWidth
            color: page.cardColor
            radius: Style.cubeRadius
            height: shortCutColumn.height // 自适应高度

            Column {
                id: shortCutColumn
                width: parent.width
                padding: 0
                property bool displayDefau: width > 720
                Repeater {
                    model: actionDefs
                    delegate: Rectangle {
                        id: shortCutDel
                        width: shortCutColumn.width
                        height: 56
                        color: "transparent"
                        radius: Style.labelRadius
                        required property var model
                        Row {
                            spacing: 12
                            anchors.verticalCenter: parent.verticalCenter
                            x: 16

                            Text {
                                text: shortCutDel.model.desc
                                width: 150
                                font.pixelSize: Style.textmain
                                color: Theme.fontColor
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }

                            Text {
                                text: ShortCut[shortCutDel.model.name] || shortCutDel.model.defau
                                width: 120
                                font.pixelSize: Style.textmain
                                color: Theme.fontColor
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }

                            Text {
                                text: "默认: " + shortCutDel.model.defau
                                width: 120
                                visible: shortCutColumn.displayDefau
                                font.pixelSize: Style.textmain
                                color: Qt.rgba(Theme.fontColor.r, Theme.fontColor.g, Theme.fontColor.b, 0.6)
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }
                        }
                        QSwitch {
                            x: parent.width - 336
                            y: 10
                            width: 96
                            height: 36
                            switchTrue: ShortCut[page.globalPropertyName(shortCutDel.model.name)]
                            onToggled: {
                                const prop = page.globalPropertyName(shortCutDel.model.name);
                                ShortCut[prop] = !ShortCut[prop];
                            }
                        }

                        QButton {
                            x: parent.width - 224
                            y: 10
                            width: 96
                            height: 36
                            text: "设置"
                            shadowEnabled: false
                            radius: Style.labelRadius
                            borderWidth: 2
                            buttonColor: "transparent"
                            onClicked: page.startRecording(shortCutDel.model.name)
                        }

                        QButton {
                            x: parent.width - 112
                            y: 10
                            width: 96
                            height: 36
                            text: "重置"
                            shadowEnabled: false
                            radius: Style.labelRadius
                            borderWidth: 2
                            buttonColor: "transparent"
                            onClicked: {
                                ShortCut[shortCutDel.model.name] = shortCutDel.model.defau
                                Options.warned("已恢复默认快捷键", 1)
                            }
                        }
                    }
                }
            }
        }

        // 底部提示
        Text {
            width: page.standWidth
            text: "提示：点击「设置」后按下新的组合键（如 Ctrl+Shift+A），按 Esc 取消。"
            color: Theme.textColor
            font.pixelSize: 12
            wrapMode: Text.Wrap
        }
    }
}
