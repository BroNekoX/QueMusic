// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
// 胶囊标签栏：顶部主导航与页内小标签共用
import QtQuick

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
        color: "#aafafafa"
        border.color: "#aaffffff"
        border.width: 2
        radius: 21
    }

    Rectangle {
        x: root.currentIndex * root.tabWidth + 3
        z: 1
        y: 3
        width: root.tabWidth
        height: parent.height - 6
        radius: height / 2
        color: "#ffffff"
        visible: root.showPill
        Behavior on x { NumberAnimation { duration: 240; easing.type: Easing.OutExpo } }
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
                    radius: 21
                    opacity: area.containsMouse ? 1 : 0
                    color: "#31111111"
                    Behavior on opacity { NumberAnimation { duration: 120 } }
                }
                Text {
                    anchors.fill: parent
                    text: modelData
                    font.pixelSize: 13
                    font.weight: active ? Font.DemiBold : Font.Normal
                    color: active ? "#14161c" : (area.containsMouse ? "#ffffff" : "#555555")
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
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
