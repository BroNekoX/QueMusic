// QPicture.qml
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QueMusic 1.0
import QtQuick.Controls.Basic
import QtQuick.Effects

Item {
    id: root
    width: 80
    height: 80

    property url source//: "qrc:/QueMusic/resources/app/musicpic.png"
    property int radius: width / 2
    property bool cache: Options.settings.picCache ? true : false
    property alias radius1: mask.topLeftRadius
    property alias radius2: mask.topRightRadius
    property alias radius3: mask.bottomLeftRadius
    property alias radius4: mask.bottomRightRadius
    property alias picScale: mask.scale
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
        onStatusChanged: if(status === Image.Error) source = "qrc:/QueMusic/resources/app/musicpic.png";
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
    Rectangle {
        id: mask
        width: root.width
        height: root.height
        radius: root.radius
        color: "#000000"
        layer.enabled: true
        visible: false
    }
}
