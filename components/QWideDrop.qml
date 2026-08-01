// QWideDrop.qml
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024-2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root
    width: 360
    height: 40
    color: "transparent"
    radius: Style.settings.labelRadius
    property list<string> model: ["Click1","Click2","Click3"]
    property int singleWidth: width / model.length - 16
    property int choice: 0
    signal transformed(int choiced)
    Row {
        x: 8
        y: 0
        width: root.width - 16
        height: root.height
        property int choiceIndex: 0
        spacing: 16
        Repeater {
            model: root.model
            delegate: Rectangle {
                height: parent.height
                width: root.singleWidth
                color: root.choice == index ? Style.themes.themeColor : Style.themes.primaryColor
                border.width: 2
                border.color: Style.themes.secondaryColor
                radius: Style.settings.labelRadius
                Behavior on color { ColorAnimation { duration: 80 } }
            
                Rectangle {
                    id: hover
                    color: Style.themes.hoverColor
                    anchors.fill: parent
                    radius: root.radius
                    opacity: 0
                    Behavior on opacity { NumberAnimation { duration: 80 } }
                }
            
                Text {
                    anchors.fill: parent
                    text: modelData
                    font.pixelSize: Style.settings.textmain
                    color: root.choice == index ? Style.themes.primaryColor : Style.themes.textColor
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                }
                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: hover.opacity = 1
                    onExited: hover.opacity = 0
                    onClicked: {
                        //root.choice = index
                        root.transformed(index)
                    }
                }
            }
        }
    }
}