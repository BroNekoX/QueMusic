// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
import QtQuick

Rectangle {
    id: root
    width: settingStack.standWidth / 2 - 12
    height: 88
    radius: 16
    color: cardArea.containsMouse ? Style.themes.containColor : Style.themes.fullColor
    border.color: Style.themes.secondaryColor
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
            color: Style.themes.fontColor
            verticalAlignment: Text.AlignVCenter
            font.bold: true
            font.pixelSize: Style.settings.textH2
        }
        Text {
            text: root.text
            width: root.width - 96
            color: Style.themes.textColor
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: Style.settings.text
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