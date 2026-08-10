// QSwitch.qml
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Layouts
import QtQuick.Effects

Rectangle {
    id: root
    width: 160
    height: 36
    color: isBack ? Style.themes.secondaryColor : "transparent"
    radius: isBack ? 12 : 0
    signal toggled(bool switchTrue)
    
    property string text: switchTrue ? "开" : "关"
    property bool isBack: false
    property bool switchTrue: false
    property bool letRight: false
    
    Text {
        x: root.letRight ? root.width - 120 : 0
        y: 0
        width: root.width / 2
        height: root.height
        text: root.text
        color: Style.themes.textColor
        font.pixelSize: Style.settings.text
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
    }
    
    Rectangle {
        x: root.letRight ? root.width - 52 : root.width / 2
        y: 6
        width: 48
        height: 24
        radius: 12
        color: root.switchTrue ? Style.themes.themeColor : Style.themes.sideColor
        Behavior on color { ColorAnimation { duration: 280 } }
        
        Rectangle {
            x: root.switchTrue ? 27 : 5
            y: 4
            width: 16
            height: 16
            radius: 9
            color: Style.themes.primaryColor
            Behavior on x { NumberAnimation { duration: 320; easing.type: Easing.OutExpo } }
        }
    }
    
    MouseArea {
        anchors.fill: parent
        onClicked: {
            //root.switchTrue = !root.switchTrue
            root.toggled(root.switchTrue)
        }
    }
}