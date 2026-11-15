// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors

import QtQuick
import QueMusic 1.0
import QtQuick.Templates

ToolTip {
    id: root
    property int radius: 8

    // 定位/尺寸/关闭策略原来由 Basic 的 ToolTip 提供，Templates 版必须自己给。
    x: parent ? (parent.width - implicitWidth) / 2 : 0
    y: -implicitHeight - 3
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    margins: 6
    padding: 6
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
                 | Popup.CloseOnReleaseOutsideParent

    horizontalPadding: 10
    verticalPadding: 6
    delay: 480
    opacity: visible ? 1 : 0
    Behavior on opacity { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
    contentItem: Text {
        text: root.text
        font.pixelSize: Style.text
        wrapMode: Text.Wrap
        color: "#f5f5f7"
    }
    background: Rectangle {
        anchors.fill: parent
        radius: root.radius
        color: "#e61d1d20"
        border.color: "#ff333437"
        border.width: 1
    }
}