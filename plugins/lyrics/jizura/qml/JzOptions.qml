// SPDX-License-Identifier: MIT
//
// JIZURA × QueMusic — the settings block this plugin contributes to the player's
// "播放器样式" dialog (host contract: `property Component styleOptions`).
//
// It uses the application's own controls (SettingItem / QDrop / QSlider /
// QSwitch / QHead from `import QueMusic 1.0`) so the panel matches every other
// settings page, rather than QtQuick.Controls.
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtCore
import QueMusic 1.0
import "../js/labels.js" as Labels

Item {
    id: settingStack                 // resolved by SettingItem's width binding
    property int standWidth: width

    // Style keys the engine actually offers, for the drop-down.  A plain array of
    // strings handed over once by the plugin - a value, not a live object, so it
    // stays valid after the plugin is gone.  Falls back to the catalogue.
    property var styleKeys: []

    // the plugin's settings, shared through the settings file
    Settings {
        id: js
        category: "JizuraLyrics"
        property string styleKey: "noir"
        property string qualityMode: "auto"
        property bool useCoverColors: true
        property bool unify: false
        property bool typeset: false
        property int seedMode: 1
        property int fixedSeed: 20260922
        property real fxMotion: 0.7
        property real fxGlitch: 0.55
        property real fxChroma: 0.7
        property real fxDecor: 0.5
        property real fxDensity: 0.55
        property real textScale: 1.0
        property string rendererMode: "geometry"
        property string komaMode: "smooth"
    }


    readonly property var keys: settingStack.styleKeys.length
                                ? settingStack.styleKeys : Labels.styleKeys()
    readonly property var qualityKeys: ["auto", "high", "medium", "low", "debug"]
    readonly property var qualityLabels: ["自动（按帧时升降）", "全部效果", "无转场 / 无色差", "仅歌词", "调试信息"]
    readonly property var seedLabels: ["固定种子", "每首歌一致", "每次播放都不同"]

    implicitHeight: column.implicitHeight
    implicitWidth: column.implicitWidth

    Column {
        id: column
        width: parent ? parent.width : 0
        spacing: 16

        // ---------------------------------------------------------------- 编排
        QHead { text: "编排风格" }

        SettingItem {
            label: "风格"
            width: parent.width
            Text {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                width: Math.min(parent.width - 30,implicitWidth)
                visible: text.length > 0
                wrapMode: Text.WordWrap
                color: Theme.textColor
                font.pixelSize: Style.textTip
                text: Labels.styleDesc(js.styleKey)
            }
        }

        Flow {
            id: cardFlow
            spacing: 6
            y: 36
            x: 16
            width: parent.width - 32
            clip: true
            property int choice: Math.max(0, settingStack.keys.indexOf(js.styleKey))
            Repeater {
                model: settingStack.keys.map(function (k) { return Labels.styleName(k) })
                delegate: Rectangle {
                    id: cardDel
                    required property string modelData
                    required property int index
                    width: choiceText.implicitWidth + 20
                    height: choiceText.height + 16
                    color: cardFlow.choice === index ? Theme.containColor : Theme.secondaryColor
                    radius: Style.labelRadius
                    border.color: Theme.primaryColor
                    border.width: 1
                    Rectangle {
                        radius: parent.radius
                        anchors.fill: parent
                        color: Theme.hoverColor
                        opacity: choiceArea.containsMouse ? 1 : 0
                        Behavior on opacity { NumberAnimation { duration: 120 } }
                    }

                    Text {
                        id: choiceText
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: 8
                        anchors.right: parent.right
                        anchors.rightMargin: 8
                        //超出部分省略号截断，强制单行
                        elide: Text.ElideRight
                        maximumLineCount: 1
                        text: cardDel.modelData
                        color: Theme.fontColor
                        font.pixelSize: Style.text
                    }
                    MouseArea {
                        id: choiceArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            js.styleKey = settingStack.keys[cardDel.index];
                        }
                    }
                }
            }
        }

        SettingItem {
            label: "随机种子"
            width: parent.width
            QDrop {
                anchors.right: parent.right
                width: 200
                height: 36
                model: settingStack.seedLabels
                choice: js.seedMode
                onTransformed: (choiced) => {
                    js.seedMode = choiced;
                }
            }
        }

        SettingItem {
            label: "种子数值"
            width: parent.width
            visible: js.seedMode === 0
            height: visible ? 36 : 0
            QSlider {
                anchors.right: parent.right
                from: 1
                to: 99999
                stepSize: 1
                width: 200
                height: 36
                leftText: true
                valueText: String(value)
                value: js.fixedSeed
                onMoved: {
                    js.fixedSeed = value;
                }
            }
        }

        SettingItem {
            label: "跟随封面配色"
            width: parent.width
            QSwitch {
                height: 36
                width: 120
                anchors.right: parent.right
                switchTrue: js.useCoverColors
                onToggled: {
                    js.useCoverColors = !js.useCoverColors;
                }
            }
        }
        SettingItem {
            label: "统一感"
            width: parent.width
            QSwitch {
                height: 36
                width: 120
                anchors.right: parent.right
                switchTrue: js.unify
                onToggled: {
                    js.unify = !js.unify;
                }
            }
        }
        SettingItem {
            label: "文字整列"
            width: parent.width
            QSwitch {
                height: 36
                width: 120
                anchors.right: parent.right
                switchTrue: js.typeset
                onToggled: {
                    js.typeset = !js.typeset;
                }
            }
        }

        // ------------------------------------------------------------ 画质
        QHead { text: "画质与性能" }

        SettingItem {
            label: "画质"
            width: parent.width
            QDrop {
                anchors.right: parent.right
                width: 200
                height: 36
                model: settingStack.qualityLabels
                choice: Math.max(0, settingStack.qualityKeys.indexOf(js.qualityMode))
                onTransformed: (choiced) => {
                    js.qualityMode = settingStack.qualityKeys[choiced];
                }
            }
        }

        SettingItem {
            label: "图形后端"
            width: parent.width
            QDrop {
                anchors.right: parent.right
                width: 200
                height: 36
                model: ["Geometry（Qt 默认，稳）", "Curve（GPU，快）"]
                choice: js.rendererMode === "curve" ? 1 : 0
                onTransformed: (choiced) => {
                    js.rendererMode = choiced === 1 ? "curve" : "geometry";
                }
            }
        }

        SettingItem {
            label: "画面步进"
            width: parent.width
            QDrop {
                anchors.right: parent.right
                width: 200
                height: 36
                model: ["流畅（每渲染帧）", "动画二拍（12 帧/秒）", "跟随风格"]
                choice: js.komaMode === "smooth" ? 0 : js.komaMode === "onTwos" ? 1 : 2
                onTransformed: (choiced) => {
                    js.komaMode = choiced === 0 ? "smooth" : choiced === 1 ? "onTwos" : "auto";
                }
            }
        }

        SettingItem {
            label: "整体缩放"
            width: parent.width
            QSlider {
                anchors.right: parent.right
                from: 85
                to: 120
                stepSize: 1
                width: 200
                height: 36
                leftText: true
                valueText: (value / 100).toFixed(2) + "x"
                value: Math.round(js.textScale * 100)
                onMoved: {
                    js.textScale = value / 100;
                }
            }
        }

        // ------------------------------------------------------------ 效果
        QHead { text: "效果强度" }

        SettingItem {
            label: "动感"
            width: parent.width
            QSlider {
                anchors.right: parent.right
                from: 0
                to: 100
                stepSize: 5
                width: 200
                height: 36
                leftText: true
                valueText: value + "%"
                value: Math.round(js.fxMotion * 100)
                onMoved: {
                    js.fxMotion = value / 100;
                }
            }
        }
        SettingItem {
            label: "故障感"
            width: parent.width
            QSlider {
                anchors.right: parent.right
                from: 0
                to: 100
                stepSize: 5
                width: 200
                height: 36
                leftText: true
                valueText: value + "%"
                value: Math.round(js.fxGlitch * 100)
                onMoved: {
                    js.fxGlitch = value / 100;
                }
            }
        }
        SettingItem {
            label: "色差重影"
            width: parent.width
            QSlider {
                anchors.right: parent.right
                from: 0
                to: 140
                stepSize: 5
                width: 200
                height: 36
                leftText: true
                valueText: value + "%"
                value: Math.round(js.fxChroma * 100)
                onMoved: {
                    js.fxChroma = value / 100;
                }
            }
        }
        SettingItem {
            label: "装饰量"
            width: parent.width
            QSlider {
                anchors.right: parent.right
                from: 0
                to: 150
                stepSize: 5
                width: 200
                height: 36
                leftText: true
                valueText: value + "%"
                value: Math.round(js.fxDecor * 100)
                onMoved: {
                    js.fxDecor = value / 100;
                }
            }
        }
        SettingItem {
            label: "切镜密度"
            width: parent.width
            QSlider {
                anchors.right: parent.right
                from: 0
                to: 100
                stepSize: 5
                width: 200
                height: 36
                leftText: true
                valueText: value + "%"
                value: Math.round(js.fxDensity * 100)
                onMoved: {
                    js.fxDensity = value / 100;
                }
            }
        }

        // ------------------------------------------------------------ 说明
        Text {
            width: parent.width
            wrapMode: Text.WordWrap
            color: Theme.textColor
            font.pixelSize: Style.textTip
            lineHeight: 1.3
            text: "JIZURA 会把整首歌词自动编排成一段文字 PV：版式、入场出场、装饰与转场都由原版引擎决定，"
                + "画面用 QtQuick.Shapes 逐帧绘制。画质设为「自动」时会按实际帧时在"
                + "「色差重影 → 转场 → 背景图形 → 仅歌词」之间自动升降。"
                + "帧时与帧内计数可在画质「调试信息」下于画面左下角查看。"
        }
    }
}
