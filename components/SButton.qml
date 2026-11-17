// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QueMusic 1.0
import QtQuick.Templates

AbstractButton {
    id: root
    padding: 0

    // ==== 外部接口 ====
    property string iconCharacter: ""          // 图标字符 (FontAwesome 等)
    property string iconFontFamily: Fonts.icon    // 图标字体
    property string tipText: ""                // 鼠标悬停提示文字（为空则不显示）
    readonly property bool tipEnabled: tipText !== ""

    // ==== 样式 ====
    property color iconColor: Theme.textColor
    property int borderWidth: 0   // 边框大小，0即无
    property color borderColor: Theme.sideColor   // 边框颜色
    property int radius: Style.noControlRadius ? Style.labelRadius : 20
    property real pressedScale: 0.92                 // 按下缩放比例
    property color hoverColor: Theme.hoverColor
    property color buttonColor: Theme.secondaryColor
    property int iconSize: Style.texticon

    // ==== 尺寸控制 ====
    implicitHeight: 40
    implicitWidth: 40

    onHoveredChanged: {
        if(hovered && tipEnabled) {
            Options.mainTip.parent = root;
            Options.mainTip.text = root.tipText;
            Options.mainTip.visible = true;
        } else {
            Options.mainTip.visible = false;
        }
    }
    
    background: Rectangle {
        width: root.width
        height: root.height
        color: root.buttonColor
        radius: root.radius
        border.width: root.borderWidth
        border.color: root.borderColor
        Rectangle {
            anchors.fill: parent
            color: root.hoverColor
            radius: root.radius
            opacity: root.hovered ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 88 } }
        }
    }
    
    contentItem: Text {
        scale: root.pressed ? root.pressedScale : 1.0
        Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
        anchors.fill: parent
        text: root.iconCharacter
        color: root.iconColor
        font.pixelSize: root.iconSize
        font.family: root.iconFontFamily
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
    }
}
