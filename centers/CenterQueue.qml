// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
// 播放队列抽屉
import QtQuick
import QueMusic 1.0

Rectangle {
    id: root
    width: 360
    radius: 26
    color: "#b30b0c12"
    border.width: 1
    border.color: "#1fffffff"

    property bool opened: false
    property QueueModel queue: null
    property int currentIndex: -1
    property int areaTop: 0
    property int areaHeight: 400
    property int areaRight: 24
    signal picked(int index)

    x: root.opened ? parent.width - width - root.areaRight : parent.width + 12
    y: root.areaTop
    height: root.areaHeight
    visible: x < parent.width
    Behavior on x { NumberAnimation { duration: 420; easing.type: Easing.OutExpo } }

    Column {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 12
        Text {
            text: "播放队列 · " + (root.queue ? root.queue.count : 0) + " 首"
            font.pixelSize: 15
            font.weight: Font.Medium
            color: "#f2f3f7"
        }
        ListView {
            width: parent.width
            height: parent.height - 44
            model: root.queue
            clip: true
            spacing: 4
            delegate: Rectangle {
                required property int index
                required property string name
                required property string songer
                readonly property bool isCurrent: index === root.currentIndex

                width: ListView.view.width - 8
                height: 52
                radius: 12
                color: isCurrent ? "#1effffff" : (rowArea.containsMouse ? "#12ffffff" : "transparent")

                MouseArea {
                    id: rowArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.picked(index)
                }
                Text {
                    x: 12
                    width: 28
                    anchors.verticalCenter: parent.verticalCenter
                    text: index + 1
                    font.pixelSize: 12
                    color: isCurrent ? "#ffffff" : "#8fd7de"
                    horizontalAlignment: Text.AlignHCenter
                }
                Text {
                    anchors { left: parent.left; leftMargin: 48; right: parent.right; rightMargin: 12; top: parent.top; topMargin: 8 }
                    text: name
                    font.pixelSize: 13
                    font.weight: isCurrent ? Font.DemiBold : Font.Normal
                    color: isCurrent ? "#ffffff" : "#cde6ea"
                    elide: Text.ElideRight
                }
                Text {
                    anchors { left: parent.left; leftMargin: 48; right: parent.right; rightMargin: 12; bottom: parent.bottom; bottomMargin: 7 }
                    text: songer
                    font.pixelSize: 11
                    color: "#9ac9d2"
                    elide: Text.ElideRight
                }
            }
        }
    }
}
