// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//

pragma ComponentBehavior: Bound
import QtQuick
import QueMusic 1.0

QScrollView {
    id: page
    property int standWidth: 0
    property int containWidth: 0
    property int containX: 0
    property color cardColor: Options.cardColor

    y: 60
    width: parent ? parent.width : 0
    height: parent ? parent.height - 60 : 0
    visible: false

    Column {
        spacing: 16
        padding: 24
        width: page.containWidth
        x: page.containX

        Text {
            height: 40
            color: Theme.fontColor
            verticalAlignment: Text.AlignVCenter
            text: "DeBug"
            font.pixelSize: Style.pageTitle
            font.weight: Font.DemiBold
        }

        Text { text: "本页设置仅供调试，可能会出现崩溃甚至软件失效，如要恢复请到软件配置目录删除"; color: Theme.fontColor; font.pixelSize: Style.textmain }

        QHead { text: "渲染" }

        Rectangle {
            width: page.standWidth
            color: Theme.primaryColor
            radius: Style.cubeRadius
            Column {
                width: parent.width
                padding: 0
                Component.onCompleted: parent.height = height

                SettingItemCard {
                    label: "使用系统标题栏"
                    tip: "改用系统原生标题栏，关闭无边框窗口效果"
                    QSwitch {
                        letRight: true
                        switchTrue: Config.noWindowKit
                        onToggled: {
                            Config.noWindowKit = !Config.noWindowKit;
                            Options.dialog.openSimpleDialog("提示", "重启本应用以生效更改.", null);
                        }
                    }
                }

                SettingItemCard {
                    label: "渲染引擎"
                    tip: "选择界面渲染使用的图形后端，重启后生效"
                    QDrop {
                        choice: Config.gpuRenderMode
                        model: ["系统偏好","OpenGL","Vulkan","Software"]
                        onTransformed: (choiced) => {
                            Config.gpuRenderMode = choiced;
                            Options.dialog.openSimpleDialog("提示", "重启本应用以生效更改.", null);
                        }
                    }
                }

                SettingItemCard {
                    label: "Timer驱动界面"
                    tip: "用QTimer引擎驱动动画，代替Vsync垂直同步"
                    QSwitch {
                        letRight: true
                        switchTrue: Config.timerAnimator
                        onToggled: {
                            Config.timerAnimator = !Config.timerAnimator;
                            Options.dialog.openSimpleDialog("提示", "重启本应用以完全生效更改.", null);
                        }
                    }
                }

                SettingItemCard {
                    label: "QML动画引擎"
                    tip: "选择 QML 动画的驱动方式，重启后生效"
                    QSwitch {
                        letRight: true
                        text: switchTrue ? "Vsync" : "默认"
                        switchTrue: Config.qmlAnimator
                        onToggled: {
                            Config.qmlAnimator = !Config.qmlAnimator;
                            Options.dialog.openSimpleDialog("提示", "重启本应用以完全生效更改.", null);
                        }
                    }
                    bottomLine: false
                }
            }
        }

        QHead { text: "调试" }

        Rectangle {
            width: page.standWidth
            color: Theme.primaryColor
            radius: Style.cubeRadius
            Column {
                width: parent.width
                padding: 0
                Component.onCompleted: parent.height = height

                SettingItemCard {
                    label: "显示渲染帧率"
                    tip: "在界面右上角显示实时帧率"
                    QSwitch {
                        letRight: true
                        switchTrue: Config.displayFps
                        onToggled: Config.displayFps = !Config.displayFps
                    }
                }

                SettingItemCard {
                    label: "调试模式"
                    tip: "打开QueMusic Console调试窗口"
                    QSwitch {
                        letRight: true
                        switchTrue: Config.debug
                        onToggled: Config.debug = !Config.debug
                    }
                    bottomLine: false
                }
            }
        }

        QHead { text: "日志" }

        Rectangle {
            width: page.standWidth
            color: Theme.primaryColor
            radius: Style.cubeRadius
            Column {
                width: parent.width
                padding: 0
                Component.onCompleted: parent.height = height

                SettingItemCard {
                    label: "启用日志"
                    tip: "开启后把运行日志写入本地文件"
                    QSwitch {
                        letRight: true
                        switchTrue: LogManager.enabled
                        onToggled: LogManager.enabled = !LogManager.enabled
                    }
                }

                SettingItemCard {
                    label: "记录等级"
                    tip: "设置写入日志的最低等级"
                    QDrop {
                        choice: LogManager.minimumLevel
                        model: ["调试","信息","警告","错误","致命"]
                        onTransformed: (choiced) => { LogManager.minimumLevel = choiced }
                    }
                }

                SettingItemCard {
                    label: "打开日志目录"
                    tip: "在文件管理器中打开日志文件所在目录"
                    QButton {
                        text: "打开"
                        shadowEnabled: false
                        buttonColor: "transparent"
                        radius: Style.labelRadius
                        borderWidth: 2
                        onClicked: LogManager.openLogFolder()
                    }
                    bottomLine: false
                }
            }
        }

        Rectangle {
            width: page.standWidth
            height: 320
            color: Theme.primaryColor
            radius: Style.cubeRadius
            Column {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 8
                Text {
                    width: parent.width
                    text: "实时日志预览（完整内容见日志文件）"
                    color: Theme.textColor
                    font.pixelSize: Style.text
                }
                Flickable {
                    id: logPreviewFlick
                    width: parent.width
                    height: parent.height - 30
                    clip: true
                    contentWidth: width
                    contentHeight: logPreviewText.height
                    onContentHeightChanged: contentY = Math.max(0, contentHeight - height)
                    Text {
                        id: logPreviewText
                        width: parent.width
                        text: LogManager.logPreview
                        color: Theme.textColor
                        font.pixelSize: Style.text
                        wrapMode: Text.Wrap
                    }
                }
            }
        }
    }
}
