// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QueMusic 1.0
import QtQuick.Controls.Basic
import 'qrc:/QueMusic/components'
Popup {
    id: dialog
    property Item blurSource: mainLayout // 使用父内容作为模糊源
    property rect rectXy: Qt.rect(dialog.x, dialog.y, dialog.width, dialog.height)
    property alias title: titleText.text
    default property alias options: content.children
    //property Component options
    property string cancelText: ""
    property string cancelIcon: "\uf10f"
    property string confirmText: "完成"
    property bool isInput: false
    property bool dismissOnOverlay: true
    // false 时 cancel 只发信号、不关闭：调用方把左下按钮当确认按钮用时
    // （例如网易云手机号登录，失败提示要留在弹窗里）
    property bool cancelCloses: true
    signal confirm()
    signal cancel()
    parent: Overlay.overlay
    anchors.centerIn: parent
    modal: true
    focus: true
    width: 480
    height: contentCol.implicitHeight + 40
    Connections {
        target: window
        enabled: dialog.visible
        function onExit(): void {
            dialog.close();
        }
    }

    background: QBlurCard {
        anchors.fill: parent
        blurSource: dialog.blurSource
        rectXy: dialog.rectXy
        shadowEffect: true
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
            height: 20
            font.bold: true
            color: Style.themes.fontColor
            wrapMode: Text.WordWrap
        }

        Flickable {
            id: dialogContent
            width: contentCol.width + 10
            height: contentHeight > window.height - 320 ? window.height - 320 : contentHeight
            contentHeight: contentItem.childrenRect.height
            contentWidth: width - 10
            boundsBehavior: Flickable.StopAtBounds
            clip: true
            synchronousDrag: true
            ScrollBar.vertical: ScrollBar {
                anchors.right: dialogContent.right
                //anchors.rightMargin: 10
                anchors.top: dialogContent.top
                anchors.bottom: dialogContent.bottom
            }
            Item {
                id: content
                width: dialogContent.contentWidth
                height: children[0].height
            }
        }

        Row {
            anchors.right: contentCol.right
            spacing: 10

            QButton {
                width: 120
                height: 36
                text: dialog.cancelText
                visible: dialog.cancelText !== ""
                iconCharacter: dialog.cancelIcon // X 图标
                radius: Style.settings.labelRadius
                buttonColor: Style.themes.secondaryColor
                borderColor: Style.themes.sideColor
                borderWidth: 1
                iconSize: Style.settings.texticon - 2
                onClicked: { dialog.cancel(); if (dialog.cancelCloses) dialog.close() }
            }
            QButton {
                width: 108
                height: 36
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
