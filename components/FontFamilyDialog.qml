// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// 字体家族选择对话框（继承 QOptionDialog）：可搜索的家族列表，每行用它自己的字体渲染。
// 用法：currentFamily 指定初始选中项，openFamily() 打开，onAccepted 里读 selectedFamily。
// 注意：QOptionDialog 的 blurSource 默认指向 main.qml 的 mainLayout，跨文件使用需调用方显式设置。
import QtQuick
import QtQuick.Controls.Basic
import QueMusic 1.0

QOptionDialog {
    id: root

    title: "选择字体"
    cancelText: "取消"
    confirmText: "确定"

    property string currentFamily: ""
    property string selectedFamily: ""

    signal accepted()

    // 空字符串表示跟随系统默认字体，放列表首位
    readonly property var families: [""].concat(Qt.fontFamilies())
    readonly property var filteredFamilies: {
        const needle = search.inputText.trim().toLowerCase();
        return needle === "" ? families
                             : families.filter(name => name.toLowerCase().includes(needle));
    }

    function openFamily(family: string): void {
        search.inputText = "";
        selectedFamily = family === "" ? families[0] : family;
        familyList.currentIndex = Math.max(0, filteredFamilies.indexOf(selectedFamily));
        familyList.positionViewAtIndex(familyList.currentIndex, ListView.Center);
        open();
    }

    onConfirm: root.accepted()

    options: Column {
        width: parent.width
        spacing: 10

        QInput {
            id: search
            width: parent.width
        }

        ListView {
            id: familyList
            width: parent.width
            height: 300
            clip: true
            model: root.filteredFamilies
            property int scrollToY: familyList.contentY
            //boundsBehavior: Flickable.StopAtBounds
            onDraggingChanged: familyList.scrollToY = familyList.contentY

            ScrollBar.vertical: ScrollBar {
                id: viewBar
                parent: familyList
                anchors.top: familyList.top
                anchors.right: familyList.right
                anchors.bottom: familyList.bottom
                onPressedChanged: {
                    familyList.scrollToY = familyList.contentY
                }
            }
            WheelHandler {
                acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                readonly property real wheelHeightCount: Qt.application.styleHints.wheelScrollLines * 0.25
                onWheel: (event) => {
                    listViewAnime.running = false;
                    familyList.scrollToY = Math.max(0, Math.min( familyList.scrollToY - (event.angleDelta.y * wheelHeightCount), familyList.contentHeight - familyList.height));
                    viewBar.active = true;
                    event.accepted = true;
                    listViewAnime.running = true;
                }
            }
            NumberAnimation {
                id: listViewAnime
                target: familyList
                property: "contentY"
                duration: 240
                to: familyList.scrollToY
                easing.type: Easing.OutCubic
                onFinished: viewBar.active = false
            }

            delegate: Rectangle {
                required property int index
                required property string modelData

                width: familyList.width
                height: 40
                radius: Style.settings.labelRadius
                color: root.selectedFamily === modelData
                       ? Style.themes.containColor
                       : (familyHover.containsMouse ? Style.themes.hoverColor : "transparent")

                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 12
                    anchors.right: parent.right
                    anchors.rightMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    text: modelData === "" ? "系统默认" : modelData
                    font.family: modelData
                    font.pixelSize: Style.settings.textmain
                    color: root.selectedFamily === modelData
                           ? Style.themes.themeColor
                           : Style.themes.fontColor
                    elide: Text.ElideRight
                }

                MouseArea {
                    id: familyHover
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: root.selectedFamily = modelData
                }
            }
        }
    }
}
