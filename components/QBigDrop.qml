// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
pragma ComponentBehavior: Bound
import QtQuick
import QueMusic 1.0

Rectangle {
    id: root
    width: 360
    height: 120
    color: "transparent"
    radius: Style.labelRadius
    property list<string> model: ["Click1","Click2","Click3"]
    property list<url> picModel: ["","",""]
    property int singleWidth: model.length > 0 ? width / model.length - 16 : 0
    property int choice: 0
    signal transformed(int choiced)
    Row {
        x: 8
        y: 0
        width: root.width - 16
        height: root.height
        spacing: 16
        Repeater {
            model: root.model
            delegate: Rectangle {
                id: dropDel
                required property string modelData
                required property int index
                height: parent.height
                width: root.singleWidth
                color: root.choice == index ? Theme.themeColor : Theme.primaryColor
                border.width: 2
                border.color: Theme.secondaryColor
                radius: Style.labelRadius
                Behavior on color { ColorAnimation { duration: 80 } }
            
                Rectangle {
                    id: hover
                    color: Theme.hoverColor
                    anchors.fill: parent
                    radius: root.radius
                    opacity: 0
                    visible: opacity > 0
                    Behavior on opacity { NumberAnimation { duration: 80 } }
                }

                Image {
                    x: 16
                    y: 16
                    source: root.picModel[dropDel.index]
                    height: 64
                    width: root.singleWidth - 32
                    sourceSize: Qt.size(64,64)
                    asynchronous: true
                    fillMode: Image.PreserveAspectFit
                }
            
                Text {
                    width: parent.width
                    height: 40
                    y: 80
                    text: dropDel.modelData
                    font.pixelSize: Style.textmain
                    color: root.choice == dropDel.index ? Theme.primaryColor : Theme.textColor
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                }
                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: hover.opacity = 1
                    onExited: hover.opacity = 0
                    onClicked: {
                        root.transformed(dropDel.index)
                    }
                }
            }
        }
    }
}
