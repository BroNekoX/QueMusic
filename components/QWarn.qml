// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024-2026 QueMusic Contributors
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
    x: parent.width / 2 - width / 2
    horizontalPadding: 12
    verticalPadding: 10
    z: 3
    height: 36
    width: contentRow.width + 24
    //onClosed: { input.text = ""; input.focus = false }
    Behavior on width { NumberAnimation { duration: 120; easing.type: Easing.OutExpo } }
    function tiped(title,type) {
        dialog.title = title
        dialog.type = type
        dialog.open()
        delay.running = true
    }

    Timer {
        id: delay
        interval: 1500; running: false; repeat: false
        onTriggered: dialog.close()
    }

    background: Rectangle {
        anchors.fill: parent
        radius: Style.settings.labelRadius
        color: Style.themes.fullColor
        opacity: 1
        Rectangle {
            id: backRec
            anchors.fill: parent
            radius: Style.settings.labelRadius
            color: dialog.type === 0 ? "#ffffaa" : dialog.type === 1 ? "#bbffbb" : "#ffbbbb"
            opacity: 0.3
        }

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
        Row {
            id: contentRow
            anchors.centerIn: parent
            spacing: 16
            Behavior on width { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }
        
        
            Text {
                id: icon
                width: 20
                height: 20
                text: "\uf11a"
                color: Style.themes.textColor
                font.pixelSize: Style.settings.texticon
                font.family: iconFont.name
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }
            Text {
                id: title
                height: 20
                //text: dialog.text
                color: Style.themes.textColor
                font.pixelSize: 13
                font.bold: false
                verticalAlignment: Text.AlignVCenter
            }
        }
    }
    enter: Transition {
        NumberAnimation { property: "y"; duration: 240; from: -60; to: 70; easing.type: Easing.OutExpo }
        NumberAnimation { property: "opacity"; duration: 160; from: 0; to: 1 }
    }
    exit: Transition {
        NumberAnimation { property: "y"; duration: 160; to: -60 }
        NumberAnimation { property: "opacity"; duration: 120; to: 0 }
    }
}