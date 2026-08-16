// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Effects
import 'qrc:/QueMusic/components'
    // 毛玻璃对话框主体
Popup {
    id: dialog
    property Item blurSource: mainLayout // 使用父内容作为模糊源
    property var rectXy: Qt.rect(dialog.x, dialog.y, dialog.width, dialog.height)
    property alias title: title.text
    property int type: 0  // 0.warn 1.success 2.error
    parent: Overlay.overlay
    x: 226
    y: 64
    horizontalPadding: 12
    verticalPadding: 10
    z: 3
    height: 100
    width: 320
    //onClosed: { input.text = ""; input.focus = false }
    Behavior on width { NumberAnimation { duration: 160; easing.type: Easing.OutExpo } }

    background: Rectangle {
        anchors.fill: parent
        radius: Style.settings.cubeRadius
        color: Style.themes.fullColor
        opacity: 1

        RectangularShadow {
            anchors.fill: parent
            z: -1
            offset.x: 3
            offset.y: 3
            radius: Style.settings.labelRadius
            blur: 16
            spread: 0
            color: Qt.hsva(backRec.color.hsvHue,1.0,0.3,0.2)//Style.themes.shadowColor
        }
    }

    contentItem: Item {
        anchors.fill: parent
        //  搜索弹出的内容，等待更新
    }
    enter: Transition {
        NumberAnimation { property: "y"; duration: 240; from: 50; to: 64; easing.type: Easing.OutExpo }
        NumberAnimation { property: "opacity"; duration: 160; from: 0; to: 1 }
    }
    exit: Transition {
        NumberAnimation { property: "y"; duration: 180; to: 50 }
        NumberAnimation { property: "opacity"; duration: 120; to: 0 }
    }
}