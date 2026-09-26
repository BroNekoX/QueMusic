// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
// 窗口控制：最小化 / 全屏切换（Esc 或系统关闭键退出沉浸中心）喵~
import QtQuick
import QueMusic 1.0
import 'qrc:/QueMusic/components'

Row {
    id: root
    spacing: 6
    signal minimize()
    signal toggleFull()
    signal close()

    SButton {
        iconCharacter: "\uf055"
        width: 38
        height: 38
        radius: 19
        buttonColor: "#14ffffff"
        hoverColor: "#24ffffff"
        iconColor: "#dfe6f0"
        iconSize: 18
        shadowEnabled: false
        onClicked: root.minimize()
        tipText: "最小化"
    }
    SButton {
        iconCharacter: "\uf07a"
        width: 38
        height: 38
        radius: 19
        buttonColor: "transparent"
        hoverColor: "#1affffff"
        iconColor: "#dfe6f0"
        iconSize: 18
        shadowEnabled: false
        onClicked: root.toggleFull()
        tipText: "全屏"
    }
}
