//ECard.qml
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
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
    property int padding: 12 // 内容区域的内边距

    // 阴影属性
    property bool shadowEnabled: true
    property color shadowColor: Style.themes.shadowColor

    // === 插槽：用户内容插入点 ===
    default property alias content: contentLayout.data

    // 卡片尺寸根据内容自适应
    implicitWidth: contentLayout.implicitWidth + padding * 2
    implicitHeight: contentLayout.implicitHeight + padding * 2

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
        anchors.margins: root.padding
    }
}
