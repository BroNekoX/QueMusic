// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QueMusic 1.0
import QtQuick.Effects


MultiEffect {
    id: root
    width: 80
    height: 80

    property url image
    property int radius: width / 2
    property bool cache: Config.picCache
    property alias radius1: mask.topLeftRadius
    property alias radius2: mask.topRightRadius
    property alias radius3: mask.bottomLeftRadius
    property alias radius4: mask.bottomRightRadius
    property alias picScale: mask.scale
    property size sourceSize: Qt.size(width,height)
    source: image.status === Image.Ready ? image : Options.musicpic

    Image {
        id: image
        source: root.image
        width: root.width
        height: root.height
        sourceSize: root.sourceSize
        cache: root.cache
        asynchronous: true
        fillMode: Image.PreserveAspectCrop
        visible: false
    }
    maskEnabled: true
    maskSource: mask
    // 边缘抗锯齿
    maskThresholdMin: 0.5
    maskSpreadAtMin: 1.0
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
