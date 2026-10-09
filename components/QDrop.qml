// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
pragma ComponentBehavior: Bound
import QtQuick
import QueMusic 1.0
import QtQuick.Controls.Basic
import QtQuick.Effects

Rectangle {
    id: root
    width: 160
    height: 36
    radius: Style.labelRadius
    property color buttonColor: Theme.primaryColor
    color: Theme.primaryColor
    border.width: 2
    border.color: Theme.borderColor
    // useId 模式下文本取自 model[choice].description；对象直接赋给 string 会产生 QML 类型警告
    // 越界下标取到的是 undefined，这里改成显式范围判断，避免和 undefined 做比较
    property string text: (!useId && model && choice >= 0 && choice < model.length) ? String(model[choice]) : ""
    property bool useId: false
    property string icon: "\uf096"
    property var model: ["Click1","Click2"]
    property int choice: 0
    property bool enabled: true
    property color textColor: Theme.textColor
    property string iconFontFamily: Fonts.icon    // 图标字体
    property int cardRadius: radius
    signal transformed(int choiced)
    clip: false
    Rectangle {
        anchors.fill: parent
        color: Theme.hoverColor
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
        font.pixelSize: Style.texticon
        font.family: root.iconFontFamily
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
    }
    
    Text {
        x: root.height
        width: root.width - root.height
        height: root.height
        clip: true
        text: {
            if (!root.useId)
                return root.text
            // 设备列表可能为空，直接取 .description 会抛 TypeError
            const item = root.model ? root.model[root.choice] : null
            return (item && item.description) ? String(item.description) : ""
        }
        color: root.textColor
        font.pixelSize: Style.textmain
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
        enter: Transition {
            NumberAnimation { property: "opacity"; duration: 240; from: 0.0; to: 1.0; easing.type: Easing.OutExpo }
            NumberAnimation { property: "scale"; duration: 240; from: 0.5; to: 1.0; easing.type: Easing.OutExpo }
        }
        exit: Transition {
            NumberAnimation { property: "opacity"; duration: 120; to: 0.0 }
            NumberAnimation { property: "scale"; duration: 120; to: 0.7 }
        }
        transformOrigin: Popup.Top

        background: Rectangle {
            id: menuCard
            color: Theme.primaryColor
            radius: root.cardRadius
            RectangularShadow {
                anchors.fill: parent
                z: -1
                offset.x: 5
                offset.y: 5
                radius: root.cardRadius
                blur: 24
                spread: 0
                color: Theme.shadowColor
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
                    color: root.choice == dropDele.index ? Theme.themeColor : "transparent"
                    radius: root.cardRadius
                    required property string modelData
                    required property var model
                    required property int index
                    Text {
                        anchors.fill: parent
                        text: {
                            if (!root.useId)
                                return dropDele.modelData
                            const item = root.model ? root.model[dropDele.index] : null
                            return (item && item.description) ? String(item.description) : ""
                        }
                        color: root.choice == dropDele.index ? Theme.primaryColor : Theme.textColor
                        font.pixelSize: Style.textmain
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                    }
                
                    Rectangle {
                        id: hover
                        color: Theme.hoverColor
                        anchors.fill: parent
                        radius: root.cardRadius
                        opacity: 0
                        visible: opacity > 0
                        Behavior on opacity { NumberAnimation { duration: 80 } }
                    }
            
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onEntered: hover.opacity = 1
                        onExited: hover.opacity = 0
                        onClicked: {
                            root.transformed(dropDele.index);
                            popmenu.close();
                        }
                    }
                }
            }
        }
    }
}
