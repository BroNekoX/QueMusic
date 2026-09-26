// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// 均衡器与音频处理面板：10 段参数均衡 + 声道工具 + ReplayGain + 限幅喵~
import QtQuick
import QtQuick.Effects
import QueMusic 1.0

Column {
    id: panel
    spacing: 16

    property AudioEngine engine

    readonly property int eqBands: 10
    readonly property real eqMax: 24
    readonly property int faderHeight: 148
    readonly property int trackTop: 6
    readonly property int trackHeight: faderHeight - 40
    readonly property var sampleRates: [0, 44100, 48000, 88200, 96000, 192000]
    property int controlWidth: settingStack.standWidth

    QHead { width: panel.width; text: "均衡器与音频处理" }

    Rectangle {
        width: panel.width
        height: statusRow.height + 20
        color: Style.themes.primaryColor
        radius: Style.settings.cubeRadius

        Row {
            id: statusRow
            x: 18
            y: 10
            spacing: 24
            Text {
                font.pixelSize: Style.settings.textTip
                color: Style.themes.textColor
                verticalAlignment: Text.AlignVCenter
                text: panel.engine
                      ? (panel.engine.codecName + " · " + panel.engine.sourceSampleRate + " Hz · "
                         + panel.engine.sourceChannels + "ch · " + panel.engine.bitRate + " kbps")
                      : "无音源"
            }
            Text {
                font.pixelSize: Style.settings.textTip
                color: Style.themes.textColor
                verticalAlignment: Text.AlignVCenter
                text: panel.engine ? "输出 " + panel.engine.outputFormatName : ""
            }
            Text {
                font.pixelSize: Style.settings.textTip
                color: Style.themes.textColor
                verticalAlignment: Text.AlignVCenter
                text: panel.engine
                      ? "解码 " + panel.engine.decodeSampleRate + " Hz · 缓冲 " + panel.engine.bufferMs + " ms"
                      : ""
            }
            Text {
                font.pixelSize: Style.settings.textTip
                color: panel.engine && panel.engine.dspActive ? Style.themes.themeColor : Style.themes.textColor
                verticalAlignment: Text.AlignVCenter
                text: panel.engine && panel.engine.dspActive
                      ? "DSP 已介入 · 增益 " + panel.engine.effectiveGainDb.toFixed(1) + " dB"
                      : "直通（未做有损处理）"
            }
        }
    }

    Rectangle {
        width: panel.width
        color: Style.themes.primaryColor
        radius: Style.settings.cubeRadius
        height: eqColumn.height

        Column {
            id: eqColumn
            width: parent.width
            padding: 0

            SettingItemCard {
                label: "音频处理总开关"
                tip: "关闭后全链直通，不做均衡/限幅/声道处理"
                width: panel.controlWidth
                controlItem: QSwitch {
                    anchors.fill: parent
                    letRight: true
                    switchTrue: panel.engine ? panel.engine.dspEnabled : true
                    onToggled: {
                        if (panel.engine)
                            panel.engine.dspEnabled = !panel.engine.dspEnabled
                    }
                }
            }

            SettingItemCard {
                label: "输出采样率"
                tip: "设备默认时不重采样；改此项会重开输出并保留播放位置"
                width: panel.controlWidth
                controlItem: QDrop {
                    anchors.fill: parent
                    choice: panel.engine
                            ? Math.max(0, panel.sampleRates.indexOf(panel.engine.outputSampleRate))
                            : 0
                    model: ["设备默认", "44100 Hz", "48000 Hz", "88200 Hz", "96000 Hz", "192000 Hz"]
                    onTransformed: (choiced) => {
                        if (panel.engine)
                            panel.engine.outputSampleRate = panel.sampleRates[choiced]
                    }
                }
            }

            SettingItemCard {
                label: "输出缓冲"
                tip: "范围 10-100 ms，越小延迟越低；默认 30 ms"
                width: panel.controlWidth
                controlItem: QSlider {
                    anchors.fill: parent
                    live: false
                    from: 10
                    to: 100
                    stepSize: 10
                    leftText: true
                    valueText: value + " ms"
                    value: panel.engine ? panel.engine.bufferMs : 30
                    onMoved: {
                        if (panel.engine)
                            panel.engine.bufferMs = value
                    }
                }
            }

            SettingItemCard {
                label: "音高"
                tip: "半音为单位，±12 半音（一个八度）；不影响播放速度"
                width: panel.controlWidth
                controlItem: QSlider {
                    anchors.fill: parent
                    from: -12
                    to: 12
                    stepSize: 1
                    leftText: true
                    valueText: (value > 0 ? "+" : "") + value + " 半音"
                    value: panel.engine ? panel.engine.pitchSemitones : 0
                    onMoved: {
                        if (panel.engine)
                            panel.engine.pitchSemitones = value
                    }
                }
            }

            SettingItemCard {
                label: "启用均衡器"
                tip: "10 段参数均衡，增益范围 ±24 dB"
                width: panel.controlWidth
                controlItem: QSwitch {
                    anchors.fill: parent
                    letRight: true
                    switchTrue: panel.engine ? panel.engine.eqEnabled : false
                    onToggled: {
                        if (panel.engine)
                            panel.engine.eqEnabled = !panel.engine.eqEnabled
                    }
                }
            }

            SettingItemCard {
                label: "预设"
                tip: "套用后可直接微调各频段"
                width: panel.controlWidth
                controlItem: QDrop {
                    anchors.fill: parent
                    choice: 0
                    model: panel.engine ? panel.engine.eqPresetNames() : []
                    onTransformed: (choiced) => {
                        if (panel.engine) panel.engine.applyEqPreset(model[choiced]);
                        choice = choiced;
                    }
                }
            }

            SettingItemCard {
                label: "前级增益"
                tip: "整体电平补偿，避免 EQ 抬升后削波"
                width: panel.controlWidth
                controlItem: QSlider {
                    anchors.fill: parent
                    from: -24
                    to: 24
                    stepSize: 0.5
                    leftText: true
                    valueText: value.toFixed(1) + " dB"
                    value: panel.engine ? panel.engine.preampDb : 0
                    onMoved: {
                        if (panel.engine)
                            panel.engine.preampDb = value
                    }
                }
            }

            SettingItemCard {
                label: "频段 Q 值"
                tip: "越小过渡越平缓，越大越窄；影响全部频段"
                width: panel.controlWidth
                controlItem: QSlider {
                    anchors.fill: parent
                    from: 0.3
                    to: 4
                    stepSize: 0.05
                    leftText: true
                    valueText: value.toFixed(2)
                    value: panel.engine ? panel.engine.eqQ : 1.41
                    onMoved: {
                        if (panel.engine)
                            panel.engine.eqQ = value
                    }
                }
            }

            SettingItemCard {
                label: "自动余量"
                tip: "按 EQ 最大抬升量自动回退增益，降低削波风险"
                width: panel.controlWidth
                controlItem: QSwitch {
                    anchors.fill: parent
                    letRight: true
                    switchTrue: panel.engine ? panel.engine.autoHeadroom : true
                    onToggled: {
                        if (panel.engine)
                            panel.engine.autoHeadroom = !panel.engine.autoHeadroom
                    }
                }
            }

            Item {
                width: parent.width
                height: panel.faderHeight + 26

                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    spacing: 2

                    Repeater {
                        model: panel.engine ? panel.engine.eqBandLabels() : []

                        delegate: Item {
                            id: fader
                            width: 46
                            height: panel.faderHeight
                            readonly property real gain: panel.engine ? panel.engine.eqGains[index] : 0
                            readonly property real trackY: panel.trackTop
                            readonly property real trackH: panel.trackHeight
                            readonly property real centerY: trackY + trackH / 2

                            Rectangle {
                                id: track
                                x: (parent.width - width) / 2
                                y: fader.trackY
                                width: 6
                                height: fader.trackH
                                radius: 3
                                color: Style.themes.sideColor
                            }

                            Rectangle {
                                x: (parent.width - 22) / 2
                                y: fader.centerY
                                width: 22
                                height: 1
                                color: Style.themes.borderColor
                            }

                            Rectangle {
                                x: (parent.width - width) / 2
                                width: 6
                                radius: 3
                                color: Style.themes.themeColor
                                y: fader.gain >= 0 ? fader.centerY - height : fader.centerY
                                height: Math.abs(fader.gain) / panel.eqMax * fader.trackH / 2
                            }

                            Rectangle {
                                id: knob
                                width: 14
                                height: 14
                                radius: width / 2
                                scale: drag.pressed ? 1.3 : 1
                                x: (parent.width - width) / 2
                                y: fader.centerY - fader.gain / panel.eqMax * fader.trackH / 2 - height / 2
                                color: "#ffffff"
                                Behavior on scale { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }

                                RectangularShadow {
                                    anchors.fill: parent
                                    z: -1
                                    offset.y: 1
                                    radius: parent.width / 2
                                    blur: 6
                                    spread: 0
                                    color: "#40000000"
                                }
                            }

                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                y: fader.trackY + fader.trackH + 2
                                font.pixelSize: Style.settings.textTip
                                color: Style.themes.fontColor
                                text: (fader.gain > 0 ? "+" : "") + fader.gain.toFixed(1)
                            }

                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                y: fader.trackY + fader.trackH + 20
                                font.pixelSize: Style.settings.textTip
                                color: Style.themes.textColor
                                text: modelData
                            }

                            MouseArea {
                                id: drag
                                anchors.fill: parent
                                preventStealing: true
                                function apply(y) {
                                    const clamped = Math.max(fader.trackY, Math.min(fader.trackY + fader.trackH, y))
                                    const value = (fader.centerY - clamped) / (fader.trackH / 2) * panel.eqMax
                                    if (panel.engine)
                                        panel.engine.setEqBand(index, Math.round(value * 2) / 2)
                                }
                                onPressed: (mouse) => apply(mouse.y)
                                onPositionChanged: (mouse) => { if (pressed) apply(mouse.y) }
                            }
                        }
                    }
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottom: parent.bottom
                    font.pixelSize: Style.settings.textTip
                    color: Style.themes.textColor
                    text: "频段中心频率（Hz）"
                }
            }

            SettingItemCard {
                label: "重置"
                tip: "所有频段归零并关闭均衡"
                width: panel.controlWidth
                bottomLine: false
                controlItem: SButton {
                    anchors.fill: parent
                    iconCharacter: "\uf01e"
                    radius: Style.settings.labelRadius
                    buttonColor: Style.themes.secondaryColor
                    tipText: "重置均衡器"
                    onClicked: {
                        if (!panel.engine)
                            return
                        panel.engine.eqGains = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
                        panel.engine.preampDb = 0
                        panel.engine.eqEnabled = false
                    }
                }
            }
        }
    }

    QHead { width: panel.width; text: "声道" }

    Rectangle {
        width: panel.width
        color: Style.themes.primaryColor
        radius: Style.settings.cubeRadius
        height: soundRoadColumn.height
        Column {
            id: soundRoadColumn
            width: parent.width
            padding: 0

            SettingItemCard {
                label: "声道平衡"
                tip: "左负右正，0 为居中"
                width: panel.controlWidth
                controlItem: QSlider {
                    anchors.fill: parent
                    from: -1
                    to: 1
                    stepSize: 0.05
                    leftText: true
                    valueText: (value > 0 ? "+" : "") + value.toFixed(2)
                    value: panel.engine ? panel.engine.balance : 0
                    onMoved: {
                        if (panel.engine)
                            panel.engine.balance = value
                    }
                }
            }

            SettingItemCard {
                label: "立体声宽度"
                tip: "0 为单声道叠加，1 为原始，2 为超宽"
                width: panel.controlWidth
                controlItem: QSlider {
                    anchors.fill: parent
                    from: 0
                    to: 2
                    stepSize: 0.05
                    leftText: true
                    valueText: value.toFixed(2)
                    value: panel.engine ? panel.engine.stereoWidth : 1
                    onMoved: {
                        if (panel.engine)
                            panel.engine.stereoWidth = value
                    }
                }
            }

            SettingItemCard {
                label: "左声道增益"
                tip: "仅作用于左声道"
                width: panel.controlWidth
                controlItem: QSlider {
                    anchors.fill: parent
                    from: -12
                    to: 12
                    stepSize: 0.5
                    leftText: true
                    valueText: value.toFixed(1) + " dB"
                    value: panel.engine ? panel.engine.channelGainLeft : 0
                    onMoved: {
                        if (panel.engine)
                            panel.engine.channelGainLeft = value
                    }
                }
            }

            SettingItemCard {
                label: "右声道增益"
                tip: "仅作用于右声道"
                width: panel.controlWidth
                controlItem: QSlider {
                    anchors.fill: parent
                    from: -12
                    to: 12
                    stepSize: 0.5
                    leftText: true
                    valueText: value.toFixed(1) + " dB"
                    value: panel.engine ? panel.engine.channelGainRight : 0
                    onMoved: {
                        if (panel.engine)
                            panel.engine.channelGainRight = value
                    }
                }
            }

            SettingItemCard {
                label: "单声道"
                tip: "左右声道叠加后同时输出"
                width: panel.controlWidth
                controlItem: QSwitch {
                    anchors.fill: parent
                    letRight: true
                    switchTrue: panel.engine ? panel.engine.mono : false
                    onToggled: {
                        if (panel.engine)
                            panel.engine.mono = !panel.engine.mono
                    }
                }
            }

            SettingItemCard {
                label: "交换声道"
                tip: "左右对调，用于修正接反的音箱"
                width: panel.controlWidth
                bottomLine: false
                controlItem: QSwitch {
                    anchors.fill: parent
                    letRight: true
                    switchTrue: panel.engine ? panel.engine.swapChannels : false
                    onToggled: {
                        if (panel.engine)
                            panel.engine.swapChannels = !panel.engine.swapChannels
                    }
                }
            }
        }
    }

    QHead { width: panel.width; text: "增益与动态" }

    Rectangle {
        width: panel.width
        height: premiumSoundColumn.height
        color: Style.themes.primaryColor
        radius: Style.settings.cubeRadius
        Column {
            id: premiumSoundColumn
            width: parent.width
            padding: 0

            SettingItemCard {
                label: "ReplayGain"
                tip: "按标签增益归一化音量：关闭 / 单曲 / 专辑"
                width: panel.controlWidth
                controlItem: QDrop {
                    anchors.fill: parent
                    choice: panel.engine ? panel.engine.replayGainMode : 0
                    model: ["关闭", "单曲", "专辑"]
                    onTransformed: (choiced) => {
                        if (panel.engine)
                            panel.engine.replayGainMode = choiced
                    }
                }
            }

            SettingItemCard {
                label: "ReplayGain 前级"
                tip: "在标签增益基础上再整体增减"
                width: panel.controlWidth
                controlItem: QSlider {
                    anchors.fill: parent
                    from: -12
                    to: 12
                    stepSize: 0.5
                    leftText: true
                    valueText: value.toFixed(1) + " dB"
                    value: panel.engine ? panel.engine.replayGainPreampDb : 0
                    onMoved: {
                        if (panel.engine)
                            panel.engine.replayGainPreampDb = value
                    }
                }
            }

            SettingItemCard {
                label: "削波保护"
                tip: "按曲目峰值限制增益上限"
                width: panel.controlWidth
                controlItem: QSwitch {
                    anchors.fill: parent
                    letRight: true
                    switchTrue: panel.engine ? panel.engine.replayGainPreventClip : true
                    onToggled: {
                        if (panel.engine)
                            panel.engine.replayGainPreventClip = !panel.engine.replayGainPreventClip
                    }
                }
            }

            SettingItemCard {
                label: "限幅器"
                tip: "输出前的峰值保护，防止数字削波"
                width: panel.controlWidth
                controlItem: QSwitch {
                    anchors.fill: parent
                    letRight: true
                    switchTrue: panel.engine ? panel.engine.limiterEnabled : false
                    onToggled: {
                        if (panel.engine)
                            panel.engine.limiterEnabled = !panel.engine.limiterEnabled
                    }
                }
            }

            SettingItemCard {
                label: "限幅阈值"
                tip: "低于 0 dB 更安全，代价是整体响度略降"
                width: panel.controlWidth
                bottomLine: false
                controlItem: QSlider {
                    anchors.fill: parent
                    from: -12
                    to: 0
                    stepSize: 0.5
                    leftText: true
                    valueText: value.toFixed(1) + " dB"
                    value: panel.engine ? panel.engine.limiterThresholdDb : -0.5
                    onMoved: {
                        if (panel.engine)
                            panel.engine.limiterThresholdDb = value
                    }
                }
            }
        }
    }
}
