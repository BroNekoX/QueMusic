// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QueMusic 1.0
import QtQuick.Controls.Basic
import QtQuick.Effects
import 'qrc:/QueMusic/components'
    // 毛玻璃对话框主体
Popup {
    id: dialog
    property alias title: title.text
    property int type: 0  // 0.warn 1.success 2.error
    parent: Overlay.overlay
    x: parent.width / 2 - width / 2
    horizontalPadding: 12
    verticalPadding: 10
    z: 3
    height: 36
    width: contentRow.width + 24
    Behavior on width { enabled: dialog.visible; NumberAnimation { duration: 160; easing.type: Easing.OutExpo } }
    function tiped(title: string, type: int): void {
        delay.running = false;
        dialog.title = title;
        dialog.type = type;
        dialog.open();
        delay.running = true;
    }

    Timer {
        id: delay
        interval: 1500//dialog.type === 2 ? 3500 : 1500
        running: false; repeat: false
        onTriggered: dialog.close()
    }

    background: Rectangle {
        anchors.fill: parent
        radius: Style.settings.labelRadius
        color: Style.themes.fullColor
        border.width: 1
        border.color: Style.themes.sideColor

        RectangularShadow {
            anchors.fill: parent
            z: -1
            offset.x: 0
            offset.y: 6
            radius: 18
            blur: 20
            spread: 0
            color: Style.themes.shadowColor
        }
    }

    contentItem: Item {
        anchors.fill: parent
        Row {
            id: contentRow
            anchors.centerIn: parent
            spacing: 16
        
            Text {
                id: icon
                width: 20
                height: 20
                text: "\uf11a"
                color: dialog.type === 1 ? "#34c759" : dialog.type === 2 ? "#ff453a" : "#ff9f0a"
                font.pixelSize: Style.settings.texticon
                font.family: iconFont.name
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }
            Text {
                id: title
                height: 20
                color: Style.themes.fontColor
                font.pixelSize: 13
                font.weight: Font.DemiBold
                verticalAlignment: Text.AlignVCenter
            }
        }
    }
    enter: Transition {
        NumberAnimation { property: "y"; duration: 280; from: 30; to: 70; easing.type: Easing.OutExpo }
        NumberAnimation { property: "opacity"; duration: 240; from: 0; to: 1; easing.type: Easing.OutCubic }
        NumberAnimation { property: "scale"; duration: 280; from: 0.7; to: 1; easing.type: Easing.OutExpo }
    }
    exit: Transition {
        NumberAnimation { property: "y"; duration: 160; to: 30 }
        NumberAnimation { property: "opacity"; duration: 140; to: 0 }
        NumberAnimation { property: "scale"; duration: 160; to: 0.7 }
    }
}
