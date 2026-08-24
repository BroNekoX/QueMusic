// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects
import 'qrc:/QueMusic/components'
    // 毛玻璃对话框主体
Popup {
    id: dialog
    property Item blurSource: mainLayout // 使用父内容作为模糊源
    property var rectXy: Qt.rect(dialog.x, dialog.y, dialog.width, dialog.height)
    parent: Overlay.overlay
    x: 226
    y: 64
    //horizontalPadding: 12
    //verticalPadding: 10
    z: 3
    height: 200
    width: 320
    signal searchIndex(int index)
    //onClosed: { input.text = ""; input.focus = false }
    Behavior on width { NumberAnimation { duration: 160; easing.type: Easing.OutExpo } }

    background: Rectangle {
        anchors.fill: parent
        radius: Style.settings.cubeRadius
        color: Style.themes.fullColor
        opacity: 1

        RectangularShadow {
            anchors.fill: parent
            z: -1
            offset.x: 3
            offset.y: 3
            radius: Style.settings.labelRadius
            blur: 16
            spread: 0
            color: Style.themes.shadowColor
        }
    }

    contentItem: Item {
        anchors.fill: parent
        clip: true
        //  搜索弹出的内容，等待更新
        Text {
            text: "搜索历史记录"
            font.pixelSize: Style.themes.textTip
            color: Style.themes.textColor
            x: 16
            y: 12
        }

        // 清空搜索记录（右上角）
        Text {
            id: clearButton
            anchors.right: parent.right
            anchors.rightMargin: 16
            anchors.top: parent.top
            anchors.topMargin: 10
            text: "清空"
            visible: Options.settings.searchList.length > 0
            font.pixelSize: Style.themes.textTip
            color: clearArea.containsMouse ? Style.themes.fontColor : Style.themes.textColor
            MouseArea {
                id: clearArea
                anchors.fill: parent
                hoverEnabled: true
                onClicked: Options.settings.searchList = []
            }
        }

        Flow {
            spacing: 6
            y: 36
            x: 16
            width: parent.width - 32
            clip: true
            Repeater {
                model: Options.settings.searchList
                delegate: Rectangle {
                    //短搜索记录自适应即可，超长时以父容器宽度为上限
                    width: Math.min(searchText.implicitWidth + 16, parent.width)
                    height: searchText.height + 12
                    color: Style.themes.secondaryColor
                    radius: Style.settings.labelRadius
                    Rectangle {
                        radius: parent.radius
                        anchors.fill: parent
                        color: Style.themes.hoverColor
                        opacity: searchArea.containsMouse ? 1 : 0
                        Behavior on opacity { NumberAnimation { duration: 120 } }
                    }

                    Text {
                        id: searchText
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: 8
                        anchors.right: parent.right
                        anchors.rightMargin: 8
                        //超出部分省略号截断，强制单行
                        elide: Text.ElideRight
                        maximumLineCount: 1
                        text: modelData
                        color: Style.themes.fontColor
                        font.pixelSize: Style.settings.text
                    }
                    MouseArea {
                        id: searchArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: dialog.searchIndex(index);
                    }
                }
            }
        }
    }
    enter: Transition {
        NumberAnimation { property: "y"; duration: 240; from: 50; to: 64; easing.type: Easing.OutExpo }
        NumberAnimation { property: "opacity"; duration: 160; from: 0; to: 1; easing.type: Easing.OutCubic }
    }
    exit: Transition {
        NumberAnimation { property: "y"; duration: 180; to: 50 }
        NumberAnimation { property: "opacity"; duration: 120; to: 0 }
    }
}