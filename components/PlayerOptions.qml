// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick

// 播放器选项：倍速 / 音质 / 输出设备 / A-B 循环 / 睡眠定时 / 淡入淡出 / 跳转步长
QOptionDialog {
    id: options
    title: "播放器选项"
    dialogContentHeight: 750
    cancelText: "重置"
    cancelIcon: "\uf0c7"

    readonly property var rates: [0.5, 0.75, 1.0, 1.25, 1.5, 2.0]
    readonly property var seekSteps: [3, 5, 10, 15, 30]

    onCancel: {
        Options.settings.playerRateIndex = 2
        mainMedia.playbackRate = 1.0
        Playback.clearAb()
        Playback.stopSleep()
        Playback.muted = false
    }

    Column {
        width: parent.width
        spacing: 16

        SettingItem {
            label: "播放倍速"
            controlWidth: 120
            width: parent.width
            QDrop {
                height: 36
                anchors.right: parent.right
                width: 160
                choice: Options.settings.playerRateIndex
                model: ["0.5x","0.75x","1x-默认","1.25x","1.5x","2x","自定义"]
                onTransformed: (choiced) => {
                    Options.settings.playerRateIndex = choiced
                    if (choiced !== 6) mainMedia.playbackRate = options.rates[choiced]
                }
            }
        }

        SettingItem {
            label: "自定义倍速"
            controlWidth: 160
            width: parent.width
            opacity: Options.settings.playerRateIndex === 6 ? 1 : 0.5
            QSlider {
                height: 36
                width: 160
                anchors.right: parent.right
                from: 0.5
                to: 4.0
                stepSize: 0.1
                leftText: true
                valueText: value.toFixed(1) + "x"
                value: mainMedia.playbackRate
                onMoved: {
                    if (Options.settings.playerRateIndex === 6)
                        mainMedia.playbackRate = value
                }
            }
        }

        SettingItem {
            label: "音高补偿"
            controlWidth: 120
            width: parent.width
            QSwitch {
                height: 36
                width: 160
                anchors.right: parent.right
                letRight: true
                switchTrue: mainMedia.pitchCompensation
                onToggled: mainMedia.pitchCompensation = !mainMedia.pitchCompensation
            }
        }

        SettingItem {
            label: "在线音质"
            controlWidth: 160
            width: parent.width
            QDrop {
                height: 36
                width: 160
                anchors.right: parent.right
                choice: Options.settings.soundQuality
                model: ["标准-144k","高清-320k","无损-500+k"]
                onTransformed: (choiced) => Options.settings.soundQuality = choiced
            }
        }

        SettingItem {
            label: "默认输出设备"
            controlWidth: 120
            width: parent.width
            QSwitch {
                height: 36
                width: 160
                anchors.right: parent.right
                letRight: true
                switchTrue: Options.settings.useDefaultDevice
                onToggled: Options.settings.useDefaultDevice = !Options.settings.useDefaultDevice
            }
        }

        SettingItem {
            label: "自定输出设备"
            controlWidth: 160
            width: parent.width
            opacity: Options.settings.useDefaultDevice ? 0.5 : 1
            QDrop {
                height: 36
                width: 160
                anchors.right: parent.right
                useId: true
                choice: Options.settings.audioDevice
                model: musicDevices.audioOutputs
                onTransformed: (choiced) => Options.settings.audioDevice = choiced
            }
        }

        SettingItem {
            label: "A-B 片段循环"
            controlWidth: 220
            width: parent.width
            Row {
                spacing: 8
                height: 36
                anchors.right: parent.right
                QButton {
                    height: 36; radius: 18; shadowEnabled: false
                    fontSize: Style.settings.text
                    text: Playback.abA >= 0 ? Playback.fmt(Playback.abA) : "设 A"
                    tipText: "在当前位置设为起点"
                    buttonColor: Playback.abA >= 0 ? Style.themes.themeColor : Style.themes.secondaryColor
                    textColor: Playback.abA >= 0 ? Style.themes.primaryColor : Style.themes.fontColor
                    onClicked: Playback.setAbPoint(0)
                }
                QButton {
                    height: 36; radius: 18; shadowEnabled: false
                    fontSize: Style.settings.text
                    text: Playback.abArmed ? Playback.fmt(Playback.abB) : "设 B"
                    tipText: "在当前位置设为终点"
                    buttonColor: Playback.abArmed ? Style.themes.themeColor : Style.themes.secondaryColor
                    textColor: Playback.abArmed ? Style.themes.primaryColor : Style.themes.fontColor
                    onClicked: Playback.setAbPoint(1)
                }
                QButton {
                    height: 36; radius: 18; shadowEnabled: false
                    fontSize: Style.settings.text
                    text: "清除"
                    onClicked: Playback.clearAb()
                }
            }
        }

        SettingItem {
            label: "睡眠定时"
            controlWidth: 160
            width: parent.width
            QDrop {
                height: 36
                width: 160
                anchors.right: parent.right
                choice: Playback.sleepMode
                model: ["关闭","倒计时","播完本首"]
                onTransformed: (choiced) => Playback.armSleep(choiced, Options.settings.sleepMinutes)
            }
        }

        SettingItem {
            label: "定时剩余"
            controlWidth: 200
            width: parent.width
            opacity: Playback.sleepMode === 1 ? 1 : 0.5
            QSlider {
                height: 36
                width: 160
                anchors.right: parent.right
                from: 1
                to: 120
                stepSize: 1
                valueText: value.toString() + "分"
                value: Options.settings.sleepMinutes
                onMoved: Options.settings.sleepMinutes = value
            }
        }

        SettingItem {
            label: "淡入淡出"
            controlWidth: 120
            width: parent.width
            QSwitch {
                height: 36
                width: 160
                anchors.right: parent.right
                letRight: true
                switchTrue: Options.settings.fadeEnabled
                onToggled: Options.settings.fadeEnabled = !Options.settings.fadeEnabled
            }
        }

        SettingItem {
            label: "淡变时长"
            controlWidth: 160
            width: parent.width
            opacity: Options.settings.fadeEnabled ? 1 : 0.5
            QSlider {
                height: 36
                width: 160
                anchors.right: parent.right
                from: 0
                to: 2000
                stepSize: 100
                leftText: true
                valueText: value.toString() + "ms"
                value: Options.settings.fadeMs
                onMoved: Options.settings.fadeMs = value
            }
        }

        SettingItem {
            label: "跳转步长"
            controlWidth: 160
            width: parent.width
            QDrop {
                height: 36
                width: 160
                anchors.right: parent.right
                choice: options.seekSteps.indexOf(Options.settings.seekStep)
                model: ["3 秒","5 秒","10 秒","15 秒","30 秒"]
                onTransformed: (choiced) => Options.settings.seekStep = options.seekSteps[choiced]
            }
        }

        SettingItem {
            label: "随机避免最近"
            controlWidth: 160
            width: parent.width
            QSlider {
                height: 36
                width: 160
                anchors.right: parent.right
                from: 0
                to: 5
                stepSize: 1
                leftText: true
                valueText: value.toString() + "首"
                value: Options.settings.shuffleAvoid
                onMoved: Options.settings.shuffleAvoid = value
            }
        }

        SettingItem {
            label: "自动播放"
            controlWidth: 120
            width: parent.width
            QSwitch {
                height: 36
                width: 160
                anchors.right: parent.right
                letRight: true
                switchTrue: Options.settings.autoPlay
                onToggled: Options.settings.autoPlay = !Options.settings.autoPlay
            }
        }
    }
}
