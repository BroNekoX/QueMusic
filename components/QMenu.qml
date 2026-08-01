// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024-2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Effects
import 'qrc:/QueMusic/components'
    // 毛玻璃对话框主体
Menu {
    id: dialog
    property Item blurSource: mainLayout // 使用父内容作为模糊源
    property var rectXy: Qt.rect(dialog.x, dialog.y, dialog.width, dialog.height)
    title: "Menu"
    //parent: Overlay.overlay
    //closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
    property list<string> model: []
    signal clicked(int index)

    //height: 40
    //onClosed: { input.text = ""; input.focus = false }

    background: QBlurCard {
        implicitWidth: 150
        implicitHeight: 40
        shadowEffect: true
        blurSource: mainLayout
        rectXy: Qt.rect(menu.x, menu.y, menu.width, menu.height)
        cardColor: Style.themes.blurSecondaryColor
        borderRadius: Style.settings.labelRadius
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
                color: menuItem.down || menuItem.highlighted ? Style.themes.hoverColor : "transparent"
            }
            text: modelData
            onTriggered: dialog.clicked(index)
        }
        onObjectAdded: (i, obj) => menu.insertItem(i, obj)
        onObjectRemoved: (i, obj) => menu.removeItem(obj)
    }


    enter: Transition {
        NumberAnimation { property: "opacity"; duration: 160; from: 0; to: 1 }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; duration: 120; to: 0 }
    }
}