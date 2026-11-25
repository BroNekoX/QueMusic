// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
pragma ComponentBehavior: Bound
// 自绘菜单：基于 Templates 的 Popup，条目用 Column+Repeater 自己排。
import QtQuick
import QtQuick.Effects
import QueMusic 1.0
import QtQuick.Templates

Popup {
    id: dialog
    property int current: -1
    property list<string> model: []
    property int index: -1                                  // 触发它的行号（调用方设置）
    readonly property int displayIndex: visible ? index : -1
    signal clicked(int index)

    // 弹到鼠标位置；popup(x, y) 则用给定坐标，越界时贴边。
    function popup(x, y: int): void {
        let px = x
        let py = y
        if (arguments.length < 2) {
            const p = Style.cursorPos(Overlay.overlay)
            px = p.x
            py = p.y
        }
        if (Overlay.overlay) {
            px = Math.max(0, Math.min(px, Overlay.overlay.width - dialog.width))
            py = Math.max(0, Math.min(py, Overlay.overlay.height - dialog.height))
        }
        dialog.x = px
        dialog.y = py
        dialog.open()
    }

    padding: 3

    parent: Overlay.overlay

    implicitWidth: 160
    implicitHeight: menuColumn.implicitHeight + 6

    contentItem: Column {
        id: menuColumn
        Repeater {
            model: dialog.model
            delegate: Rectangle {
                id: menuItem
                implicitWidth: dialog.width - 6
                implicitHeight: 36
                radius: Style.labelRadius - 2
                color: menuItem.index === dialog.current ? Theme.containColor : hoverHandler.hovered ? Theme.hoverColor : "transparent"
                Behavior on color { ColorAnimation { duration: 80 } }
                required property string modelData
                required property int index
                HoverHandler {
                    id: hoverHandler
                }
                TapHandler {
                    gesturePolicy: TapHandler.WithinBounds
                    onTapped: {
                        dialog.clicked(menuItem.index);
                        dialog.close();
                    }
                }
                Text {
                    text: menuItem.modelData
                    color: menuItem.index === dialog.current ? Theme.themeColor : Theme.fontColor
                    font.pixelSize: Style.textmain
                    anchors.verticalCenter: parent.verticalCenter
                    leftPadding: 12
                    elide: Text.ElideRight//保证超长歌手名不会撑破菜单项
                }
            }
        }
    }

    background: Rectangle {
        color: Theme.primaryColor
        radius: Style.labelRadius
        RectangularShadow {
            anchors.fill: parent
            z: -1
            offset.x: 3
            offset.y: 5
            radius: parent.radius
            blur: 20
            spread: 0
            color: Theme.shadowColor
        }
    }

    enter: Transition {
        NumberAnimation { property: "opacity"; duration: 160; from: 0; to: 1 }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; duration: 120; to: 0 }
    }
}