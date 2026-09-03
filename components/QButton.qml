// QButton.qml
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects

Button {
    id: root

    // ==== 外部接口 ====
    text: "Button"              // 按钮文字
    property string iconCharacter: ""          // 图标字符
    property string iconFontFamily: iconFont.name    // 图标字体
    property string tipText: ""                // 鼠标悬停提示文字（为空则不显示）

    // ==== 样式 ====
    property color textColor: Style.themes.fontColor      // 文字颜色
    property color iconColor: Style.themes.fontColor
    property bool shadowEnabled: true              // 是否显示阴影
    property int textBetween: 6                   // 图标与文字间距
    property int borderWidth: 0   // 边框大小，0即无
    property color borderColor: Style.themes.borderColor   // 边框颜色
    property int radius: Style.settings.noControlRadius ? Style.settings.labelRadius : 20
    property real pressedScale: 0.92                 // 按下缩放比例
    property color shadowColor: Style.themes.shadowColor    // 阴影颜色
    property color hoverColor: Style.themes.hoverColor
    property color buttonColor: Style.themes.primaryColor
    property int iconSize: Style.settings.texticon
    property int fontSize: Style.settings.text

    // ==== 尺寸控制 ====
    //horizontalPadding: 16
    height: 36
    width: rowItem.width + 32//layout.width + horizontalPadding * 2
    Behavior on width { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }
    
    Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutExpo } }
    onPressed: scale = pressedScale
    onReleased: scale = 1.0
    onCanceled: scale = 1.0
    
    background: Rectangle {
        width: root.width
        height: root.height
        color: root.buttonColor
        Behavior on color { ColorAnimation { duration: 120 } }
        radius: root.radius
        border.width: root.borderWidth
        border.color: root.borderColor
        //anchors.fill: parent
        RectangularShadow {
            anchors.fill: parent
            z: -1
            offset.x: 3
            offset.y: 5
            radius: root.radius
            blur: 24
            spread: 0
            color: root.shadowColor
            visible: root.shadowEnabled
        }
        Rectangle {
            anchors.fill: parent
            color: Style.themes.hoverColor
            radius: root.radius
            opacity: root.hovered ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 80 } }
        }
    }
    
    contentItem: Item {
        anchors.fill: parent
        clip: true
        Row {
            id: rowItem
            anchors.centerIn: parent
            spacing: root.textBetween
        
            Text {
                width: root.iconSize
                height: root.iconSize
                text: root.iconCharacter
                visible: root.iconCharacter !== ""
                color: root.iconColor
                font.pixelSize: root.iconSize
                font.family: root.iconFontFamily
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }
            Text {
                height: root.iconSize
                text: root.text
                color: root.textColor
                font.pixelSize: root.fontSize
                font.bold: false
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    QTip {
        visible: root.tipText !== "" && root.hovered
        text: root.tipText
    }
}