// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
// 分区网格：标题 + 封面卡（歌单 / 榜单 / 歌手）喵~
import QtQuick
import QtQuick.Effects
import 'qrc:/QueMusic/components'

Column {
    id: root
    width: 200
    spacing: 14

    property string title: ""
    property var source: null
    property int cardWidth: 148
    property int cardHeight: 204
    property bool round: false
    property var badgeOf: null          // function(model) -> string
    signal picked(int index)

    visible: source ? source.count > 0 : false

    Row {
        width: root.width
        height: 26
        spacing: 8
        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            width: 3
            height: 14
            radius: 1.5
            color: center.c1
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: root.title
            font.pixelSize: 16
            font.weight: Font.DemiBold
            font.letterSpacing: 0.3
            color: "#f2f5fa"
            elide: Text.ElideRight
        }
    }

    Flow {
        width: root.width
        spacing: 18
        Repeater {
            model: root.source
            delegate: Item {
                width: root.cardWidth
                height: root.cardHeight
                scale: cardArea.containsMouse ? 1.04 : 1
                y: cardArea.containsMouse ? -3 : 0
                Behavior on scale { NumberAnimation { duration: 200; easing.type: Easing.OutExpo } }
                Behavior on y { NumberAnimation { duration: 200; easing.type: Easing.OutExpo } }

                // 悬浮投影
                RectangularShadow {
                    anchors.fill: coverWrap
                    anchors.margins: -4
                    z: -1
                    radius: coverWrap.radius
                    blur: 26
                    offset.y: 12
                    opacity: cardArea.containsMouse ? 0.55 : 0
                    color: "#b0000000"
                    Behavior on opacity { NumberAnimation { duration: 200 } }
                }

                Rectangle {
                    id: coverWrap
                    width: parent.width
                    height: width
                    radius: root.round ? width / 2 : 16
                    color: "#14ffffff"
                    border.width: cardArea.containsMouse ? 1 : 0
                    border.color: "#3dffffff"
                    Behavior on border.width { NumberAnimation { duration: 120 } }
                    QPicture {
                        anchors.fill: parent
                        anchors.margins: 1
                        radius: parent.radius - 1
                        source: root.coverOf(model.cover)
                        sourceSize: Qt.size(256, 256)
                    }
                }
                Rectangle {
                    visible: root.badgeOf !== null
                    x: 10
                    y: 10
                    width: badgeLabel.implicitWidth + 18
                    height: 22
                    radius: 11
                    color: "#b30e1219"
                    border.width: 1
                    border.color: "#2affffff"
                    Text {
                        id: badgeLabel
                        anchors.centerIn: parent
                        text: root.badgeOf ? root.badgeOf(model) : ""
                        font.pixelSize: 11
                        font.weight: Font.DemiBold
                        color: "#eef2f8"
                    }
                }
                Text {
                    y: parent.width + 12
                    width: parent.width
                    height: 20
                    text: model.title || model.name || ""
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    color: "#eef2f8"
                    elide: Text.ElideRight
                    horizontalAlignment: root.round ? Text.AlignHCenter : Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                }
                Text {
                    y: parent.width + 34
                    width: parent.width
                    height: 18
                    text: model.artist || model.singer || ""
                    visible: text !== ""
                    font.pixelSize: 11
                    color: "#8d99aa"
                    elide: Text.ElideRight
                    horizontalAlignment: root.round ? Text.AlignHCenter : Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                }
                MouseArea {
                    id: cardArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.picked(index)
                }
            }
        }
    }

    function coverOf(c: var): string {
        const s = c ? String(c).replace("{size}", "256") : ""
        return s !== "" ? s : "qrc:/QueMusic/resources/app/musicpic.png"
    }
}
