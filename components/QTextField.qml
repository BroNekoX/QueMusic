// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// 自绘输入框：Templates 的 TextField 只有编辑内核，占位文字和背景在这里补，
// 配色走 Theme。调用点原有的 background / placeholderText / padding 仍然有效。
import QtQuick
import QtQuick.Templates
import QueMusic 1.0

TextField {
    id: root

    implicitWidth: Math.max(contentWidth, placeholder.implicitWidth) + leftPadding + rightPadding
    implicitHeight: Math.max(contentHeight, placeholder.implicitHeight) + topPadding + bottomPadding

    padding: 6
    leftPadding: padding + 4

    color: Theme.fontColor
    selectionColor: Theme.themeColor
    selectedTextColor: Theme.primaryColor
    placeholderTextColor: Theme.textColor
    verticalAlignment: TextInput.AlignVCenter
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
                 && (!root.activeFocus || root.horizontalAlignment !== Qt.AlignHCenter)
        elide: Text.ElideRight
    }

    background: Rectangle {
        implicitWidth: 200
        implicitHeight: 36
        radius: Style.labelRadius
        color: Theme.primaryColor
        border.width: 1
        border.color: root.activeFocus ? Theme.themeColor : Theme.sideColor
    }
}