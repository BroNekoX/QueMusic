// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Effects
import 'qrc:/QueMusic/components'
    // 毛玻璃对话框主体
Popup {
    id: dialog
    property Item blurSource: mainLayout // 使用父内容作为模糊源
    property var rectXy: Qt.rect(dialog.x, dialog.y, dialog.width, dialog.height)
    property alias title: titleText.text
    property alias message: messageText.text
    property alias input: input.text
    property string cancelText: "取消"
    property string confirmText: "确定"
    property bool isInput: false
    property bool dismissOnOverlay: true
    signal confirm()
    signal cancel()
    parent: Overlay.overlay
    anchors.centerIn: parent
    modal: true
    focus: true
    //visible: card.opacity !== 0
    width: 420
    height: contentCol.implicitHeight + 40
    onClosed: { input.text = ""; input.focus = false }

    background: QBlurCard {
        anchors.fill: parent
        blurSource: dialog.blurSource
        rectXy: dialog.rectXy
        //cardColor: Style.themes.primaryBlurColor
        borderRadius: Style.settings.cubeRadius
    }

    contentItem: Column {
        id: contentCol
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        Text {
            id: titleText
            text: "Title"
            font.pixelSize: 20
            font.bold: true
            color: Style.themes.fontColor
            wrapMode: Text.WordWrap
        }

        Text {
            id: messageText
            text: "Messages"
            font.pixelSize: 13
            color: Style.themes.fontColor
            wrapMode: Text.WordWrap
            width: parent.width
        }

        Rectangle {
            visible: dialog.isInput
            width: parent.width
            implicitHeight: 36
            radius: 12
            color: Style.themes.primaryColor
            border.width: 2
            border.color: input.focus ? Style.themes.themeColor : Style.themes.secondaryColor
            TextInput {
                id: input
                anchors.fill: parent
                anchors.margins: 4
                color: Style.themes.textColor
                font.pixelSize: Style.settings.textmain
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                onAccepted: {
                    dialog.confirm()
                    dialog.close()
                }
            }
        }

        Row {
            anchors.right: parent.right
            spacing: 10

            QButton {
                width: 108
                height: 36
                text: dialog.cancelText
                iconCharacter: "\uf025" // X 图标
                radius: Style.settings.labelRadius
                buttonColor: Style.themes.secondaryColor
                borderColor: Style.themes.sideColor
                borderWidth: 1
                onClicked: { dialog.cancel(); dialog.close() }
            }
            QButton {
                width: 108
                height: 36
                text: dialog.confirmText
                buttonColor: Style.themes.themeColor
                textColor: Style.themes.primaryColor
                iconColor: Style.themes.primaryColor
                shadowColor: Style.themes.themeShadowColor
                radius: Style.settings.labelRadius
                iconCharacter: "\uf0e7" // 继续图标
                onClicked: { dialog.confirm(); dialog.close() }
            }
        }
    }
    enter: Transition {
        NumberAnimation { property: "scale"; duration: 240; from: 1.1; to: 1.0; easing.type: Easing.OutCubic }
        NumberAnimation { property: "opacity"; duration: 160; from: 0; to: 1 }
    }
    exit: Transition {
        NumberAnimation { property: "scale"; duration: 160; to: 1.1 }
        NumberAnimation { property: "opacity"; duration: 120; to: 0 }
    }
}