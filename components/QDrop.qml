// QDrop.qml
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024-2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import QtQuick.Effects

Rectangle {
    id: root
    width: 160
    height: 36
    radius: Style.settings.labelRadius
    property color buttonColor: Style.themes.primaryColor
    color: mouseArea.containsMouse ? Qt.darker(Style.themes.primaryColor, 1.2) : Style.themes.primaryColor
    border.width: 2
    border.color: Style.themes.secondaryColor
    property string text: model[choice]
    property bool useId: false
    property string icon: "\uf096"
    property var model: ["Click1","Click2"]
    property int choice: 0
    property string iconFontFamily: iconFont.name    // 图标字体
    signal transformed(int choiced)
    clip: false
    Behavior on color { ColorAnimation { duration: 80 } }
    Text {
        x: 0
        y: 0
        width: root.height
        height: root.height
        color: Style.themes.textColor
        text: root.icon
        font.pixelSize: Style.settings.texticon
        font.family: root.iconFontFamily
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
    }
    
    Text {
        x: root.height
        width: root.width - root.height
        height: root.height
        clip: true
        text: root.useId ? root.model[choice].description : root.text
        color: Style.themes.fontColor
        font.pixelSize: Style.settings.textmain
        font.bold: true
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
    }
    
    MouseArea {
        id: mouseArea
        z: 2
        anchors.fill: root
        hoverEnabled: true
        onClicked: {
            if(popmenu.visible) {
                popmenu.close()
            } else {
                popmenu.open()
            }
        }
    }
   
    // 弹出菜单
    Popup {
        id: popmenu
        x: 0
        y: root.height + 6
        z: 10
        width: root.width
        height: root.model.length * 36 + 4
        padding: 0
        margins: 0
        //radius: 12
        enter: Transition {
            NumberAnimation { property: "opacity"; duration: 240; from: 0.0; to: 1.0; easing.type: Easing.OutExpo }
            NumberAnimation { property: "scale"; duration: 240; from: 0.3; to: 1.0; easing.type: Easing.OutExpo }
        }
        exit: Transition {
            NumberAnimation { property: "opacity"; duration: 120; to: 0.0 }
            NumberAnimation { property: "scale"; duration: 120; to: 0.5 }
        }
        transformOrigin: Popup.TopLeft
        //color: Style.themes.primaryColor

        background: Item {
            Rectangle {
                z: 1
                id: menuCard
                anchors.fill: parent
                color: Style.themes.primaryColor
                radius: root.radius
            }
            RectangularShadow {
                anchors.fill: parent
                z: 0
                offset.x: 5
                offset.y: 5
                radius: root.radius
                blur: 24
                spread: 0
                color: Style.themes.shadowColor
            }
        }

        contentItem: Column {
            id: dropList 
            anchors.fill: parent
            anchors.margins: 2
            Repeater {
                model: root.model
                delegate: Rectangle {
                    Behavior on color { ColorAnimation { duration: 80 } }
                    id: dropDele
                    width: dropList.width
                    height: 36
                    color: root.choice == index ? Style.themes.themeColor : "transparent"
                    radius: root.radius
                    Text {
                        anchors.fill: parent
                        text: root.useId ? root.model[index].description : modelData
                        color: root.choice == index ? Style.themes.primaryColor : Style.themes.textColor
                        font.pixelSize: Style.settings.textmain
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                    }
                
                    Rectangle {
                        id: hover
                        color: Style.themes.hoverColor
                        anchors.fill: parent
                        radius: root.radius
                        opacity: 0
                        Behavior on opacity { NumberAnimation { duration: 80 } }
                    }
            
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onEntered: hover.opacity = 1
                        onExited: hover.opacity = 0
                        onClicked: {
                            //root.choice = index
                            root.transformed(index)
                            popmenu.close()
                        }
                    }
                }
            }
        }
    }
}