// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// 颜色选择对话框（继承 QOptionDialog）：二维饱和/明度色板 + HSV 滑块 + 十六进制输入。
// 色板用分层渐变实现，效果对齐 Qt 自带 ColorDialog 的非原生实现。
// 用法：openColor(当前色) 打开，onAccepted 里读 selectedColor。
// 注意：QOptionDialog 的 blurSource 默认指向 main.qml 的 mainLayout，跨文件使用需调用方显式设置。
import QtQuick
import QtQuick.Controls.Basic
import QueMusic 1.0

QOptionDialog {
    id: root

    title: "选择颜色"
    cancelText: "取消"
    confirmText: "确定"

    property color selectedColor: "#7e57c2"
    property real hue: 0.6
    property real sat: 0.9
    property real val: 0.8

    signal accepted()

    function setColor(c: color): void {
        hue = c.hsvHue < 0 ? 0 : c.hsvHue;
        sat = c.hsvSaturation;
        val = c.hsvValue;
        syncFromHsv();
    }

    function openColor(c: color): void {
        setColor(c);
        open();
    }

    function syncFromHsv(): void {
        selectedColor = Qt.hsva(hue, sat, val, 1.0);
        hexInput.inputText = selectedColor.toString().toUpperCase();
        // 直接赋值而不是绑定：拖动滑块会打断 value 上的绑定
        hueSlider.value = Math.round(hue * 359);
        satSlider.value = Math.round(sat * 100);
        valSlider.value = Math.round(val * 100);
    }

    // 输入框里的十六进制优先于滑块
    function applyHex(): void {
        const picked = Qt.color(hexInput.inputText);
        if (picked.valid)
            setColor(picked);
        else
            hexInput.inputText = selectedColor.toString().toUpperCase();
    }

    onConfirm: {
        applyHex();
        root.accepted();
    }

    options: Column {
        width: parent.width
        spacing: 14

        // 二维色板：横向调饱和度、纵向调明度，底色是当前色相
        Rectangle {
            id: picker
            width: parent.width
            height: 170
            radius: Style.settings.labelRadius
            color: Qt.hsva(root.hue, 1, 1, 1)

            Rectangle {
                anchors.fill: parent
                radius: parent.radius
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: "#ffffffff" }
                    GradientStop { position: 1.0; color: "#00ffffff" }
                }
            }

            Rectangle {
                anchors.fill: parent
                radius: parent.radius
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#00000000" }
                    GradientStop { position: 1.0; color: "#ff000000" }
                }
            }

            Rectangle {
                width: 16
                height: 16
                radius: 8
                color: "transparent"
                border.width: 2
                border.color: "#ffffff"
                x: root.sat * (picker.width - width)
                y: (1.0 - root.val) * (picker.height - height)

                Rectangle {
                    anchors.centerIn: parent
                    width: 8
                    height: 8
                    radius: 4
                    color: root.selectedColor
                }
            }

            MouseArea {
                anchors.fill: parent
                function pick(px, py): void {
                    root.sat = Math.max(0, Math.min(1, px / picker.width));
                    root.val = 1.0 - Math.max(0, Math.min(1, py / picker.height));
                    root.syncFromHsv();
                }
                onPressed: mouse => pick(mouse.x, mouse.y)
                onPositionChanged: mouse => pick(mouse.x, mouse.y)
            }
        }

        QSlider {
            id: hueSlider
            x: 68
            width: parent.width - 68
            from: 0
            to: 359
            stepSize: 1
            leftText: true
            valueText: "H " + Math.round(value)
            onMoved: {
                root.hue = value / 359;
                root.syncFromHsv();
            }

            background: Rectangle {
                x: hueSlider.leftPadding
                y: hueSlider.topPadding + hueSlider.availableHeight / 2 - height / 2
                width: hueSlider.availableWidth
                height: 10
                radius: height / 2
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.000000; color: "#ff0000" }
                    GradientStop { position: 0.166666; color: "#ffff00" }
                    GradientStop { position: 0.333333; color: "#00ff00" }
                    GradientStop { position: 0.500000; color: "#00ffff" }
                    GradientStop { position: 0.666666; color: "#0000ff" }
                    GradientStop { position: 0.833333; color: "#ff00ff" }
                    GradientStop { position: 1.000000; color: "#ff0000" }
                }
            }
        }

        QSlider {
            id: satSlider
            x: 68
            width: parent.width - 68
            from: 0
            to: 100
            stepSize: 1
            leftText: true
            valueText: "S " + Math.round(value) + "%"
            onMoved: {
                root.sat = value / 100;
                root.syncFromHsv();
            }

            background: Rectangle {
                x: satSlider.leftPadding
                y: satSlider.topPadding + satSlider.availableHeight / 2 - height / 2
                width: satSlider.availableWidth
                height: 10
                radius: height / 2
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: Qt.hsva(root.hue, 0, root.val, 1) }
                    GradientStop { position: 1.0; color: Qt.hsva(root.hue, 1, root.val, 1) }
                }
            }
        }

        QSlider {
            id: valSlider
            x: 68
            width: parent.width - 68
            from: 0
            to: 100
            stepSize: 1
            leftText: true
            valueText: "V " + Math.round(value) + "%"
            onMoved: {
                root.val = value / 100;
                root.syncFromHsv();
            }

            background: Rectangle {
                x: valSlider.leftPadding
                y: valSlider.topPadding + valSlider.availableHeight / 2 - height / 2
                width: valSlider.availableWidth
                height: 10
                radius: height / 2
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: Qt.hsva(root.hue, root.sat, 0, 1) }
                    GradientStop { position: 1.0; color: Qt.hsva(root.hue, root.sat, 1, 1) }
                }
            }
        }

        Row {
            width: parent.width
            spacing: 12

            Rectangle {
                width: 56
                height: 36
                radius: Style.settings.labelRadius
                color: root.selectedColor
                border.width: 1
                border.color: Style.themes.sideColor
            }

            QInput {
                id: hexInput
                width: parent.width - 68
                onEntered: root.applyHex()
            }
        }
    }
}
