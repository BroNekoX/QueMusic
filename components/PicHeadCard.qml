// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Effects

Item {
    id: root
    implicitHeight: 200
    implicitWidth: 150
    property int radius: Style.settings.cubeRadius
    signal clicked()
    property real pressScale: 0.94
    property url source//: "qrc:/QueMusic/resources/app/musicpic.png"
    property int textSize: 18
    property string title: ""
    Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutExpo } }
    Rectangle {
        id: hover
        z: 2
        anchors.fill: parent
        radius: root.radius
        opacity: 0
        visible: opacity > 0
        color: Style.themes.hoverColor
        Behavior on opacity { NumberAnimation { duration: 120 } }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.clicked()
        onPressed: root.scale = root.pressScale
        onReleased: root.scale = 1.0
        onCanceled: root.scale = 1.0
        onEntered: hover.opacity = 1
        onExited: hover.opacity = 0
    }
    Item {
        width: root.width
        height: root.width

        // 原始图像，隐藏
        Image {
            id: sourceItem
            source: root.source
            anchors.fill: parent
            sourceSize.width: parent.width
            sourceSize.height: parent.height
            fillMode: Image.PreserveAspectCrop
            visible: false
        }

        //
        MultiEffect {
            id: multiEffect
            source: sourceItem
            anchors.fill: parent
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
        id: text
        width: root.width - 24
        height: root.height - root.width
        x: 12
        y: root.width
        text: root.title
        font.pixelSize: root.textSize
        font.bold: true
        color: Style.themes.fontColor
        verticalAlignment: Text.AlignVCenter
    }
}