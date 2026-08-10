// QBigDrop.qml
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root
    width: 360
    height: 120
    color: "transparent"
    radius: Style.settings.labelRadius
    property list<string> model: ["Click1","Click2","Click3"]
    property list<string> picModel: ["","",""]
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

                Image {
                    x: 16
                    y: 16
                    source: root.picModel[index]
                    height: 64
                    width: root.singleWidth - 32
                    sourceSize: Qt.size(64,64)
                    fillMode: Image.PreserveAspectFit
                }
            
                Text {
                    width: parent.width
                    height: 40
                    y: 80
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