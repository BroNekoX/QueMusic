// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QueMusic 1.0

// 播放器选项：倍速 / 音质 / 输出设备 / A-B 循环 / 睡眠定时 / 淡入淡出 / 跳转步长
QOptionDialog {
    id: options
    title: "播放器选项"
    //cancelText: "音频工作台"
    //cancelIcon: "\uf120"

    width: 640
    headerHeight: 60
    cardColor: Theme.secondaryBlurColor

    // 宿主注入：播放引擎不再靠上下文继承访问宿主的局部 id
    readonly property AudioEngine player: Playback.player

    readonly property list<double> rates: [0.5, 0.75, 1.0, 1.25, 1.5, 2.0]
    readonly property list<int> seekSteps: [3, 5, 10, 15, 30]

    header: QTapBar {
        id: eqTab
        y: 36
        height: 32
        model: ["播放控制", "音频处理", "声道与增益"]
        onIndexChanged: options.goTop();
    }

    options: EqualizerPanel {
        id: panel
        width: parent.width
        //height: 870
        controlWidth: parent.width
        display: eqTab.index + 1
        engine: options.player
        Rectangle {
            width: panel.width
            height: poColumn.height
            color: panel.cardColor
            radius: Style.cubeRadius
            visible: panel.display === 1
            Column {
                id: poColumn
                width: panel.width
                padding: 0

                SettingItemCard {
                    label: "播放倍速"
                    tip: "快速设置播放的速度倍数"
                    controlWidth: 160
                    width: panel.controlWidth
                    controlItem: QDrop {
                        anchors.fill: parent
                        choice: Config.playerRateIndex
                        model: ["0.5x","0.75x","1x-默认","1.25x","1.5x","2x","自定义"]
                        onTransformed: (choiced) => {
                            Config.playerRateIndex = choiced
                            if (choiced !== 6) Playback.player.playbackRate = options.rates[choiced]
                        }
                    }
                }

                SettingItemCard {
                    label: "自定义倍速"
                    tip: "精细自定义播放的速度倍数"
                    controlWidth: 160
                    width: panel.controlWidth
                    opacity: Config.playerRateIndex === 6 ? 1 : 0.5
                    controlItem: QSlider {
                        anchors.fill: parent
                        from: 0.5
                        to: 4.0
                        stepSize: 0.1
                        leftText: true
                        valueText: value.toFixed(1) + "x"
                        value: Playback.player.playbackRate
                        onMoved: {
                            if (Config.playerRateIndex === 6)
                                Playback.player.playbackRate = value
                        }
                    }
                }

                SettingItemCard {
                    label: "音高补偿"
                    tip: "在使用不同播放倍速时保持同种音高"
                    controlWidth: 160
                    width: panel.controlWidth
                    controlItem: QSwitch {
                        anchors.fill: parent
                        letRight: true
                        switchTrue: Playback.player.pitchCompensation
                        onToggled: Playback.player.pitchCompensation = !Playback.player.pitchCompensation
                    }
                }

                SettingItemCard {
                    label: "在线音质"
                    tip: "设置在线音乐默认播放的音质等级"
                    controlWidth: 160
                    width: panel.controlWidth
                    controlItem: QDrop {
                        anchors.fill: parent
                        choice: Config.soundQuality
                        model: ["标准-128k","高清-320k","无损-500+k"]
                        onTransformed: (choiced) => Config.soundQuality = choiced
                    }
                }

                SettingItemCard {
                    label: "默认输出设备"
                    tip: "跟随系统当前默认的音频输出设备播放"
                    controlWidth: 160
                    width: panel.controlWidth
                    controlItem: QSwitch {
                        anchors.fill: parent
                        letRight: true
                        switchTrue: Config.useDefaultDevice
                        onToggled: Config.useDefaultDevice = !Config.useDefaultDevice
                    }
                }

                SettingItemCard {
                    label: "自定输出设备"
                    tip: "指定播放时使用的音频输出设备"
                    controlWidth: 160
                    width: panel.controlWidth
                    opacity: Config.useDefaultDevice ? 0.5 : 1
                    controlItem: QDrop {
                        anchors.fill: parent
                        useId: true
                        choice: Config.audioDevice
                        model: musicDevices.audioOutputs
                        onTransformed: (choiced) => Config.audioDevice = choiced
                    }
                }

                SettingItemCard {
                    label: "A-B 片段循环"
                    tip: "设置起点和终点以在该片段进行播放循环"
                    controlWidth: 180
                    width: panel.controlWidth
                    controlItem: Row {
                        spacing: 8
                        height: 36
                        anchors.right: parent.right
                        QButton {
                            height: 36; radius: 18; shadowEnabled: false
                            fontSize: Style.text
                            text: Playback.abA >= 0 ? Playback.fmt(Playback.abA) : "设 A"
                            tipText: "在当前位置设为起点"
                            buttonColor: Playback.abA >= 0 ? Theme.themeColor : Theme.secondaryColor
                            textColor: Playback.abA >= 0 ? Theme.primaryColor : Theme.fontColor
                            onClicked: Playback.setAbPoint(0)
                        }
                        QButton {
                            height: 36; radius: 18; shadowEnabled: false
                            fontSize: Style.text
                            text: Playback.abArmed ? Playback.fmt(Playback.abB) : "设 B"
                            tipText: "在当前位置设为终点"
                            buttonColor: Playback.abArmed ? Theme.themeColor : Theme.secondaryColor
                            textColor: Playback.abArmed ? Theme.primaryColor : Theme.fontColor
                            onClicked: Playback.setAbPoint(1)
                        }
                        QButton {
                            height: 36; radius: 18; shadowEnabled: false
                            fontSize: Style.text
                            text: "清除"
                            onClicked: Playback.clearAb()
                        }
                    }
                }

                SettingItemCard {
                    label: "睡眠定时"
                    tip: "设置播放完自动睡眠的方式"
                    controlWidth: 160
                    width: panel.controlWidth
                    controlItem: QDrop {
                        anchors.fill: parent
                        choice: Playback.sleepMode
                        model: ["关闭","倒计时","播完本首"]
                        onTransformed: (choiced) => Playback.armSleep(choiced, Config.sleepMinutes)
                    }
                }

                SettingItemCard {
                    label: "定时剩余"
                    tip: "设置定时剩余多长时间睡眠"
                    controlWidth: 160
                    width: panel.controlWidth
                    opacity: Playback.sleepMode === 1 ? 1 : 0.5
                    controlItem: QSlider {
                        anchors.fill: parent
                        from: 1
                        to: 120
                        stepSize: 1
                        leftText: true
                        valueText: value.toString() + "分"
                        value: Config.sleepMinutes
                        onMoved: Config.sleepMinutes = value
                    }
                }

                SettingItemCard {
                    label: "淡入淡出"
                    tip: "切歌或暂停时对音量淡入淡出，避免爆音"
                    controlWidth: 160
                    width: panel.controlWidth
                    controlItem: QSwitch {
                        anchors.fill: parent
                        letRight: true
                        switchTrue: Config.fadeEnabled
                        onToggled: Config.fadeEnabled = !Config.fadeEnabled
                    }
                }

                SettingItemCard {
                    label: "淡变时长"
                    tip: "淡入淡出动画持续的毫秒数"
                    controlWidth: 160
                    width: panel.controlWidth
                    opacity: Config.fadeEnabled ? 1 : 0.5
                    controlItem: QSlider {
                        anchors.fill: parent
                        from: 0
                        to: 500
                        stepSize: 50
                        leftText: true
                        valueText: value.toString() + "ms"
                        value: Config.fadeMs
                        onMoved: Config.fadeMs = value
                    }
                }

                SettingItemCard {
                    label: "跳转步长"
                    tip: "设置跳转的时长"
                    controlWidth: 160
                    width: panel.controlWidth
                    controlItem: QDrop {
                        anchors.fill: parent
                        choice: options.seekSteps.indexOf(Config.seekStep)
                        model: ["3 秒","5 秒","10 秒","15 秒","30 秒"]
                        onTransformed: (choiced) => Config.seekStep = options.seekSteps[choiced]
                    }
                }

                SettingItemCard {
                    label: "随机避免最近"
                    tip: "随机播放时避免重复最近播放的歌曲数量"
                    controlWidth: 160
                    width: panel.controlWidth
                    controlItem: QSlider {
                        anchors.fill: parent
                        from: 0
                        to: 5
                        stepSize: 1
                        leftText: true
                        valueText: value.toString() + "首"
                        value: Config.shuffleAvoid
                        onMoved: Config.shuffleAvoid = value
                    }
                }

                SettingItemCard {
                    label: "自动播放"
                    tip: "当切换歌曲时是否自动播放"
                    controlWidth: 160
                    width: panel.controlWidth
                    bottomLine: false
                    controlItem: QSwitch {
                        anchors.fill: parent
                        letRight: true
                        switchTrue: Config.autoPlay
                        onToggled: Config.autoPlay = !Config.autoPlay
                    }
                }
            }
        }
    }
}
