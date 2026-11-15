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
    property color cardColor: Theme.primaryColor
    property Item blurSource: null

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
            text: "界面"
            font.pixelSize: Style.pageTitle
            font.weight: Font.DemiBold
        }
        QHead { text: "界面样式" }

        Rectangle {
            width: page.standWidth
            color: page.cardColor
            radius: Style.cubeRadius
            height: jmysColumn.height
            Column {
                id: jmysColumn
                width: page.standWidth
                padding: 0
                SettingItemCard {
                    label: "左栏沉浸"
                    tip: "左侧的页面切换栏将使用与背景一致的色彩，提高观感"
                    QSwitch {
                        letRight: true
                        switchTrue: Style.sidebarColor
                        onToggled: {
                            Style.sidebarColor = !Style.sidebarColor;
                            Options.changeTheme();
                        }
                    }
                }

                SettingItemCard {
                    label: "左栏样式"
                    tip: "左侧的页面切换栏样式"
                    QDrop {
                        choice: Style.sidebarStyle
                        model: ["Basic","Fancy"]
                        onTransformed: (choiced) => {
                            Style.sidebarStyle = choiced;
                            Options.changeTheme();
                        }
                    }
                }

                SettingItemCard {
                    label: "左栏缩小方式"
                    tip: "通过设置左栏如何缩小达到主内容更佳的显示面积"
                    QDrop {
                        choice: Style.leftBarMode
                        model: ["通过窗口宽度","保持展开","保持缩小"]
                        onTransformed: (choiced) => {
                            Style.leftBarMode = choiced;
                        }
                    }
                }

                SettingItemCard {
                    label: "不透明底栏"
                    tip: "播放器控制底栏将不再透明"
                    QSwitch {
                        letRight: true
                        switchTrue: Style.noOpacityControl
                        onToggled: Style.noOpacityControl = !Style.noOpacityControl
                    }
                }

                SettingsItemVisibleCard {
                    label: "封面色底栏"
                    tip: "播放器控制底栏将使用和当前歌曲封面相同的主题色"
                    display: Style.noOpacityControl
                    QSwitch {
                        letRight: true
                        switchTrue: Style.coverColorControl
                        onToggled: Style.coverColorControl = !Style.coverColorControl
                    }
                }

                SettingItemCard {
                    label: "全局字体"
                    tip: "设置整个应用程序默认显示的字体样式"
                    QDrop {
                        choice: Style.globalFont
                        model: ["Figtree(en)","系统默认","自定义"]
                        onTransformed: (choiced) => {
                            Style.globalFont = choiced;
                        }
                    }
                }

                SettingsItemVisibleCard {
                    label: "自定义字体"
                    display: Style.globalFont === 2
                    tip: "自定义整个应用程序显示的字体样式"
                    QButton {
                        shadowEnabled: false
                        radius: Style.labelRadius
                        borderWidth: 2
                        text: Style.fontStyle ? Style.fontStyle : "系统默认"
                        fontSize: Style.text
                        onClicked: fontDialog.openFamily(Style.fontStyle)
                    }
                    FontFamilyDialog {
                        id: fontDialog
                        blurSource: page.blurSource
                        currentFamily: Style.fontStyle
                        onAccepted: Style.fontStyle = fontDialog.selectedFamily
                    }
                }

                SettingItemCard {
                    label: "卡片圆角大小"
                    tip: "设置全局大型卡片的圆角大小"
                    QSlider {
                        from: 0
                        to: 24
                        stepSize: 2
                        leftText: true
                        valueText: value
                        value: Style.cubeRadius
                        onMoved: {
                            Style.cubeRadius = value
                        }
                    }
                }

                SettingItemCard {
                    label: "控件圆角大小"
                    tip: "设置全局控件的圆角大小"
                    QSlider {
                        from: 0
                        to: 24
                        stepSize: 2
                        leftText: true
                        valueText: value
                        value: Style.labelRadius
                        onMoved: {
                            Style.labelRadius = value
                        }
                    }
                }

                SettingItemCard {
                    label: "不使用控件大圆角"
                    tip: "(废弃)将部分完全圆角的控件取消完全圆角"
                    QSwitch {
                        letRight: true
                        switchTrue: Style.noControlRadius
                        onToggled: Style.noControlRadius = !Style.noControlRadius
                    }
                    bottomLine: false
                }
            }
        }

        QHead { text: "界面效果" }

        Rectangle {
            width: page.standWidth
            color: page.cardColor
            radius: Style.cubeRadius
            Column {
                width: page.standWidth
                padding: 0
                Component.onCompleted: {
                    parent.height = height
                }
                SettingItemCard {
                    label: "高级材质"
                    tip: "使用更高质量的背景模糊提升观感"
                    QSwitch {
                        letRight: true
                        switchTrue: Style.highQualityBlur
                        onToggled: Style.highQualityBlur = !Style.highQualityBlur
                    }
                }

                SettingItemCard {
                    label: "卡片阴影模糊度"
                    tip: "设置全局卡片的阴影模糊程度"
                    QSlider {
                        from: 8
                        to: 32
                        stepSize: 2
                        leftText: true
                        valueText: value
                        value: Style.shadowSize
                        onMoved: {
                            Style.shadowSize = value
                        }
                    }
                }

                SettingItemCard {
                    label: "卡片背景模糊度"
                    tip: "设置全局卡片的背景模糊程度"
                    QSlider {
                        from: 16
                        to: 64
                        stepSize: 2
                        leftText: true
                        valueText: value
                        value: Style.blurSize
                        onMoved: {
                            Style.blurSize = value
                        }
                    }
                }

                SettingItemCard {
                    label: "子页面切入切出动画"
                    tip: "在打开或关闭子页面所使用的动画效果"
                    QDrop {
                        choice: Style.animeType
                        model: ["放大缩小","左右渐入"]
                        onTransformed: (choiced) => {
                            Style.animeType = choiced;
                        }
                    }
                }

                SettingItemCard {
                    label: "高级动画效果"
                    tip: "启动更精细的动画效果"
                    QSwitch {
                        letRight: true
                        switchTrue: Style.premiumAnime
                        onToggled: Style.premiumAnime = !Style.premiumAnime
                    }
                    bottomLine: false
                }
            }
        }

        QHead { text: "播放器歌词模式" }

        Rectangle {
            width: page.standWidth
            color: page.cardColor
            radius: Style.cubeRadius
            Column {
                width: page.standWidth
                padding: 0
                Component.onCompleted: {
                    parent.height = height
                }
                SettingItemCard {
                    label: "标准歌词大小"
                    tip: "设置歌词显示字体大小(会跟随窗口大小改变)"
                    QSlider {
                        from: 0
                        to: 20
                        stepSize: 2
                        leftText: true
                        valueText: value
                        value: Style.lyricSize
                        onMoved: {
                            Style.lyricSize = value
                        }
                    }
                }

                SettingItemCard {
                    label: "歌词字重"
                    tip: "设置歌词显示的字重"
                    QSlider {
                        from: 300
                        to: 800
                        stepSize: 50
                        leftText: true
                        valueText: value
                        value: Style.textWidth
                        onMoved: {
                            Style.textWidth = value
                        }
                    }
                }

                SettingItemCard {
                    label: "歌词字体"
                    tip: "设置歌词显示的字体"
                    QButton {
                        shadowEnabled: false
                        radius: Style.labelRadius
                        borderWidth: 2
                        text: Style.fontFamily ? Style.fontFamily : "系统默认"
                        fontSize: Style.text
                        onClicked: lyricFontDialog.openFamily(Style.fontFamily)
                    }
                    FontFamilyDialog {
                        id: lyricFontDialog
                        blurSource: page.blurSource
                        currentFamily: Style.fontFamily
                        onAccepted: Style.fontFamily = lyricFontDialog.selectedFamily
                    }
                }

                SettingItemCard {
                    label: "背景着色器样式"
                    tip: "设置沉浸播放页的流体背景算法，静态渐变最省性能"
                    isBigItem: true
                    QWideDrop {
                        model: ["Fluid", "Classic", "静态渐变"]
                        choice: Style.flowStyle
                        onTransformed: (choiced) => {
                            Style.flowStyle = choiced
                        }
                    }
                }

                SettingItemCard {
                    label: "显示音波效果"
                    tip: "在沉浸播放页显示实时音波频谱"
                    QSwitch {
                        letRight: true
                        switchTrue: Style.waveDisplay
                        onToggled: Style.waveDisplay = !Style.waveDisplay
                    }
                }

                SettingItemCard {
                    label: "歌词渐进模糊"
                    tip: "让非当前行歌词渐进模糊，突出正在播放的一句"
                    QSwitch {
                        letRight: true
                        switchTrue: Style.maskBlur
                        onToggled: Style.maskBlur = !Style.maskBlur
                    }
                }

                SettingItemCard {
                    label: "弹簧动画过冲度"
                    tip: "歌词逐行切换时弹簧动画弹过位置的弹动效果强度"
                    QSlider {
                        from: 0
                        to: 4
                        stepSize: 1
                        leftText: true
                        valueText: value + 1
                        value: Style.springValue
                        onMoved: {
                            Style.springValue = value
                        }
                    }
                }

                SettingItemCard {
                    label: "自动进入沉浸模式"
                    tip: "进入沉浸播放页后自动隐藏界面控件"
                    QSwitch {
                        letRight: true
                        switchTrue: Style.lyricHideGui
                        onToggled: Style.lyricHideGui = !Style.lyricHideGui
                    }
                    bottomLine: false
                }
            }
        }
    }
}
