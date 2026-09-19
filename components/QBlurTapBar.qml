// QBlurTapBar.qml
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QueMusic 1.0
import QtQuick.Controls.Basic
import QtQuick.Effects

Item {
    id: root
    clip: false

    // 公共属性
    property Item blurSource
    property real blurAmount: 1
    property bool dragable: false
    property bool blurMask: true
    property rect rectXy: Qt.rect(root.x, root.y, root.width, root.height)
    property real blurMax: Style.settings.blurSize / 2
    property real borderRadius: Style.settings.noControlRadius ? Style.settings.labelRadius : height / 2
    property color borderColor: "transparent"
    property real borderWidth: 0
    property int tabWidth: 120
    property var model: []
    signal tabChange(int index)

    width: 244
    height: 40

    readonly property int _texW: Math.max(2, Math.round(root.width  * Screen.devicePixelRatio))
    readonly property int _texH: Math.max(2, Math.round(root.height * Screen.devicePixelRatio))

    // 捕获背景内容
    ShaderEffectSource {
        id: effectSource
        anchors.fill: parent
        sourceItem: root.blurSource
        sourceRect: root.rectXy
        textureSize: Qt.size(root._texW, root._texH)
        mipmap: true
        visible: false
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

    ShaderEffect {
        anchors.fill: parent
        z: 2
        visible: root.blurSource !== null

        property var src: effectSource
        property real blur: root.blurAmount
        property real blurMax: root.blurMax
        property real saturation: 1.4
        property real corner: root.borderRadius
        property vector2d cardSize: Qt.vector2d(root.width, root.height)
        property vector2d texSize: Qt.vector2d(root._texW, root._texH)
        property real aa: 1.0
        property color fillColor: Style.themes.secondaryColor

        fragmentShader: "qrc:/shaders/shaders/cardblur.frag.qsb"
    }

    // 叠加主题色, 避免过亮/过透明
    Rectangle {
        id: topCard
        anchors.fill: root
        radius: root.borderRadius
        color: Style.themes.sideBlurColor
        z: 3
        border.color: root.borderColor
        border.width: root.borderWidth
    }

    Rectangle {
        id: topAnine
        y: 3
        x: 3 + tabView.choiceIndex * root.tabWidth
        z: 4
        width: root.tabWidth
        height: root.height - 6
        radius: root.borderRadius
        color: Style.themes.primaryColor
        Behavior on x { NumberAnimation { duration: 300; easing.type: Easing.Bezier; easing.bezierCurve: [ 0.23, 0.06, 0.00, 0.98, 1, 1 ] } }
    }
    Row {
        id: tabView
        anchors.fill: parent
        anchors.margins: 3
        property int choiceIndex: 0
        z: 5
        Repeater {
            model: root.model

            delegate: Item {
                id: navMusic
                width: root.tabWidth
                height: root.height - 6
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
