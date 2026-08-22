// QBlurCard.qml
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
    property real blurAmount: 1.0
    property real cardOpacity: 1.0
    property rect rectXy: Qt.rect(root.x, root.y, root.width, root.height)
    property real blurMax: Style.settings.blurSize
    property real borderRadius: Style.settings.cubeRadius
    property color cardColor: Style.themes.primaryBlurColor
    property color borderColor: Style.themes.primaryBlurColor
    property bool masked: false
    property real borderWidth: 1
    property bool shadowEffect: false

    default property alias content: contentItem.data

    implicitWidth: 300
    implicitHeight: 200

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
        anchors.fill: parent
        layer.enabled: true
        layer.smooth: true
        radius: root.borderRadius
        color: Style.themes.primaryColor
        visible: root.masked
    }

    RectangularShadow {
        anchors.fill: root
        z: 0
        offset.x: 5
        offset.y: 5
        radius: root.borderRadius
        blur: 24
        spread: 0
        visible: root.shadowEffect
        color: Style.themes.shadowColor
    }

    // === 启用遮罩 ===
    MultiEffect {
        z: 2
        anchors.fill: effectSource
        source: effectSource
        autoPaddingEnabled: false
        blurEnabled: true
        blurMax: root.blurMax
        blur: root.blurAmount
        blurMultiplier: 0.5
        saturation: 0.7
        maskEnabled: true
        maskSource: maskItem
        maskThresholdMin: 0.5
        maskSpreadAtMin: 1.0
    }

    // 叠加主题色, 避免过亮/过透明
    Rectangle {
        id: topCard
        anchors.fill: parent
        radius: root.borderRadius
        color: root.cardColor
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
    }
}
