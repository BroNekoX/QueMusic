// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
// 窗口控制：最小化 / 全屏 / 关闭（几何图形绘制，不依赖私有区字形）
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
        width: 40
        height: 40
        radius: 12
        buttonColor: "transparent"
        iconColor: "#fcfdff"
        iconSize: 20
        shadowEnabled: false
        tipText: "Account"
    }
    SButton {
        iconCharacter: "\uf07a"
        width: 40
        height: 40
        radius: 12
        buttonColor: "transparent"
        iconColor: "#fcfdff"
        iconSize: 20
        shadowEnabled: false
        onClicked: root.toggleFull();
        tipText: "全屏"
    }
}
