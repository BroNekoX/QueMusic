// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QueMusic 1.0
import QtQuick.Effects
import QtQuick.Controls.Basic
Menu {
    id: dialog
    property int current: -1
    title: "Menu"
    parent: Overlay.overlay
    property list<string> model: []
    signal clicked(int index)


    background: Rectangle {
        implicitWidth: 160
        implicitHeight: 40
        color: Style.primaryColor
        radius: Style.settings.labelRadius
        RectangularShadow {
            anchors.fill: parent
            z: -1
            offset.x: 0
            offset.y: 5
            radius: parent.radius
            blur: 20
            spread: 0
            color: Style.shadowColor
        }
    }

    Instantiator {
        model: dialog.model
        delegate: MenuItem {
            id: menuItem
            background: Rectangle {
                implicitWidth: 146
                implicitHeight: 36
                x: 2
                y: 2
                radius: Style.settings.labelRadius - 2
                width: menuItem.width - 4
                height: menuItem.height - 4
                color: index === dialog.current ? Style.containColor
                     : (menuItem.down || menuItem.highlighted) ? Style.hoverColor
                     : "transparent"
            }
            text: modelData
            //显式指定contentItem，
            contentItem: Text {
                text: menuItem.text
                color: index === dialog.current ? Style.themeColor : Style.fontColor//使用项目主题文字色，深浅色主题下都可读
                font.pixelSize: Style.settings.textmain
                verticalAlignment: Text.AlignVCenter
                leftPadding: 12
                elide: Text.ElideRight//保证超长歌手名不会撑破菜单项
            }
            onTriggered: dialog.clicked(index)
        }
        onObjectAdded: (i, obj) => dialog.insertItem(i, obj)
        onObjectRemoved: (i, obj) => dialog.removeItem(obj)
    }


    enter: Transition {
        NumberAnimation { property: "opacity"; duration: 160; from: 0; to: 1 }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; duration: 120; to: 0 }
    }
}
