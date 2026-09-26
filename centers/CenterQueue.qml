// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
// 播放队列抽屉（实时模糊玻璃）喵~
import QtQuick
import QueMusic 1.0
import QtQuick.Controls.Basic
import 'qrc:/QueMusic/components'

QBlurCard {
    id: root
    width: 360
    blurMax: 48
    blur: 1.0
    saturation: 1.25
    borderRadius: 26
    cardColor: "#5c111420"
    borderColor: "#1fffffff"
    borderWidth: 1

    property Item blurSource
    rectXy: blurSource ? root.mapToItem(blurSource, 0, 0, root.width, root.height)
                       : Qt.rect(root.x, root.y, root.width, root.height)

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
        Row {
            width: parent.width
            height: 22
            spacing: 8
            Text {
                id: queueTitle
                anchors.verticalCenter: parent.verticalCenter
                text: "播放队列"
                font.pixelSize: 15
                font.weight: Font.DemiBold
                color: "#f2f5fa"
            }
            Text {
                anchors.baseline: queueTitle.baseline
                text: "· " + (root.queue ? root.queue.count : 0) + " 首"
                font.pixelSize: 12
                color: "#8d99aa"
            }
        }
        // 预留空态提示位，列表吃掉剩余高度，避免底部溢出被裁
        Item {
            width: parent.width
            height: parent.height - 34
            ListView {
                id: queueList
                anchors.fill: parent
                model: root.queue
                clip: true
                spacing: 4
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar {
                    parent: queueList
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                }
            delegate: Item {
                id: row
                required property int index
                required property string name
                required property string songer
                readonly property bool isCurrent: index === root.currentIndex

                width: ListView.view ? ListView.view.width : 0
                height: 52

                Rectangle {
                    anchors.fill: parent
                    radius: 14
                    visible: row.isCurrent
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0; color: Qt.rgba(center.c1.r, center.c1.g, center.c1.b, 0.30) }
                        GradientStop { position: 1; color: Qt.rgba(center.c2.r, center.c2.g, center.c2.b, 0.14) }
                    }
                }
                Rectangle {
                    anchors.fill: parent
                    radius: 14
                    visible: !row.isCurrent
                    color: rowArea.containsMouse ? "#12ffffff" : "#08ffffff"
                    Behavior on color { ColorAnimation { duration: 140 } }
                }
                Rectangle {
                    x: 0
                    width: 3
                    height: 20
                    radius: 1.5
                    anchors { left: parent.left; verticalCenter: parent.verticalCenter }
                    color: center.c1
                    visible: row.isCurrent
                }

                MouseArea {
                    id: rowArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.picked(index)
                }
                Text {
                    x: 14
                    width: 28
                    anchors.verticalCenter: parent.verticalCenter
                    text: row.index + 1
                    font.pixelSize: 12
                    font.weight: row.isCurrent ? Font.DemiBold : Font.Normal
                    color: row.isCurrent ? center.c1 : "#7c8798"
                    horizontalAlignment: Text.AlignHCenter
                }
                Column {
                    anchors { left: parent.left; leftMargin: 50; right: parent.right; rightMargin: 12; verticalCenter: parent.verticalCenter }
                    spacing: 3
                    Text {
                        width: parent.width
                        text: row.name
                        font.pixelSize: 13
                        font.weight: row.isCurrent ? Font.DemiBold : Font.Normal
                        color: row.isCurrent ? "#ffffff" : "#dfe5ee"
                        elide: Text.ElideRight
                    }
                    Text {
                        width: parent.width
                        text: row.songer
                        font.pixelSize: 11
                        color: "#8d99aa"
                        elide: Text.ElideRight
                    }
                }
            }
            }

            Column {
                anchors.centerIn: parent
                spacing: 6
                visible: !root.queue || root.queue.count === 0
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "队列空空如也"
                    font.pixelSize: 14
                    font.weight: Font.DemiBold
                    color: "#93a0b2"
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "添加喜欢的歌曲开始聆听吧"
                    font.pixelSize: 12
                    color: "#6b7686"
                }
            }
        }
    }
}
