// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
import QtQuick
import QueMusic 1.0

Rectangle {
    id: root
    width: 300
    height: 88
    radius: 16
    color: cardArea.containsMouse ? Theme.containColor : Theme.primaryColor
    border.color: Theme.secondaryColor
    border.width: 2
    property url source: "qrc:/QueMusic/resources/app/header.png"
    property string title: "Account"
    property string text: ""
    property string openUrl: ""
    Behavior on color { ColorAnimation { duration: 120 } }
    QPicture {
        x: 19
        y: 19
        source: root.source
        width: 50
        height: 50
        radius: 25
    }

    Column {
        spacing: 6
        x: 80
        anchors.verticalCenter: root.verticalCenter
        Text {
            text: root.title
            color: Theme.fontColor
            verticalAlignment: Text.AlignVCenter
            font.bold: true
            font.pixelSize: Style.textH2
        }
        Text {
            text: root.text
            width: root.width - 96
            color: Theme.textColor
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: Style.text
            wrapMode: Text.Wrap
        }
    }
    MouseArea {
        id: cardArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: {
            if(root.openUrl != "") {
                Qt.openUrlExternally(root.openUrl);
            }
        }
    }
}