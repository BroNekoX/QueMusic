// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
// 分区网格：标题 + 封面卡（歌单 / 榜单 / 歌手）
import QtQuick
import 'qrc:/QueMusic/components'

Column {
    id: root
    width: 200
    spacing: 12

    property string title: ""
    property var source: null
    property int cardWidth: 148
    property int cardHeight: 204
    property bool round: false
    property var badgeOf: null          // function(model) -> string
    signal picked(int index)

    visible: source ? source.count > 0 : false

    Text {
        width: root.width
        height: 26
        text: root.title
        font.pixelSize: 15
        font.weight: Font.DemiBold
        color: "#f5f7fb"
        elide: Text.ElideRight
        verticalAlignment: Text.AlignVCenter
    }

    Flow {
        width: root.width
        spacing: 16
        Repeater {
            model: root.source
            delegate: Item {
                width: root.cardWidth
                height: root.cardHeight
                scale: cardArea.containsMouse ? 1.04 : 1
                Behavior on scale { NumberAnimation { duration: 180; easing.type: Easing.OutExpo } }

                QPicture {
                    width: parent.width
                    height: width
                    radius: root.round ? width / 2 : 14
                    source: root.coverOf(model.cover)
                    sourceSize: Qt.size(256, 256)
                }
                Rectangle {
                    visible: root.badgeOf !== null
                    x: 8
                    y: 8
                    width: badgeLabel.implicitWidth + 16
                    height: 20
                    radius: 10
                    color: "#ffffff"
                    Text {
                        id: badgeLabel
                        anchors.centerIn: parent
                        text: root.badgeOf ? root.badgeOf(model) : ""
                        font.pixelSize: 11
                        font.weight: Font.DemiBold
                        color: "#14161c"
                    }
                }
                Text {
                    y: parent.width + 10
                    width: parent.width
                    height: 20
                    text: model.title || model.name || ""
                    font.pixelSize: 13
                    font.bold: true
                    color: "#f2f5fa"
                    elide: Text.ElideRight
                    horizontalAlignment: root.round ? Text.AlignHCenter : Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                }
                Text {
                    y: parent.width + 30
                    width: parent.width
                    height: 18
                    text: model.artist || model.singer || ""
                    visible: text !== ""
                    font.pixelSize: 11
                    color: "#93a8b8"
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

    function coverOf(c) {
        var s = c ? String(c).replace("{size}", "256") : ""
        return s !== "" ? s : "qrc:/QueMusic/resources/app/musicpic.png"
    }
}
