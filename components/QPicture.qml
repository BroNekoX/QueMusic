// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QueMusic 1.0
import QtQuick.Effects

Item {
    id: root
    width: 80
    height: 80

    property url source
    property int radius: width / 2
    property bool cache: Config.picCache ? true : false
    property alias radius1: mask.topLeftRadius
    property alias radius2: mask.topRightRadius
    property alias radius3: mask.bottomLeftRadius
    property alias radius4: mask.bottomRightRadius
    property alias picScale: mask.scale
    property size sourceSize: Qt.size(width,height)

    readonly property url fallbackSource: "qrc:/QueMusic/resources/app/musicpic.png"
    property bool loadFailed: false
    onSourceChanged: loadFailed = false

    Image {
        id: sourceItem
        source: root.loadFailed ? root.fallbackSource : root.source
        anchors.fill: parent
        sourceSize: root.sourceSize
        cache: root.cache
        asynchronous: true
        fillMode: Image.PreserveAspectCrop
        visible: false
        onStatusChanged: {
            if(status === Image.Error && source !== root.fallbackSource)
                root.loadFailed = true;
        }
    }

    MultiEffect {
        id: multiEffect
        source: sourceItem
        anchors.fill: sourceItem
        maskEnabled: true
        maskSource: mask
        // 边缘抗锯齿
        maskThresholdMin: 0.5
        maskSpreadAtMin: 1.0
    }

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
