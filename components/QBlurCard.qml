// QBlurCard.qml
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// 卡片实时背景模糊：抓取指定区域，单 frag 完成模糊 + 饱和度 + 圆角。
import QtQuick
import QtQuick.Window
import QueMusic 1.0
import QtQuick.Effects

Item {
    id: root
    clip: false

    property Item blurSource                            // 模糊源
    property real blur: 1.0                             // 模糊强度 0~1
    property real blurMax: Style.settings.blurSize / 2  // 模糊半径上限（设备像素）
    property real saturation: 1.4                     // 饱和度倍数
    property bool highQuality: Style.settings.highQualityBlur
    property int  borderRadius: Style.settings.cubeRadius
    property color cardColor: Style.themes.primaryBlurColor
    property color borderColor: Style.themes.primaryBlurColor
    property real borderWidth: 1
    property color fillColor: Style.themes.secondaryColor  // 源透明/空缺处的补底色
    property bool shadowEffect: false
    property bool masked: false                         // 外部接口保留

    default property alias content: topCard.data

    implicitWidth: 300
    implicitHeight: 200

    property rect rectXy: Qt.rect(root.x, root.y, root.width, root.height)

    readonly property int _texW: Math.max(2, Math.round(root.width  * Screen.devicePixelRatio))
    readonly property int _texH: Math.max(2, Math.round(root.height * Screen.devicePixelRatio))

    ShaderEffectSource {
        id: effectSource
        anchors.fill: parent
        sourceItem: root.blurSource
        sourceRect: root.rectXy
        textureSize: Qt.size(root._texW, root._texH)
        mipmap: true
        live: root.visible && root.blurSource !== null
        visible: false
    }

    RectangularShadow {
        anchors.fill: root
        z: 0
        offset.x: 0
        offset.y: 8
        radius: root.borderRadius
        blur: 28
        spread: 0
        visible: root.shadowEffect
        color: Style.themes.shadowColor
    }

    ShaderEffect {
        anchors.fill: parent
        z: 2
        visible: root.blurSource !== null

        property var src: effectSource
        property real blur: root.blur
        property real blurMax: root.blurMax
        property real saturation: root.saturation
        property real corner: root.borderRadius
        property vector2d cardSize: Qt.vector2d(root.width, root.height)
        property vector2d texSize: Qt.vector2d(root._texW, root._texH)
        property real aa: 1.0
        property color fillColor: root.fillColor

        fragmentShader: root.highQuality ? "qrc:/shaders/shaders/cardblur_hq.frag.qsb" : "qrc:/shaders/shaders/cardblur.frag.qsb"
    }

    // 叠加主题色, 避免过亮/过透明
    Rectangle {
        id: topCard
        anchors.fill: parent
        radius: root.borderRadius
        color: root.cardColor
        z: 3
        border.color: root.borderColor
        border.width: root.borderWidth
    }
}
