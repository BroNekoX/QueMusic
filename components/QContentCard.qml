//QContentCard.qml
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024-2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Layouts
import QtQuick.Effects

Item {
    id: root

    // === 公共接口与样式 ===
    property bool backgroundVisible: true

    property color cardColor: Style.themes.primaryColor
    property real radius: Style.settings.cubeRadius
    property string picSource
    property int picX: 16
    property int picY: 16
    property int picWidth: 108
    property int picHeight: 80
    property string title: ""
    property string text: ""

    // 阴影属性
    property bool shadowEnabled: true
    property color shadowColor: Style.themes.shadowColor

    width: 140
    height: 160

    // === 阴影效果 ===
    RectangularShadow {
        anchors.fill: root
        z: 0
        offset.x: 5
        offset.y: 5
        radius: root.radius
        blur: 24
        spread: 0
        visible: root.shadowEnabled
        color: Style.themes.shadowColor
    }

    // === 卡片背景 ===
    Rectangle {
        id: background
        z: 1
        visible: root.backgroundVisible
        anchors.fill: parent
        radius: root.radius
        color: root.cardColor
    }

    // === 内容布局 ===
    Item {
        id: contentLayout
        z: 2
        anchors.fill: parent
        anchors.margins: 0//root.padding
        Item {
            id: picContent
            x: root.picX
            y: root.picY
            width: root.picWidth
            height: root.picHeight

            // 原始图像，隐藏
            Image {
                id: sourceItem
                source: root.picSource
                anchors.fill: parent
                sourceSize.width: root.picWidth
                sourceSize.height: root.picHeight
                fillMode: Image.PreserveAspectCrop
                visible: false
            }

         
            MultiEffect {
                id: multiEffect
                source: sourceItem
                anchors.fill: sourceItem
                maskEnabled: true
                maskSource: mask
                // 下面两个属性抗锯齿
                maskThresholdMin: 0.5
                maskSpreadAtMin: 1.0
            }

            // 圆形黑色矩形（用于遮罩）
            Item {
                id: mask
                width: sourceItem.width
                height: sourceItem.height
                layer.enabled: true
                visible: false
        

                Rectangle {
                    anchors.fill: parent
                    radius: root.radius
                    color: "black" // 黑色用于掩码：纯黑表示完全不透明
                }
            }
        }
        Text {
            x: 20
            y: root.height - 64
            height: 28
            verticalAlignment: Text.AlignVCenter
            text: root.title
            font.pixelSize: Style.settings.textH2
            font.bold: true
            color: Style.themes.fontColor
        }
        Text {
            x: 20
            y: root.height - 36
            height: 20
            verticalAlignment: Text.AlignVCenter
            text: root.text
            font.pixelSize: Style.settings.text
            color: Style.themes.textColor
        }
    }
}
