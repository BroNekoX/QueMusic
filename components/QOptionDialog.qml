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
    default property alias options: dialogContent.contentData
    property string cancelText: "默认"
    property string cancelIcon: "\uf10f"
    property string confirmText: "完成"
    property bool isInput: false
    property bool dismissOnOverlay: true
    property int dialogContentHeight: 320
    signal confirm()
    signal cancel()
    parent: Overlay.overlay
    anchors.centerIn: parent
    modal: true
    focus: true
    //visible: card.opacity !== 0
    width: 480
    height: contentCol.implicitHeight + 40
    //onClosed: { input.text = ""; input.focus = false }

    background: QBlurCard {
        anchors.fill: parent
        blurSource: dialog.blurSource
        rectXy: dialog.rectXy
        shadowEffect: true
        cardColor: Style.themes.blurSecondaryColor
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

        ScrollView {
            id: dialogContent
            width: contentCol.width + 10
            height: dialog.dialogContentHeight > window.height - 320 ? window.height - 320 : dialog.dialogContentHeight
            contentWidth: contentCol.width
            contentHeight: dialog.dialogContentHeight
            clip: true
        }

        Row {
            anchors.right: contentCol.right
            spacing: 10

            QButton {
                implicitWidth: 108
                implicitHeight: 36
                text: dialog.cancelText
                iconCharacter: dialog.cancelIcon // X 图标
                radius: Style.settings.labelRadius
                iconSize: Style.settings.texticon - 2
                onClicked: { dialog.cancel(); dialog.close() }
            }
            QButton {
                implicitWidth: 108
                implicitHeight: 36
                text: dialog.confirmText
                buttonColor: Style.themes.themeColor
                textColor: Style.themes.primaryColor
                iconColor: Style.themes.primaryColor
                shadowColor: Style.themes.themeShadowColor
                iconCharacter: "\uf0e7" // 勾图标
                radius: Style.settings.labelRadius
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