// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
// 胶囊标签栏：顶部主导航与页内小标签共用（深色玻璃 + 主题光 pill）喵~
import QtQuick
import QtQuick.Effects

Item {
    id: root
    height: 42
    width: root.model.length * root.tabWidth + 6

    property var model: []
    property int currentIndex: 0
    property int tabWidth: 90
    property bool showPill: true
    signal tabClicked(int index)

    Rectangle {
        anchors.fill: parent
        z: 0
        radius: height / 2
        color: "#14ffffff"
        border.width: 1
        border.color: "#1fffffff"
    }

    Rectangle {
        x: root.currentIndex * root.tabWidth + 3
        z: 1
        y: 3
        width: root.tabWidth
        height: parent.height - 6
        radius: height / 2
        color: "#2bffffff"
        border.width: 1
        border.color: "#24ffffff"
        visible: root.showPill
        // 选中页签的主题色微光
        RectangularShadow {
            anchors.fill: parent
            anchors.margins: -6
            radius: parent.radius
            blur: 16
            color: Qt.rgba(center.c1.r, center.c1.g, center.c1.b, 0.55)
        }
        Behavior on x { NumberAnimation { duration: 260; easing.type: Easing.OutExpo } }
    }

    Row {
        anchors.fill: parent
        anchors.margins: 3
        z: 2
        Repeater {
            model: root.model
            delegate: Item {
                width: root.tabWidth
                height: root.height - 6
                readonly property bool active: index === root.currentIndex
                Rectangle {
                    anchors.fill: parent
                    radius: height / 2
                    opacity: area.containsMouse && !active ? 1 : 0
                    color: "#14ffffff"
                    Behavior on opacity { NumberAnimation { duration: 120 } }
                }
                Text {
                    anchors.fill: parent
                    text: modelData
                    font.pixelSize: 13
                    font.weight: active ? Font.DemiBold : Font.Normal
                    color: active ? "#ffffff" : (area.containsMouse ? "#e8edf4" : "#98a4b4")
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    Behavior on color { ColorAnimation { duration: 160 } }
                }
                MouseArea {
                    id: area
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.tabClicked(index)
                }
            }
        }
    }
}
