// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// 自绘多行输入框：同 QTextField，补上占位文字与背景。
import QtQuick
import QtQuick.Templates
import QueMusic 1.0

TextArea {
    id: root

    implicitWidth: Math.max(contentWidth + leftPadding + rightPadding,
                            implicitBackgroundWidth + leftInset + rightInset,
                            placeholder.implicitWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(contentHeight + topPadding + bottomPadding,
                             implicitBackgroundHeight + topInset + bottomInset,
                             placeholder.implicitHeight + topPadding + bottomPadding)

    padding: 6
    leftPadding: padding + 4

    color: Theme.fontColor
    selectionColor: Theme.themeColor
    selectedTextColor: Theme.primaryColor
    placeholderTextColor: Theme.textColor
    selectByMouse: true

    Text {
        id: placeholder
        x: root.leftPadding
        y: root.topPadding
        width: root.width - (root.leftPadding + root.rightPadding)
        height: root.height - (root.topPadding + root.bottomPadding)

        text: root.placeholderText
        font: root.font
        color: root.placeholderTextColor
        verticalAlignment: root.verticalAlignment
        visible: !root.length && !root.preeditText
        elide: Text.ElideRight
    }

    background: Rectangle {
        color: "transparent"
        border.width: root.activeFocus ? 2 : 1
        border.color: root.activeFocus ? Theme.themeColor : Theme.sideColor
    }
}