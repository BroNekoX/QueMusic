// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024-2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Controls.Basic

Rectangle {
    id: root
    implicitWidth: 200
    implicitHeight: 36
    radius: Style.settings.labelRadius
    color: Style.themes.primaryColor
    border.width: 2
    border.color: input.focus ? Style.themes.themeColor : Style.themes.sideColor
    signal entered()
    property alias inputText: input.text
    TextInput {
        id: input
        anchors.fill: parent
        anchors.margins: 4
        color: Style.themes.textColor
        font.pixelSize: Style.settings.textmain
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        //focus: true
        //onTextEdited: parent.border.color = Style.themes.themeColor
        //onEditingFinished: parent.border.color = "transparent"
        onAccepted: {
            root.entered()
            input.focus = false
        }
    }
}