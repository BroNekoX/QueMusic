// QDrop.qml
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
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
    color: Style.themes.primaryColor
    border.width: 2
    border.color: Style.themes.borderColor
    property string text: model[choice]
    property bool useId: false
    property string icon: "\uf096"
    property var model: ["Click1","Click2"]
    property int choice: 0
    property bool enabled: true
    property color textColor: Style.themes.textColor
    property string iconFontFamily: iconFont.name    // 图标字体
    property int cardRadius: radius
    signal transformed(int choiced)
    clip: false
    Rectangle {
        anchors.fill: parent
        color: Style.themes.hoverColor
        radius: root.radius
        opacity: mouseArea.containsMouse ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 80 } }
    }
    Text {
        x: 0
        y: 0
        width: root.height
        height: root.height
        color: root.textColor
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
        color: root.textColor
        font.pixelSize: Style.settings.textmain
        font.bold: true
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
    }
    
    MouseArea {
        id: mouseArea
        z: 2
        anchors.fill: root
        enabled: root.enabled
        hoverEnabled: true
        onClicked: {
            if(root)
            if(popmenu.visible) {
                popmenu.close();
            } else {
                popmenu.open();
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
            NumberAnimation { property: "scale"; duration: 240; from: 0.5; to: 1.0; easing.type: Easing.OutExpo }
        }
        exit: Transition {
            NumberAnimation { property: "opacity"; duration: 120; to: 0.0 }
            NumberAnimation { property: "scale"; duration: 120; to: 0.7 }
        }
        transformOrigin: Popup.Top
        //color: Style.themes.primaryColor

        background: Rectangle {
            id: menuCard
            color: Style.themes.primaryColor
            radius: root.cardRadius
            RectangularShadow {
                anchors.fill: parent
                z: -1
                offset.x: 5
                offset.y: 5
                radius: root.cardRadius
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
                    radius: root.cardRadius
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
                        radius: root.cardRadius
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
                            root.transformed(index);
                            popmenu.close();
                        }
                    }
                }
            }
        }
    }
}