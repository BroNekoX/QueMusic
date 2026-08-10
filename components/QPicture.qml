// QPicture.qml
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects

Item {
    id: root
    width: 80
    height: 80

    property url source//: "qrc:/QueMusic/resources/app/musicpic.png"
    property int radius: width / 2
    property bool cache: false
    property alias radius1: maskRectangle.topLeftRadius
    property alias radius2: maskRectangle.topRightRadius
    property alias radius3: maskRectangle.bottomLeftRadius
    property alias radius4: maskRectangle.bottomRightRadius
    property alias picScale: maskRectangle.scale
    property size sourceSize: Qt.size(width,height)

    // 原始图像，隐藏
    Image {
        id: sourceItem
        source: root.source
        anchors.fill: parent
        sourceSize: root.sourceSize
        cache: root.cache
        fillMode: Image.PreserveAspectCrop
        visible: false
    }

    //
    MultiEffect {
        id: multiEffect
        source: sourceItem
        anchors.fill: sourceItem
        maskEnabled: true
        maskSource: mask
        // 属性抗锯齿
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
            id: maskRectangle
            anchors.fill: parent
            radius: root.radius
            color: "black" // 黑色用于掩码：纯黑表示完全不透明
        }
    }
}
