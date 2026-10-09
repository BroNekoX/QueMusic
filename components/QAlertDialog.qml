// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QueMusic 1.0
import QtQuick.Controls.Basic
    // 毛玻璃对话框主体
Popup {
    id: dialog
    property Item blurSource: Options.mainLayout // 使用父内容作为模糊源
    property rect rectXy: Qt.rect(dialog.x, dialog.y, dialog.width, dialog.height)
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
    width: 420
    height: contentCol.implicitHeight + 40
    onClosed: { input.text = ""; input.focus = false }
    Connections {
        target: Options
        enabled: dialog.visible
        function onExit(): void {
            dialog.close();
        }
    }

    background: QBlurCard {
        anchors.fill: parent
        blurSource: dialog.blurSource
        rectXy: dialog.rectXy
        borderRadius: Style.cubeRadius
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
            color: Theme.fontColor
            wrapMode: Text.WordWrap
        }

        Text {
            id: messageText
            text: "Messages"
            font.pixelSize: 13
            color: Theme.fontColor
            wrapMode: Text.WordWrap
            width: parent.width
        }

        Rectangle {
            visible: dialog.isInput
            width: parent.width
            implicitHeight: 36
            radius: 12
            color: Theme.primaryColor
            border.width: 2
            border.color: input.focus ? Theme.themeColor : Theme.secondaryColor
            TextInput {
                id: input
                anchors.fill: parent
                anchors.margins: 4
                color: Theme.textColor
                font.pixelSize: Style.textmain
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
                radius: Style.labelRadius
                buttonColor: Theme.secondaryColor
                borderColor: Theme.sideColor
                borderWidth: 1
                onClicked: { dialog.cancel(); dialog.close() }
            }
            QButton {
                width: 108
                height: 36
                text: dialog.confirmText
                buttonColor: Theme.themeColor
                textColor: Theme.primaryColor
                iconColor: Theme.primaryColor
                shadowColor: Theme.themeShadowColor
                radius: Style.labelRadius
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
