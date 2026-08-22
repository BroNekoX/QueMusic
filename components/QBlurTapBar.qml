// QBlurTapBar.qml
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects

Item {
    id: root
    clip: false

    // --- 公共属性 ---
    property Item blurSource
    property real blurAmount: 1
    property bool dragable: false
    property bool blurMask: true
    property real cardOpacity: 1.0
    property rect rectXy: Qt.rect(root.x, root.y, root.width, root.height)
    property real blurMax: Style.settings.blurSize
    property real borderRadius: Style.settings.noControlRadius ? Style.settings.labelRadius : height / 2
    property color borderColor: "transparent"
    property real borderWidth: 0
    property int tabWidth: 120
    property var model: []
    signal tabChange(int index)

    width: 244
    height: 40

    // --- 捕获背景内容 ---
    ShaderEffectSource {
        id: effectSource
        anchors.fill: parent
        sourceItem: root.blurSource
        sourceRect: root.rectXy
        visible: false
    }

    // === 创建遮罩 ===
    Rectangle {
        id: maskItem
        z: 1
        anchors.fill: root
        layer.enabled: true
        layer.smooth: true
        radius: root.borderRadius
        color: Style.themes.primaryColor
        visible: true
    }

    RectangularShadow {
        anchors.fill: root
        z: 0
        offset.x: 5
        offset.y: 5
        radius: root.borderRadius
        blur: 20
        spread: 0
        color: Style.themes.shadowColor
    }

    // === 启用遮罩 ===
    MultiEffect {
        z: 2
        anchors.fill: root
        source: effectSource
        autoPaddingEnabled: false
        blurEnabled: true
        blurMax: root.blurMax
        blur: root.blurAmount
        blurMultiplier: 0.5
        saturation: 0.7
        maskEnabled: root.blurMask
        maskSource: maskItem
    }

    // ====叠加主题色, 避免过亮/过透明 ====
    Rectangle {
        id: topCard
        anchors.fill: root
        radius: root.borderRadius
        color: Style.themes.sideBlurColor
        z: 3
        opacity: root.cardOpacity
        border.color: root.borderColor
        border.width: root.borderWidth
    }

    // 内容容器
    Item {
        id: contentItem
        clip: false
        anchors.fill: parent
        z: 4
        Rectangle {
            id: topAnine
            y: 2
            x: 2 + tabView.choiceIndex * root.tabWidth
            z: 1
            width: root.tabWidth
            height: root.height - 4
            radius: root.borderRadius
            color: Style.themes.fullColor
            Behavior on x { NumberAnimation { duration: 300; easing.type: Easing.Bezier; easing.bezierCurve: [ 0.23, 0.06, 0.00, 0.98, 1, 1 ] } }
        }
        Row {
            id: tabView
            anchors.fill: parent
            anchors.margins: 2
            property int choiceIndex: 0
            z: 2
            Repeater {
                model: root.model

                delegate: Item {
                    id: navMusic
                    width: root.tabWidth
                    height: root.height - 4
                    property bool isSelected: tabView.choiceIndex === index


                    Rectangle {
                        z: 0
                        anchors.fill: navMusic
                        radius: root.borderRadius
                        opacity: indexArea.containsMouse && !navMusic.isSelected ? 1 : 0
                        color: Style.themes.hoverColor
                        Behavior on opacity { NumberAnimation { duration: 100 } }
                    }


                    Text {
                        anchors.fill: parent
                        text: modelData
                        color: navMusic.isSelected ? Style.themes.fontColor : Style.themes.textColor
                        font.pixelSize: Style.settings.text
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        Behavior on color { ColorAnimation { duration: 150 } }
                    }


                    MouseArea {
                        id: indexArea
                        anchors.fill: navMusic
                        hoverEnabled: true
                        onClicked: {
                            if(tabView.choiceIndex !== index) {
                                root.tabChange(index);
                            }
                            tabView.choiceIndex = index;
                            forceActiveFocus();
                        }
                    }
                }
            }
        }
    }
}
